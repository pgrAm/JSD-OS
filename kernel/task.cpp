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

class task;

//Thread control block
struct __attribute__((packed)) TCB //tcb man, tcb...
{
	uintptr_t esp;
	uintptr_t esp0;
	uintptr_t cr3;
	tss* tss_ptr;

	uint32_t tls_gdt_hi;
	uint16_t tls_base_lo;
	uint16_t unused;

	task_id tid;
	task* t_data;
};

struct __attribute__((packed)) stack_items
{
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t flags;
	uint32_t eip;
	uint32_t cs;
	uintptr_t code_addr;
	uintptr_t stack_addr;
};

tss* current_TSS = nullptr;

struct process
{
	uintptr_t address_space = 0;
	std::vector<dynamic_object_ptr> objects;
	task_id parent_pid = INVALID_TASK_ID;
	task_id pid;
	std::vector<task*> tasks;
	sync::mutex mtx;
};

class task
{
public:
	constexpr task(task_id _tid, process* parent, uintptr_t _user_stack_top,
				   uintptr_t _kernel_stack_top,
				   uintptr_t tls_ptr,
				   tss* _tss = current_TSS,
				   uintptr_t pdir = std::bit_cast<uintptr_t>(get_page_directory()))
		: kernel_stack_top{_kernel_stack_top}
		, user_stack_top{_user_stack_top}
		, p_data{parent}
		, tc_block{
			.esp	 = _kernel_stack_top + PAGE_SIZE - sizeof(stack_items),
			.esp0	 = _kernel_stack_top + PAGE_SIZE,
			.cr3	 = pdir,
			.tss_ptr = _tss,
			.tls_gdt_hi =
				(tls_ptr & 0xFF000000) | 0x00CFF200 | ((tls_ptr >> 16) & 0xFF),
			.tls_base_lo = static_cast<uint16_t>(tls_ptr & 0xFFFF),
			.tid	 = _tid,
			.t_data	 = this,
		}
	{}

	uintptr_t kernel_stack_top = (uintptr_t)nullptr;
	uintptr_t user_stack_top   = (uintptr_t)nullptr;
	process* p_data;
	TCB tc_block;
};

extern "C" [[noreturn]] void run_user_code(void* address, void* stack);
extern "C" [[noreturn]] void switch_task_no_return(TCB* t);
extern "C" void switch_task(TCB* t);
alignas(4096) constinit uint8_t init_stack[PAGE_SIZE];
uint8_t* spare_stack = nullptr;

constinit process init_process{.pid = 0};
constinit task init_task{
	0,
	&init_process,
	0,//std::bit_cast<uintptr_t>(nullptr),
	0,//std::bit_cast<uintptr_t>(init_stack),
	0xFFFFFFFF,
	nullptr,
	0,
};

TCB* current_task_TCB = &init_task.tc_block;

static std::vector<TCB*> running_tasks;

static task_id active_process = 0;

[[noreturn]] void switch_to_task_no_return(task_id pid)
{
	k_assert(running_tasks[pid]->tid != INVALID_TASK_ID);

	switch_task_no_return(running_tasks[pid]);
	__builtin_unreachable();
}

task_id get_running_process()
{
	return current_task_TCB->tid;
}

task_id get_active_process()
{
	return active_process;
}

task_id task_is_running(task_id tid)
{
	return (get_running_process() == tid);
}

task_id this_task_is_active()
{
	return current_task_TCB->t_data->p_data->pid == active_process;
	//return task_is_running(active_process);
}

void run_next_task()
{
	int_lock l = lock_interrupts();
	//lock tasks
	active_process = (active_process + 1) % running_tasks.size();
	task_id next   = active_process;
	//unlock tasks
	unlock_interrupts(l);
	
	switch_to_task(next);
}

void run_background_tasks()
{
	int_lock l = lock_interrupts();
	//lock tasks

	task_id pid	 = get_running_process();
	task_id next = pid;

	size_t i = running_tasks.size() - 1;
	while(i--)
	{
		next = (next + 1) % running_tasks.size();
		if(running_tasks[next]->tid != INVALID_TASK_ID)
		{
			unlock_interrupts(l);
			switch_task(running_tasks[next]);
			return;
		}
	}

	//unlock tasks
	unlock_interrupts(l);
}

