#ifndef STDLIB_H
#define STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int atoi(const char* str);

void* aligned_alloc(size_t alignment, size_t size);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void* calloc(size_t, size_t);
void free(void* ptr);

#ifndef __KERNEL
void exit(int status);

int system(const char* command);
#endif


inline int abs(int n)
{
	return n > 0 ? n : -n;
}

#ifdef __cplusplus
}
#endif
#endif