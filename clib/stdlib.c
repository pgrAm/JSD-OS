#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include "../kernel/memorymanager.h"

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

int liballoc_lock()
{
	//__asm__ ("cli");
	
	return 0;
}

int liballoc_unlock()
{
	//__asm__ ("sti");
	
	return 0;
}

void* liballoc_alloc(size_t n)
{
	return memmanager_allocate_pages(n, PAGE_PRESENT | PAGE_RW);
}

int liballoc_free(void* p, size_t n)
{
	return memmanager_free_pages(p, n);
}