void setup_first_task()
{
	uintptr_t esp0	 = std::bit_cast<uintptr_t>(&init_stack[0]) + PAGE_SIZE;
	current_TSS = create_TSS(esp0);

	init_task.kernel_stack_top = esp0;
	init_task.tc_block.esp0	   = esp0;
	init_task.tc_block.esp	   = esp0 - sizeof(stack_items);
	init_task.tc_block.tss_ptr = current_TSS;
	init_task.tc_block.cr3	   = (uintptr_t)get_page_directory();

	sync::lock_guard l{init_process.mtx};
	init_process.tasks.push_back(&init_task);
	running_tasks.push_back(&init_task.tc_block);

	active_process	 = 0;
	current_task_TCB = running_tasks.back();

	//force page to be resident
	spare_stack =
		(uint8_t*)memmanager_virtual_alloc(nullptr, 1, PAGE_PRESENT | PAGE_RW);
}

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

[[noreturn]] static void switch_task_post_terminate(task* current)
{
	auto old_id = current->tc_block.tid;

	{
		sync::lock_guard l{current->p_data->mtx};

		auto& tasks = current->p_data->tasks;

		auto it = std::find(tasks.cbegin(), tasks.cend(), current);
		assert(it != tasks.end());
		tasks.erase(it);
	}

	task_id next_pid	  = current->p_data->parent_pid;
	current->tc_block.tid = INVALID_TASK_ID;

	if(active_process == old_id)
	{
		active_process = next_pid;
	}

	switch_to_task_no_return(next_pid);
	__builtin_unreachable();
}

SYSCALL_HANDLER void exit_thread(int val)
{
	auto current = current_task_TCB->t_data;

	memmanager_free_pages((void*)current->user_stack_top, 1);

	run_on_new_stack_no_return(
		[current]()
		{
			memmanager_free_pages((void*)current->kernel_stack_top, 1);

			switch_task_post_terminate(current);
		},
		spare_stack + PAGE_SIZE);

	__builtin_unreachable();
}

[[noreturn]] void end_last_task(task* current)
{
	memmanager_free_pages((void*)current->user_stack_top, 1);

	run_on_new_stack_no_return(
		[current]()
		{
			memmanager_free_pages((void*)current->kernel_stack_top, 1);

			int_lock l = lock_interrupts();

			//we need to clean up before re-enabling interupts or else we might never get it done
			uint32_t memspace = current->p_data->address_space;
			//delete current_process;
			memmanager_destroy_memory_space(memspace);
			unlock_interrupts(l);

			switch_task_post_terminate(current);
		},
		spare_stack + PAGE_SIZE);

	__builtin_unreachable();
}

SYSCALL_HANDLER void exit_process(int val)
{
	auto current_process = current_task_TCB->t_data->p_data;
	auto current_task	 = current_task_TCB->t_data;

	auto& tasks = current_process->tasks;

	//wait for all other tasks to join
	while(true)
	{
		current_process->mtx.lock();
		if(tasks.size() != 1)
		{
			//find the first task that isn't this
			auto it = std::find_if_not(tasks.begin(), tasks.end(),
									   [current_task](auto& t)
									   { return t == current_task; });
			assert(it != tasks.end());
			current_process->mtx.unlock();

			switch_task(&(*it)->tc_block);
		}
		current_process->mtx.unlock();
		break;
	}

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

	auto last_task = current_process->tasks[0];

	delete current_process;

	end_last_task(last_task);
}

SYSCALL_HANDLER void get_process_info(process_info* data)
{
	*data = process_info{
		.pid = current_task_TCB->t_data->p_data->pid,
		.tls = current_task_TCB->t_data->p_data->objects[0]->tls_image,
	};
}

SYSCALL_HANDLER void set_tls_addr(void* ptr)
{
	auto tls_ptr = std::bit_cast<uintptr_t>(ptr);

	current_task_TCB->tls_gdt_hi =
		(tls_ptr & 0xFF000000) | 0x00CFF200 | ((tls_ptr >> 16) & 0xFF);
	current_task_TCB->tls_base_lo = static_cast<uint16_t>(tls_ptr & 0xFFFF);
	set_TLS_seg_base(std::bit_cast<uintptr_t>(ptr));
}

