#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_HANDLER __attribute__((regcall))

void setup_syscalls();

#endif