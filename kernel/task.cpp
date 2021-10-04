#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include <kernel/locks.h>
}

#include <kernel/task.h>
#include <kernel/memorymanager.h>
#include <kernel/elf.h>
#include <kernel/filesystem.h>
#include <kernel/dynamic_object.h>

#include <vector>

//Thread control block
typedef struct
{
	uint32_t esp;
	uint32_t esp0;
	uint32_t cr3;
	uint32_t pid;
	process* p_data;
} __attribute__((packed)) TCB; //tcb man, tcb...

struct process
{
	void* kernel_stack_top = nullptr;
	void* user_stack_top = nullptr;
	uintptr_t address_space = 0;
	std::vector<dynamic_object*> objects;
	int parent_pid = INVALID_PID;
	TCB tc_block;
};

extern "C" void run_user_code(void* address, void* stack);
extern "C" void switch_task(TCB *t);

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

void run_background_tasks()
{
	int_lock l = lock_interrupts();
	//lock tasks
	int next = (get_running_process() + 1) % num_processes;
	//unlock tasks
	unlock_interrupts(l);

	switch_to_task(next);
}

void setup_first_task()
{	
	running_tasks[0].esp0 = (uint32_t)tss_esp0_location;
	running_tasks[0].cr3 = (uint32_t)get_page_directory();
	running_tasks[0].pid = 0;
	active_process = 0;
	current_task_TCB = &running_tasks[0];
}

void start_user_process(process* t)
{
	//memmanager_enter_memory_space(t->address_space);
	
	run_user_code(t->objects[0]->entry_point, (void*)((uintptr_t)t->user_stack_top + PAGE_SIZE));

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

	for (size_t object_index = 0; object_index < current_process->objects.size(); object_index++)
	{
		dynamic_object* o = current_process->objects[object_index];

		for (size_t i = 0; i < o->segments.size(); i++)
		{
			memmanager_free_pages(o->segments[i].pointer, o->segments[i].num_pages);
		}

		delete o;
	}

	//we need to clean up before reenabline interupts or else we might never get it done
	uint32_t memspace = current_process->address_space;
	delete current_process;
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
	
	auto parent_pid = current_task_TCB->pid;
	auto address_space = memmanager_new_memory_space();

	memmanager_enter_memory_space(address_space);

	process* newTask = new process{};

	newTask->parent_pid = parent_pid;
	newTask->address_space = address_space;

	dynamic_object* d = new dynamic_object();
	d->symbol_map = new hash_map();
	d->glob_data_symbol_map = new hash_map();
	d->lib_set = new hash_map();

	newTask->objects.push_back(d);

	if(!load_elf(path, newTask->objects[0], true))
	{
		delete newTask;
		set_page_directory((uint32_t*)oldcr3);
		memmanager_destroy_memory_space(address_space);
		return;
	}

	newTask->user_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_USER | PAGE_PRESENT | PAGE_RW);
	newTask->kernel_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_PRESENT | PAGE_RW);
	newTask->tc_block.esp0 = (uint32_t)newTask->kernel_stack_top + PAGE_SIZE;
	newTask->tc_block.esp = newTask->tc_block.esp0 - (RESERVED_STACK_WORDS * sizeof(uint32_t));
	newTask->tc_block.cr3 = (uint32_t)get_page_directory();
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

	set_page_directory((uint32_t*)oldcr3);

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
	if (running_tasks[pid].pid != INVALID_PID && !task_is_running(pid))
	{
		switch_task(&running_tasks[pid]);
	}
}