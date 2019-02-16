#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

int atoi(const char* str);

extern void* malloc(size_t size);
extern void* realloc(void* ptr, size_t size);
extern void* calloc(size_t, size_t);
extern void free(void* ptr);

#endif