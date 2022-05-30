#ifndef THREAD_H
#define THREAD_H

#include <sys/syscalls.h>

struct tls_thread_block;

tls_thread_block* get_thread_ptr();

task_id spawn_thread(void (*func)());

#endif