#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

void* operator new(size_t size, std::align_val_t align)
{
	return aligned_alloc(static_cast<size_t>(align), size);
}

void* operator new[](size_t size, std::align_val_t align)
{
	return aligned_alloc(static_cast<size_t>(align), size);
}

void* operator new (size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* p)
{
	free(p);
}

void operator delete[](void* p)
{
	free(p);
}