#ifndef STDDEF_H
#define STDDEF_H

#include <stdint.h>

#define offsetof(st, m) ((size_t)(&((st *)0)->m))

#ifndef NULL
#define NULL (void*)0
#endif

#define _HAVE_SIZE_T
typedef uint32_t size_t;

#endif