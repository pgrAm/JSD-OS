#ifndef STDLIB_H
#define STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

int atoi(const char* str);

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void* calloc(size_t, size_t);
void free(void* ptr);

#ifndef __KERNEL
void exit(int status);
#endif

#ifdef __cplusplus
}
#endif
#endif