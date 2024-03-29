#include "locks.h"
#include "task.h"

#include <stdio.h>
#include <bit>

using cas_type = decltype(lockable_val::value);

#ifdef SYNC_HAS_CAS_FUNC
static inline bool tas_aquire(uint8_t* l)
{
	return __sync_val_compare_and_swap(l, 0, 1);
}

static inline void tas_release(uint8_t* l)
{
	__atomic_store_n(l, 0, __ATOMIC_RELEASE);
}

template<typename T>
static inline int cas_func(lockable_val* ptr, T oldval, T newval)
{
	return std::bit_cast<T>(
		__sync_val_compare_and_swap(&ptr->value,
									std::bit_cast<cas_type>(oldval),
									std::bit_cast<cas_type>(newval)));
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

static inline int_lock atomic_lock_aquire(lockable_val* ptr)
{
#ifndef EMULATE_CAS_W_TAS
	return lock_interrupts();
#else
	while(true)
	{
		int_lock l = lock_interrupts();
		if(tas_aquire(&ptr->tas_lock) == 0)
			return l;
		unlock_interrupts(l);
	}
#endif
}

static inline void atomic_lock_release(int_lock l, lockable_val* ptr)
{
#ifdef EMULATE_CAS_W_TAS
	tas_release(&ptr->tas_lock);
#endif
	unlock_interrupts(l);
}

template<typename T>
static inline T cas_func(lockable_val* ptr, T oldval, T newval)
{
	int_lock l = atomic_lock_aquire(ptr);

	T old_ptr_val = std::bit_cast<T>(ptr->value);
	if(old_ptr_val == oldval) { ptr->value = std::bit_cast<int>(newval); }

	atomic_lock_release(l, ptr);

	return old_ptr_val;
}

#endif

static inline task_id do_try_lock_mutex(kernel_mutex* m, task_id my_pid)
{
	return cas_func(&m->ownerPID, INVALID_TASK_ID, my_pid);
}

bool kernel_try_lock_mutex(kernel_mutex* m)
{
	return do_try_lock_mutex(m, get_running_task_id()) == INVALID_TASK_ID;
}

void kernel_lock_mutex(kernel_mutex* m)
{
	const auto my_pid = get_running_task_id();
	while(true)
	{
		auto pid = do_try_lock_mutex(m, my_pid);
		if(pid == INVALID_TASK_ID) [[likely]]
			break;
		switch_to_task(pid);
	}
}

void kernel_unlock_mutex(kernel_mutex* m)
{
	__atomic_store_n(&m->ownerPID.value, std::bit_cast<cas_type>(INVALID_TASK_ID),
					 __ATOMIC_RELEASE);
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

/*void wait_umtex(uint32_t* mutex)
{
	kernel_lock_mutex(mutex);
}*/