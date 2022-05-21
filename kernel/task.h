#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <stdint.h>
#include <kernel/syscall.h>
#include <kernel/filesystem.h>
#include <common/task_data.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct process process;

struct dynamic_object;
typedef struct dynamic_object dynamic_object;


SYSCALL_HANDLER task_id spawn_process(const file_handle* file,
									  directory_stream* cwd, int flags);
SYSCALL_HANDLER void exit_process(int val);

SYSCALL_HANDLER task_id spawn_thread(void* function_ptr);
SYSCALL_HANDLER void exit_thread(int val);

void run_next_task();
void run_background_tasks();
void setup_first_task();
task_id task_is_running(task_id pid);
task_id this_task_is_active();
void switch_to_task(task_id pid);
void switch_to_active_task();
task_id get_active_process();
task_id get_running_process();

#ifdef __cplusplus
}
#endif
#endif