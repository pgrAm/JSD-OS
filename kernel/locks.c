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