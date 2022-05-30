#ifndef _PHYSICAL_MANAGER_H
#define _PHYSICAL_MANAGER_H

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <kernel/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

void physical_memory_init(void);

SYSCALL_HANDLER size_t physical_num_bytes_free(void);
size_t physical_mem_size(void);

uintptr_t physical_memory_allocate_in_range(uintptr_t start, uintptr_t end, size_t size, size_t align);
uintptr_t physical_memory_allocate(size_t size, size_t align);

void physical_memory_free(uintptr_t physical_address, size_t size);

void physical_memory_reserve(uintptr_t address, size_t size);

#ifdef __cplusplus
}
#endif
#endif