#ifndef TASK_H
#define TASK_H
#include <stddef.h>
#include <stdint.h>

#include <dynamic_object.h>
#include <syscall.h>

#define INVALID_PID (~(size_t)0x0)
typedef struct process process;

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
	void* kernel_stack_top;
	void* user_stack_top;
	uintptr_t address_space;
	dynamic_object** objects;
	size_t num_objects;
	int parent_pid;
	TCB tc_block;
};

#define WAIT_FOR_PROCESS 0x01

SYSCALL_HANDLER void spawn_process(const char* path, int flags);
SYSCALL_HANDLER void exit_process(int val);
void run_next_task();
void setup_first_task();
int task_is_running(int pid);
int this_task_is_active();
void switch_to_task(int pid);
int get_active_process();
int get_running_process();

#endif