task* create_new_task(process* parent, void* execution_addr, void* tls_ptr, size_t args_size)
{
	uintptr_t user_stack_top =
		(uintptr_t)memmanager_virtual_alloc(nullptr, 1, PAGE_RW | PAGE_USER);
	uintptr_t kernel_stack_top =
		(uintptr_t)memmanager_virtual_alloc(nullptr, 1, PAGE_RW | PAGE_PRESENT);

	auto new_task =
		new task{running_tasks.size(), parent, user_stack_top, kernel_stack_top,
				 std::bit_cast<uintptr_t>(tls_ptr)};

	sync::lock_guard l{parent->mtx};
	parent->tasks.push_back(new_task);

	//we are setting up the stack of the new process
	//these will be pop'ed into registers later
	auto* stack_ptr		  = ((stack_items*)new_task->tc_block.esp);
	stack_ptr->code_addr  = (uintptr_t)execution_addr;
	stack_ptr->stack_addr = (user_stack_top + PAGE_SIZE) -
							(sizeof(uintptr_t) + sizeof(task_id) + args_size);
	//this is where the new process will start executing
	stack_ptr->eip	 = (uintptr_t)run_user_code; 
	stack_ptr->flags = (uintptr_t)0x0200; //flags are all off, except interrupt

	//pass this on the user stack, the value above this is the "return addr"
	*(task_id*)(stack_ptr->stack_addr + sizeof(uintptr_t)) =
		new_task->tc_block.tid;

	//lock tasks
	running_tasks.push_back(&new_task->tc_block);
	//unlock tasks
	return new_task;
}

extern "C" SYSCALL_HANDLER task_id spawn_thread(void* function_ptr, void* tls_ptr)
{
	auto parent_process = current_task_TCB->t_data->p_data;

	auto new_task = create_new_task(parent_process, function_ptr, tls_ptr, 0);

	return new_task->tc_block.tid;
}

extern "C" SYSCALL_HANDLER void yield_to(task_id tid)
{
	auto is_active = this_task_is_active();

	if(tid < running_tasks.size() && is_active)
	{
		switch_to_task(tid);
	}
	else if(is_active)
	{
		run_background_tasks();
	}
	else
	{
		switch_to_active_task();
	}
}

extern "C" SYSCALL_HANDLER task_id spawn_process(const file_handle* file,
												 const void* arg_ptr,
												 size_t args_size,
												 int flags)
{
	if(!file)
		return INVALID_TASK_ID;

	//TODO: check the mapping of arg_ptr
	std::vector<uint8_t> args_buf((uint8_t*)arg_ptr, args_size);

	uintptr_t oldcr3 = (uintptr_t)get_page_directory();
	
	auto parent_pid = current_task_TCB->t_data->p_data->pid;
	auto address_space = memmanager_new_memory_space();

	memmanager_enter_memory_space(address_space);

	process* new_process = new process{};

	new_process->parent_pid = parent_pid;
	new_process->address_space = address_space;
	new_process->objects.emplace_back(
		std::make_unique<dynamic_object>(
			new dynamic_object::sym_map(),
			new dynamic_object::sym_map(),
			new dynamic_object::sym_map()
		));

	assert(!!file->dir_path);
	fs::dir_stream cwd{nullptr, *file->dir_path, 0};

	if(!load_elf(file, new_process->objects[0].get(), true, cwd.get_ptr()))
	{
		delete new_process;
		set_page_directory(oldcr3);
		memmanager_destroy_memory_space(address_space);
		return INVALID_TASK_ID;
	}

	auto new_task = create_new_task(new_process, new_process->objects[0]->entry_point, nullptr, args_size + sizeof(size_t));

	auto* stack_ptr = ((stack_items*)new_task->tc_block.esp);


	memcpy((void*)(stack_ptr->stack_addr + sizeof(uintptr_t) + sizeof(size_t)),
		   &args_size, sizeof(size_t));
	memcpy((void*)(stack_ptr->stack_addr + sizeof(uintptr_t) +
				   2 * sizeof(size_t)),
		   &args_buf[0], args_buf.size());

	auto new_pid = new_task->tc_block.tid;

	new_process->pid = new_pid;

	set_page_directory(oldcr3);

	if(flags & WAIT_FOR_PROCESS)
	{
		int_lock l = lock_interrupts();
		
		if(this_task_is_active())
		{
			active_process = new_pid;
		}
		
		//unlock tasks
		unlock_interrupts(l);

		switch_task(&new_task->tc_block);
	}

	return new_pid;
}

void switch_to_task(task_id tid)
{
	assert(tid < running_tasks.size());
	if(running_tasks[tid]->tid != INVALID_TASK_ID && !task_is_running(tid))
	{
		switch_task(running_tasks[tid]);
	}
}

void switch_to_active_task()
{
	if(!this_task_is_active())
	{
		switch_to_task(get_active_process());
	}
}