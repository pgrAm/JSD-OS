#ifndef LOCKS_H
#define LOCKS_H
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t int_lock;

static inline int_lock lock_interrupts()
{
	int_lock lock;
	__asm__ volatile("pushf\n"
		"cli\n"
		"popl %0\n"
		: "=r"(lock));
	return lock;
}

static inline void unlock_interrupts(int_lock lock)
{
	__asm__ volatile("pushl %0\n"
		"popf\n"
		:
	: "r" (lock));
}

static inline bool acquire_lock(volatile int* _lock)
{
	return __sync_bool_compare_and_swap(_lock, 0, 1);
}

static inline void release_lock(volatile int* _lock)
{
	__asm__ __volatile__("" ::: "memory");
	*_lock = 0;
}

typedef struct
{
	volatile int ownerPID;
} kernel_mutex;

bool kernel_try_lock_mutex(kernel_mutex* m);
void kernel_lock_mutex(kernel_mutex* m);
void kernel_unlock_mutex(kernel_mutex* m);

#endif