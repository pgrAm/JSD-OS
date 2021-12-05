#ifndef LOCKS_H
#define LOCKS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#ifndef __I386_ONLY
#define SYNC_HAS_CAS_FUNC 1
#endif

typedef uint32_t int_lock;

static inline int_lock lock_interrupts()
{
	int_lock lock;
	__asm__ volatile("pushfl\n"
		"cli\n"
		"popl %0\n"
		: "=r"(lock));
	return lock;
}

static inline void unlock_interrupts(int_lock lock)
{
	__asm__ volatile("pushl %0\n"
		"popfl\n"
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

static inline void wait_for_interrupt()
{
	__asm__ volatile("hlt");
}

typedef struct
{
	volatile int ownerPID;
	volatile uint8_t tas_lock;
} kernel_mutex;

bool kernel_try_lock_mutex(kernel_mutex* m);
void kernel_lock_mutex(kernel_mutex* m);
void kernel_unlock_mutex(kernel_mutex* m);

typedef struct
{
	volatile int waitingPID;
	volatile uint8_t unavailable;
	volatile uint8_t tas_lock;
} kernel_cv;

void kernel_signal_cv(kernel_cv* m);
void kernel_wait_cv(kernel_cv* m);
bool kernel_try_wait_cv(kernel_cv* m);

#ifdef __cplusplus

class scoped_lock {
public:
	scoped_lock(kernel_mutex* m) : m_mutex(m)
	{
		kernel_lock_mutex(m_mutex);
	}
	~scoped_lock()
	{
		kernel_unlock_mutex(m_mutex);
	}
private:
	kernel_mutex* const m_mutex;
};

}
#endif
#endif