#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef __KERNEL
#include <sys/syscalls.h>
#else
#include <kernel/locks.h>
#include <kernel/sys/syscalls.h>
#endif

#define _HAVE_UINTPTR_T

#include "liballoc.h"

int atoi(const char * str)
{
	while(isspace(*str))
	{
		str++;
	}
	
	int8_t sign = +1;
	
	if(*str == '-')
	{
		str++;
		sign = -1;
	}
	else if(*str == '+')
	{
		str++;
	}
	
	int n = 0;
	
	while(isdigit(*str))
	{
		n = n * 10 + *str - '0';
		str++;
	}
	
	return n*sign;
}
#ifdef __KERNEL
//static volatile int_lock alloc_lock = 0;

static kernel_mutex alloc_lock = { -1 };
int liballoc_lock()
{
	kernel_lock_mutex(&alloc_lock);
	return 0;
}

int liballoc_unlock()
{
	kernel_unlock_mutex(&alloc_lock);
	return 0;
}
#else
int liballoc_lock()
{
	return 0;
}

int liballoc_unlock()
{
	return 0;
}
#endif

#ifdef __KERNEL
	#define MALLOC_FLAGS (PAGE_PRESENT | PAGE_RW)
#else
	#define MALLOC_FLAGS (PAGE_USER | PAGE_PRESENT | PAGE_RW)
#endif
	
void* liballoc_alloc(size_t n)
{
	return alloc_pages(NULL, n, MALLOC_FLAGS);
}

int liballoc_free(void* p, size_t n)
{
	return free_pages(p, n);
}
