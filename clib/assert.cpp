#ifndef NDEBUG

#include <stdio.h>
#include <assert.h>

extern "C" [[noreturn]] void __assert_fail(const char* expression, const char* file, int line)
{
	printf("%s %s %d", expression, file, line);
#ifdef __KERNEL
	__asm__ volatile ("cli;hlt");
#else
	while(true)
	{
		__asm__ volatile("nop");
	}
#endif
	__builtin_unreachable();
}

#endif