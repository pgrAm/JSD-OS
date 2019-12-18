#ifndef TASK_H
#define TASK_H
#include <stddef.h>
#include <stdint.h>

#include "syscall.h"

//Thread control block
typedef struct
{
	uint32_t esp;
	uint32_t esp0;
	uint32_t cr3;
	uint32_t pid;
} __attribute__((packed)) TCB; //tcb man, tcb...

typedef struct
{
	void* pointer;
	size_t num_pages;
} segment;

typedef struct
{
	void* kernel_stack_top;
	void* user_stack_top;
	void* entry_point;
	uint32_t address_space;
	segment* segments;
	size_t num_segments;
	
	TCB tc_block;
} process;

#define WAIT_FOR_PROCESS 0x01

SYSCALL_HANDLER void spawn_process(const char* path, int flags);
void run_next_task();
void setup_first_task();
int task_is_running(int pid);
int this_task_is_active();
void switch_to_task(int pid);
int get_active_process();

typedef uint32_t int_lock;

static inline int_lock lock_interrupts()
{
	int_lock lock;
	__asm__ volatile(	"pushf\n"
						"cli\n"
						"popl %0\n"
						: "=r"(lock));
	return lock;
}

static inline void unlock_interrupts(int_lock lock)
{
	__asm__ volatile(	"pushl %0\n"
						"popf\n"
						: 
						: "r" (lock));
}

#endif