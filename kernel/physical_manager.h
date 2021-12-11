#ifndef _PHYSICAL_MANAGER_H
#define _PHYSICAL_MANAGER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>

#include <kernel/syscall.h>

void physical_memory_init(void);

SYSCALL_HANDLER size_t physical_num_bytes_free(void);
size_t physical_mem_size(void);

uintptr_t physical_memory_allocate_in_range(uintptr_t start, uintptr_t end, size_t size, size_t align);
uintptr_t physical_memory_allocate(size_t size, size_t align);
uintptr_t physical_memory_allocate_from(size_t size, size_t align, size_t* block);

void physical_memory_free(uintptr_t physical_address, size_t size);

void physical_memory_reserve(uintptr_t address, size_t size);

//align must be a power of 2
static inline uintptr_t align_addr(uintptr_t addr, size_t align)
{
	return (addr + (align - 1)) & ~(align - 1);
}

#ifdef __cplusplus
}
#endif
#endif