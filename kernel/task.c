#include <stdint.h>
#include <stdlib.h>

#include "task.h"
#include "memorymanager.h"
#include "filesystem.h"
#include "elf.h"

extern void run_user_code(void* address, void* stack);
extern void switch_task(TCB *t);

extern TCB* current_task_TCB;
extern uint32_t* tss_esp0_location;

size_t num_processes = 1;

TCB running_tasks[3];

size_t active_process = -1;

int get_active_process()
{
	return active_process;
}

int task_is_running(int pid)
{
	return (current_task_TCB->pid == pid);
}

int this_task_is_active()
{
	return task_is_running(active_process);
}

void run_next_task()
{
	int_lock l = lock_interrupts();
	
	//lock tasks
	active_process = (active_process + 1) % num_processes;
	//unlock tasks
	
	printf("\n%d:\n", active_process);
	
	unlock_interrupts(l);
	
	switch_to_task(active_process);
}

void setup_first_task()
{	
	running_tasks[0].esp0 = *tss_esp0_location;
	running_tasks[0].cr3 = (uint32_t)get_page_directory();
	running_tasks[0].pid = 0;
	active_process = 0;
	current_task_TCB = &running_tasks[0];
}

void start_user_process(process* t)
{
	memmanager_enter_memory_space(t->address_space);
	
	run_user_code(t->entry_point, t->user_stack_top + PAGE_SIZE);
	
	__asm__ volatile ("hlt");
}

SYSCALL_HANDLER void spawn_process(const char* p, int flags)
{
	//we need to copy the path onto the stack 
	//otherwise it will be in the address space of another process
	int pathlen = strlen(p);
	if (pathlen > 80 - 1)
	{
		puts("Cannot launch process: filename too long\n");
		return;
	}
	char path[80];
	memcpy(path, p, pathlen + 1);

	uint32_t oldcr3 = get_page_directory();
	
	process* newTask = (process*)malloc(sizeof(process));
	
	newTask->address_space = memmanager_new_memory_space();
	
	//printf("created new memory space\n");
	
	memmanager_enter_memory_space(newTask->address_space);
	
	newTask->kernel_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_PRESENT | PAGE_RW);	
	
	newTask->tc_block.esp0 = (uint32_t)newTask->kernel_stack_top + PAGE_SIZE;
	newTask->tc_block.esp = newTask->tc_block.esp0 - (8 * 4);
	newTask->tc_block.cr3 = memmanager_get_physical(newTask->address_space);
	newTask->tc_block.pid = num_processes++;
	
	if(!load_elf(path, newTask))
	{
		set_page_directory(oldcr3);
		memmanager_destroy_memory_space(newTask->address_space);
		return;
	}
	
	((uint32_t*)newTask->tc_block.esp)[7] = (uint32_t)newTask; 
	((uint32_t*)newTask->tc_block.esp)[5] = (uint32_t)start_user_process; //this is where the new process will start executing
	((uint32_t*)newTask->tc_block.esp)[4] = (uint32_t)0x0200; //flags are all off, except interrupt
	
	uint32_t this_process = newTask->tc_block.pid;
	
	//lock tasks
	running_tasks[this_process] = newTask->tc_block;
	//unlock tasks
	
	if(flags & WAIT_FOR_PROCESS)
	{
		int_lock l = lock_interrupts();
		
		if(this_task_is_active())
		{
			active_process = this_process;
		}
		
		//unlock tasks
		unlock_interrupts(l);
		
		switch_to_task(this_process);
	}
}

void switch_to_task(int pid)
{
	switch_task(&running_tasks[pid]);
}