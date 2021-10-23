#ifndef TASK_H
#define TASK_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
#include <kernel/syscall.h>

#define INVALID_PID (~(size_t)0x0)
typedef struct process process;

struct dynamic_object;
typedef struct dynamic_object dynamic_object;

#define WAIT_FOR_PROCESS 0x01

SYSCALL_HANDLER void spawn_process(const char* path, size_t path_len, int flags);
SYSCALL_HANDLER void exit_process(int val);
void run_next_task();
void run_background_tasks();
void setup_first_task();
int task_is_running(int pid);
int this_task_is_active();
void switch_to_task(int pid);
void switch_to_active_task();
int get_active_process();
int get_running_process();

#ifdef __cplusplus
}
#endif
#endif