#include "locks.h"
#include "task.h"

#ifdef SYNC_HAS_CAS_FUNC
bool tas_aquire(volatile uint8_t* l)
{
	return __sync_val_compare_and_swap(l, 0, 1);
}

void tas_release(volatile uint8_t* l)
{
	__asm__ __volatile__("" ::: "memory");
	l = 0;
}

inline int cas_func(volatile uint8_t* tas_lock, volatile int* ptr, int oldval, int newval)
{
	return __sync_val_compare_and_swap(ptr, oldval, newval);
}

void cas_atomic_set(volatile uint8_t* tas_lock, volatile int* ptr, int newval)
{
	__asm__ __volatile__("" ::: "memory");
	*ptr = newval;
}
#else
bool tas_aquire(volatile uint8_t* l)
{
	return __sync_lock_test_and_set(l, 1);
}

void tas_release(volatile uint8_t* l)
{
	__sync_lock_release(l);
}

inline int cas_func(volatile uint8_t* tas_lock, volatile int* ptr, int oldval, int newval)
{
	while(tas_aquire(tas_lock) != 0);
	int old_ptr_val = *ptr;
	if(*ptr == oldval)
	{
		*ptr = newval;
	}
	tas_release(tas_lock);
	return old_ptr_val;
}

void cas_atomic_set(volatile uint8_t* tas_lock, volatile int* ptr, int newval)
{
	while(tas_aquire(tas_lock) != 0);
	__asm__ __volatile__("" ::: "memory");
	*ptr = newval;
	tas_release(tas_lock);
}

#endif

inline bool do_try_lock_mutex(kernel_mutex* m, int* pid)
{
	*pid = cas_func(&m->tas_lock, &m->ownerPID, INVALID_PID, get_running_process());
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
	while (!do_try_lock_mutex(m, &pid))
	{
		switch_to_task(pid);
	}
}

void kernel_unlock_mutex(kernel_mutex* m)
{
	cas_atomic_set(&m->tas_lock, &m->ownerPID, INVALID_PID);
	switch_to_active_task();
}

void kernel_signal_cv(kernel_cv* m)
{
	tas_release(&m->unavailable);
	switch_to_active_task();
}

inline bool do_try_wait_cv(kernel_cv* m, int current_pid)
{
	int pid = cas_func(&m->tas_lock, &m->waitingPID, INVALID_PID, current_pid);
	if(pid == INVALID_PID) //locked
	{
		while(true)
		{
			if(tas_aquire(&m->unavailable) == 0)
			{
				cas_atomic_set(&m->tas_lock, &m->waitingPID, INVALID_PID);
				return true;
			}
			run_background_tasks();
		}
	}
	return false;
}

void kernel_wait_cv(kernel_cv* m)
{
	int pid = get_running_process();
	while(!do_try_wait_cv(m, pid))
	{
		run_background_tasks();
	}
}

bool kernel_try_wait_cv(kernel_cv* m)
{
	return do_try_wait_cv(m, get_running_process());
}