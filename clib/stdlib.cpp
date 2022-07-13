#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef __KERNEL
#include <sys/syscalls.h>
#else
#include <kernel/locks.h>
#include <kernel/sys/syscalls.h>
#include <kernel/kassert.h>
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

static constinit sync::mutex alloc_lock{};
static int liballoc_lock()
{
	alloc_lock.lock();
	return 0;
}

static int liballoc_unlock()
{
	alloc_lock.unlock();
	return 0;
}
#else
static int liballoc_lock()
{
	return 0;
}

static int liballoc_unlock()
{
	return 0;
}

void exit(int status)
{
	sys_exit(status);
}

int system(const char* command)
{
	return 0;
}

#endif

#ifdef __KERNEL
	#define MALLOC_FLAGS (PAGE_RW)
#else
	#define MALLOC_FLAGS (PAGE_USER | PAGE_RW)
#endif
	
static void* liballoc_alloc(size_t n)
{
	return alloc_pages(NULL, n, MALLOC_FLAGS);
}

static int liballoc_free(void* p, size_t n)
{
	return free_pages(p, n);
}

static constinit heap_allocator library_allocator{
	liballoc_lock,
	liballoc_unlock,
	liballoc_alloc,
	liballoc_free,
};

constexpr size_t clib_default_align = 4;

void* aligned_alloc(size_t alignment, size_t size)
{
	return library_allocator.malloc_bytes(size, alignment);
}

void* malloc(size_t n)
{
	return library_allocator.malloc_bytes(n, clib_default_align);
}

void* realloc(void* ptr, size_t size)
{
	return library_allocator.realloc_bytes(ptr, size, clib_default_align);
}

void* calloc(size_t num, size_t size)
{
	return library_allocator.calloc_bytes(num, size, clib_default_align);
}

void free(void* p)
{
	return library_allocator.free_bytes(p);
}