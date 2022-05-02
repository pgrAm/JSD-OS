#ifndef STDDEF_H
#define STDDEF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define offsetof(st, m) __builtin_offsetof(st, m)

#ifndef NULL
#define NULL (void*)0
#endif

#define _HAVE_SIZE_T
typedef uint32_t size_t;

typedef size_t ptrdiff_t;

#ifdef __cplusplus
}
#endif
#endif