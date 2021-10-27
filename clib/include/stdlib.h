#ifndef STDLIB_H
#define STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int atoi(const char* str);

extern void* malloc(size_t size);
extern void* realloc(void* ptr, size_t size);
extern void* calloc(size_t, size_t);
extern void free(void* ptr);

#ifndef __KERNEL
void exit(int status);
#endif

#ifdef __cplusplus
}
#endif
#endif