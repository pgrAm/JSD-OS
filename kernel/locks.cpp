#include "locks.h"
#include "task.h"

#include <stdio.h>

#ifdef SYNC_HAS_CAS_FUNC
static inline bool tas_aquire(uint8_t* l)
{
	return __sync_val_compare_and_swap(l, 0, 1);
}

static inline void tas_release(uint8_t* l)
{
	__atomic_store_n(l, 0, __ATOMIC_RELEASE);
}

static inline int cas_func(lockable_val* ptr, int oldval, int newval)
{
	return __sync_val_compare_and_swap(&ptr->value, oldval, newval);
}

#else
static inline bool tas_aquire(uint8_t* l)
{
	return __sync_lock_test_and_set(l, 1);
}

static inline void tas_release(uint8_t* l)
{
	__sync_lock_release(l);
}

static inline int_lock atomic_lock_aquire(uint8_t* tas_lock)
{
	while(true)
	{
		int_lock l = lock_interrupts();
		if(tas_aquire(tas_lock) == 0)
			return l;
		unlock_interrupts(l);
	}
}

static inline void atomic_lock_release(int_lock l, uint8_t* tas_lock)
{
	tas_release(tas_lock);
	unlock_interrupts(l);
}

static inline int cas_func(lockable_val* ptr, int oldval, int newval)
{
	int_lock l = atomic_lock_aquire(&ptr->tas_lock);

	int old_ptr_val = ptr->value;
	if(old_ptr_val == oldval)
	{
		ptr->value = newval;
	}

	atomic_lock_release(l, &ptr->tas_lock);

	return old_ptr_val;
}

#endif

static inline bool do_try_lock_mutex(kernel_mutex* m, int* pid)
{
	*pid = cas_func(&m->ownerPID, INVALID_PID, get_running_process());
	return *pid == INVALID_PID;
}

bool kernel_try_lock_mutex(kernel_mutex* m)
{
	int pid;
	return do_try_lock_mutex(m, &pid);
}

void kernel_lock_mutex(kernel_mutex* m)
{
	int pid;
	while(!do_try_lock_mutex(m, &pid))
	{
		switch_to_task(pid);
	}
}

void kernel_unlock_mutex(kernel_mutex* m)
{
	__atomic_store_n(&m->ownerPID.value, INVALID_PID, __ATOMIC_RELEASE);
	switch_to_active_task();
}

void kernel_signal_cv(kernel_cv* m)
{
	tas_release(&m->unavailable);
	switch_to_active_task();
}

void kernel_wait_cv(kernel_mutex* locked_mutex, kernel_cv* m)
{
	while(true)
	{
		if(tas_aquire(&m->unavailable) == 0)
		{
			kernel_unlock_mutex(locked_mutex);
			return;
		}
		run_background_tasks();
	}
}