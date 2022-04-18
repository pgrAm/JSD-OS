#ifndef NDEBUG

#include <kernel/kassert.h>
#include <stdio.h>

extern "C" [[noreturn]] void __kassert_fail(const char* expression, const char* file, int line)
{
	printf("%s %s %d", expression, file, line);
	__asm__ volatile ("cli;hlt");
	__builtin_unreachable();
}

#endif