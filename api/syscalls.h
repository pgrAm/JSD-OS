#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>

static inline int do_syscall_1(size_t syscall_index, const char *arg)
{
	//"int 0x80" parm[ebx] [eax] value [eax] modifies [ecx edx];
	int retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval)
						:"b"(syscall_index), "a"(arg)
						:"%ecx", "%edx");
	return retval;
}

static inline void print_string(const char *a)
{
	do_syscall_1(0, a);
}

#endif