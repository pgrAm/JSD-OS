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
#include <kernel/kassert.h>

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

extern "C" [[noreturn]] void run_user_code(void* address, void* stack);
extern "C" [[noreturn]] void switch_task_no_return(TCB* t);
extern "C" void switch_task(TCB* t);

extern TCB* current_task_TCB;

static std::vector<TCB*> running_tasks;

static size_t active_process = 0;

[[noreturn]] void switch_to_task_no_return(int pid)
{
	k_assert(running_tasks[pid]->pid != INVALID_PID);

	switch_task_no_return(running_tasks[pid]);
	__builtin_unreachable();
}

int get_running_process()
{
	if (current_task_TCB == nullptr)
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

	int pid = get_running_process();
	int next = pid;

	size_t i = running_tasks.size() - 1;
	while(i--)
	{
		next = (next + 1) % running_tasks.size();
		if(running_tasks[next]->pid != INVALID_PID)
		{
			unlock_interrupts(l);
			switch_task(running_tasks[next]);
			return;
		}
	}

	//unlock tasks
	unlock_interrupts(l);
}

extern uint8_t* init_stack;

tss* current_TSS = nullptr;

uint8_t* spare_stack = nullptr;

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

	//force page to be resident
	spare_stack = (uint8_t*)memmanager_virtual_alloc(nullptr, 1, PAGE_PRESENT | PAGE_RW); 
}

[[noreturn]] void start_user_process(process* t)
{
	run_user_code(t->objects[0]->entry_point, (void*)((uintptr_t)t->user_stack_top + PAGE_SIZE));
	__builtin_unreachable();
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

template<typename Functor>
[[noreturn]] void run_on_new_stack_no_return(Functor lambda, void* stack_addr)
{
	void (*adapter)(Functor*) = [](Functor* ptr) {
		Functor fun = *ptr;
		fun();
		__builtin_unreachable();
	};

	__asm__ volatile("movl %0, %%esp\n"
					 "pushl %1\n"
					 "call *%2"
					 :: "r"(stack_addr), "r"(&lambda), "r"(adapter) : );
	__builtin_unreachable();
}

SYSCALL_HANDLER void exit_process(int val)
{
	int current_pid = current_task_TCB->pid;

	process* current_process = running_tasks[current_pid]->p_data;

	for(auto&& object : current_process->objects)
	{
		for(auto&& seg : object->segments)
		{
			memmanager_free_pages(seg.pointer, seg.num_pages);
		}

		delete object->lib_set;
		delete object->glob_data_symbol_map;
		delete object->symbol_map;

		cleanup_elf(object.get());
	}

	memmanager_free_pages(current_process->user_stack_top, 1);

	run_on_new_stack_no_return([current_process, current_pid] ()
	{
		memmanager_free_pages(current_process->kernel_stack_top, 1);
	
		int_lock l = lock_interrupts();
	
		//we need to clean up before re-enabling interupts or else we might never get it done
		uint32_t memspace = current_process->address_space;
		//delete current_process;
		memmanager_destroy_memory_space(memspace);
		unlock_interrupts(l);
	
		int next_pid = current_process->parent_pid;
		running_tasks[current_pid]->pid = INVALID_PID;

		if(active_process == current_pid)
		{
			active_process = next_pid;
		}

		switch_to_task_no_return(next_pid);
	
	}, spare_stack + PAGE_SIZE);
}

extern "C" SYSCALL_HANDLER void spawn_process(const file_handle* file, 
											  directory_stream* cwd, int flags)
{
	if(!file || !cwd)
		return;

	uintptr_t oldcr3 = (uintptr_t)get_page_directory();
	
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

	newTask->user_stack_top	  = memmanager_virtual_alloc(nullptr, 1, PAGE_RW | PAGE_USER);
	newTask->kernel_stack_top = memmanager_virtual_alloc(nullptr, 1, PAGE_RW | PAGE_PRESENT);

	newTask->tc_block = {
		.esp	 = (uintptr_t)newTask->kernel_stack_top + PAGE_SIZE - sizeof(stack_items),
		.esp0	 = (uintptr_t)newTask->kernel_stack_top + PAGE_SIZE,
		.cr3	 = (uintptr_t)get_page_directory(),
		.tss_ptr = current_TSS,
		.pid	 = running_tasks.size(),
		.p_data	 = newTask,
	};

	//we are setting up the stack of the new process
	//these will be pop'ed into registers later
	auto* stack_ptr = ((stack_items*)newTask->tc_block.esp);
	stack_ptr->process_ptr = newTask;
	stack_ptr->eip = (uintptr_t)start_user_process; //this is where the new process will start executing
	stack_ptr->flags = (uintptr_t)0x0200; //flags are all off, except interrupt

	size_t new_process = newTask->tc_block.pid;

	//lock tasks
	running_tasks.push_back(&newTask->tc_block);
	//unlock tasks

	set_page_directory((uintptr_t*)oldcr3);

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
