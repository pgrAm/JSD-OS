#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "task.h"
#include "memorymanager.h"
#include "filesystem.h"
#include "elf.h"
#include "locks.h"

extern void run_user_code(void* address, void* stack);
extern void switch_task(TCB *t);

extern TCB* current_task_TCB;
extern uint32_t* tss_esp0_location;

size_t num_processes = 1;
const size_t max_processes = 10;
//this should be something like an std::vector
TCB running_tasks[max_processes];

size_t active_process = 0;

int get_running_process()
{
	if (current_task_TCB == NULL)
	{
		return 0;
	}
	return current_task_TCB->pid;
}

int get_active_process()
{
	return active_process;
}

int task_is_running(int pid)
{
	return (get_running_process() == pid);
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
	int next = active_process;
	//unlock tasks
	unlock_interrupts(l);
	
	switch_to_task(next);
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
	
	run_user_code(t->objects[0]->entry_point, t->user_stack_top + PAGE_SIZE);

	__asm__ volatile ("hlt");
}

enum stackitems
{
	EBP_REGISTER = 0,
	EDI_REGISTER,
	ESI_REGISTER,
	EBX_REGISTER,
	FLAGS_REGISTER,
	EIP_REGISTER,
	CS_REGISTER,
	NEW_PROCESS_PTR,
	RESERVED_STACK_WORDS
};

SYSCALL_HANDLER void exit_process(int val)
{
	int_lock l = lock_interrupts();
	int current_pid = current_task_TCB->pid;

	process* current_process = running_tasks[current_pid].p_data;

	int next_pid = current_process->parent_pid;
	running_tasks[current_pid].pid = INVALID_PID;

	if (active_process == current_pid)
	{
		active_process = next_pid;
	}

	for (size_t object_index = 0; object_index < current_process->num_objects; object_index++)
	{
		dynamic_object* o = current_process->objects[object_index];

		for (size_t i = 0; i < o->num_segments; i++)
		{
			memmanager_free_pages(o->segments[i].pointer, o->segments[i].num_pages);
		}

		free(o);
	}
	free(current_process->objects);

	//we need to clean up before reenabline interupts or else we might never get it done
	uint32_t memspace = current_process->address_space;
	free(current_process);
	memmanager_destroy_memory_space(memspace);
	unlock_interrupts(l);

	switch_to_task(next_pid);
}

SYSCALL_HANDLER void spawn_process(const char* p, int flags)
{
	if (num_processes >= max_processes)
	{
		puts("Too Many processes");
		return;
	}

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

	uint32_t oldcr3 = (uint32_t)get_page_directory();
	
	process* newTask = (process*)malloc(sizeof(process));
	
	newTask->parent_pid = current_task_TCB->pid;
	newTask->address_space = memmanager_new_memory_space();
	memmanager_enter_memory_space(newTask->address_space);

	newTask->num_objects = 1;
	newTask->objects = (dynamic_object**)malloc(sizeof(dynamic_object*));
	newTask->objects[0] = (dynamic_object*)malloc(sizeof(dynamic_object));
	newTask->objects[0]->symbol_map = hashmap_create(16);
	newTask->objects[0]->glob_data_symbol_map = hashmap_create(16);
	newTask->objects[0]->lib_set = hashmap_create(16);

	if(!load_elf(path, newTask->objects[0], true))
	{
		set_page_directory((uint32_t*)oldcr3);
		memmanager_destroy_memory_space(newTask->address_space);
		return;
	}

	newTask->user_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_USER | PAGE_PRESENT | PAGE_RW);
	newTask->kernel_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_PRESENT | PAGE_RW);
	newTask->tc_block.esp0 = (uint32_t)newTask->kernel_stack_top + PAGE_SIZE;
	newTask->tc_block.esp = newTask->tc_block.esp0 - (RESERVED_STACK_WORDS * sizeof(uint32_t));
	newTask->tc_block.cr3 = memmanager_get_physical(newTask->address_space);
	newTask->tc_block.pid = num_processes++;
	newTask->tc_block.p_data = newTask;

	//we are setting up the stack of the new process
	//these will be pop'ed into registers later
	uint32_t* stack_ptr = ((uint32_t*)newTask->tc_block.esp);
	stack_ptr[NEW_PROCESS_PTR] = (uint32_t)newTask;
	stack_ptr[EIP_REGISTER] = (uint32_t)start_user_process; //this is where the new process will start executing
	stack_ptr[FLAGS_REGISTER] = (uint32_t)0x0200; //flags are all off, except interrupt

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
	if (running_tasks[pid].pid != INVALID_PID)
	{
		switch_task(&running_tasks[pid]);
	}
}