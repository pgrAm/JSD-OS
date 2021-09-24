#include "locks.h"
#include "task.h"

bool kernel_try_lock_mutex(kernel_mutex* m)
{
	int pid = __sync_val_compare_and_swap(&m->ownerPID, INVALID_PID, get_running_process());
	return pid == INVALID_PID;
}

void kernel_lock_mutex(kernel_mutex* m)
{
	while (true)
	{
		int pid = __sync_val_compare_and_swap(&m->ownerPID, INVALID_PID, get_running_process());
		if (pid == INVALID_PID) return; //locked
		switch_to_task(pid);
	}
}

void kernel_unlock_mutex(kernel_mutex* m)
{
	__asm__ __volatile__("" ::: "memory");
	m->ownerPID = INVALID_PID;
	if (!this_task_is_active())
	{
		switch_to_task(get_active_process());
	}
}

void kernel_signal_cv(kernel_cv* m)
{
	__asm__ __volatile__("" ::: "memory");
	m->available = 1;
	
	if(!this_task_is_active())
	{
		switch_to_task(get_active_process());
	}
}

inline bool do_try_wait_cv(kernel_cv* m, int current_pid)
{
	int pid = __sync_val_compare_and_swap(&m->waitingPID, INVALID_PID, current_pid);
	if(pid == INVALID_PID) //locked
	{
		while(true)
		{
			int old = __sync_val_compare_and_swap(&m->available, 1, 0);
			if(old == 1)
			{
				__asm__ __volatile__("" ::: "memory");
				m->waitingPID = INVALID_PID;
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