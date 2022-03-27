#ifndef K_ASSERT_H
#define K_ASSERT_H

#ifndef NDEBUG

#include <stdio.h>

#define k_assert(expression)     \
		if(!(expression)) {		\
		printf("%s %s %d\n", #expression, __FILE__, (unsigned)(__LINE__)); __asm__ volatile ("cli;hlt");}

#else

#define k_assert(expression)

#endif

#endif