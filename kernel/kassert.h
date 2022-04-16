#ifndef K_ASSERT_H
#define K_ASSERT_H

#ifndef NDEBUG

#include <stdio.h>

inline void __kassert_fail(const char* expression, const char* file, unsigned int line)
{
	printf("%s %s %d\n", expression, file, line); 
	__asm__ volatile ("cli;hlt");
}

#define k_assert(expression)     \
		if(!(expression)) {		\
		__kassert_fail(#expression, __FILE__, (unsigned)(__LINE__));}

#else

#define k_assert(expression)

#endif

#endif