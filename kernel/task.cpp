#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <kernel/locks.h>
#include <kernel/task.h>
#include <kernel/memorymanager.h>
#include <kernel/elf.h>
#include <kernel/filesystem.h>
#include <kernel/dynamic_object.h>
#include <kernel/tss.h>

#include <vector>
#include <memory>

using dynamic_object_ptr = std::unique_ptr<dynamic_object>;

//Thread control block
struct __attribute__((packed)) TCB //tcb man, tcb...
{
	uintptr_t esp;
	uintptr_t esp0;
	uintptr_t cr3;
	tss* tss_ptr;
	size_t pid;
	process* p_data;
};

struct process
{
	void* kernel_stack_top = nullptr;
	void* user_stack_top = nullptr;
	uintptr_t address_space = 0;
	std::vector<dynamic_object_ptr> objects;
	int parent_pid = INVALID_PID;
	TCB tc_block;
};

extern "C" void run_user_code(void* address, void* stack);
extern "C" void switch_task(TCB *t);

extern TCB* current_task_TCB;

static std::vector<TCB*> running_tasks;

static size_t active_process = 0;

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
	active_process = (active_process + 1) % running_tasks.size();
	int next = active_process;
	//unlock tasks
	unlock_interrupts(l);
	
	switch_to_task(next);
}

void run_background_tasks()
{
	int_lock l = lock_interrupts();
	//lock tasks
	int next = (get_running_process() + 1) % running_tasks.size();
	//unlock tasks
	unlock_interrupts(l);

	switch_to_task(next);
}

extern uint8_t* init_stack;

tss* current_TSS = nullptr;

void setup_first_task()
{	
	uintptr_t esp0 = (uintptr_t)init_stack + PAGE_SIZE;

	current_TSS = create_TSS(esp0);

	running_tasks.push_back(new TCB{
		.esp = 0,
		.esp0 = (uint32_t)esp0,
		.cr3 = (uint32_t)get_page_directory(),
		.tss_ptr = current_TSS,
		.pid = 0
	});
	active_process = 0;
	current_task_TCB = running_tasks[0];
}

void start_user_process(process* t)
{
	//memmanager_enter_memory_space(t->address_space);
	
	run_user_code(t->objects[0]->entry_point, (void*)((uintptr_t)t->user_stack_top + PAGE_SIZE));

	__asm__ volatile ("hlt");
}

struct __attribute__((packed)) stack_items
{
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t flags;
	uint32_t eip;
	uint32_t cs;
	process* process_ptr;
};

SYSCALL_HANDLER void exit_process(int val)
{
	int_lock l = lock_interrupts();
	int current_pid = current_task_TCB->pid;

	process* current_process = running_tasks[current_pid]->p_data;

	int next_pid = current_process->parent_pid;
	running_tasks[current_pid]->pid = INVALID_PID;

	if (active_process == current_pid)
	{
		active_process = next_pid;
	}

	for(auto&& object : current_process->objects)
	{
		for(size_t i = 0; i < object->segments.size(); i++)
		{
			memmanager_free_pages(object->segments[i].pointer, object->segments[i].num_pages);
		}
	}

	//we need to clean up before reenabline interupts or else we might never get it done
	uint32_t memspace = current_process->address_space;
	delete current_process;
	memmanager_destroy_memory_space(memspace);
	unlock_interrupts(l);

	switch_to_task(next_pid);
}

extern "C" SYSCALL_HANDLER void spawn_process(const file_handle* file, directory_stream* cwd, int flags)
{
	if(!file || !cwd)
		return;

	uint32_t oldcr3 = (uint32_t)get_page_directory();
	
	auto parent_pid = current_task_TCB->pid;
	auto address_space = memmanager_new_memory_space();

	memmanager_enter_memory_space(address_space);

	process* newTask = new process{};

	newTask->parent_pid = parent_pid;
	newTask->address_space = address_space;
	newTask->objects.emplace_back(std::make_unique<dynamic_object>(
			new dynamic_object::sym_map(),
			new dynamic_object::sym_map(),
			new dynamic_object::sym_map()
		));

	if(!load_elf(file, newTask->objects[0].get(), true, cwd))
	{
		delete newTask;
		set_page_directory((uintptr_t*)oldcr3);
		memmanager_destroy_memory_space(address_space);
		return;
	}

	newTask->user_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_USER | PAGE_RW);
	newTask->kernel_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_RW);

	newTask->tc_block = {
		.esp = (uint32_t)newTask->kernel_stack_top + PAGE_SIZE - sizeof(stack_items),
		.esp0 = (uint32_t)newTask->kernel_stack_top + PAGE_SIZE,
		.cr3 = (uint32_t)get_page_directory(),
		.tss_ptr = current_TSS,
		.pid = running_tasks.size(),
		.p_data = newTask
	};

	//we are setting up the stack of the new process
	//these will be pop'ed into registers later
	auto* stack_ptr = ((stack_items*)newTask->tc_block.esp);
	stack_ptr->process_ptr = newTask;
	stack_ptr->eip = (uint32_t)start_user_process; //this is where the new process will start executing
	stack_ptr->flags = (uint32_t)0x0200; //flags are all off, except interrupt

	size_t new_process = newTask->tc_block.pid;

	//lock tasks
	running_tasks.push_back(&newTask->tc_block);
	//unlock tasks

	set_page_directory((uint32_t*)oldcr3);

	if(flags & WAIT_FOR_PROCESS)
	{
		int_lock l = lock_interrupts();
		
		if(this_task_is_active())
		{
			active_process = new_process;
		}
		
		//unlock tasks
		unlock_interrupts(l);
		
		switch_to_task(new_process);
	}
}

void switch_to_task(int pid)
{
	if (running_tasks[pid]->pid != INVALID_PID && !task_is_running(pid))
	{
		switch_task(running_tasks[pid]);
	}
}

void switch_to_active_task()
{
	if(!this_task_is_active())
	{
		switch_to_task(get_active_process());
	}
}
