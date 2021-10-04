#ifndef _MEMORYMANAGER_H
#define _MEMORYMANAGER_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>

#include <kernel/syscall.h>

void memmanager_init(void);
uintptr_t memmanager_get_physical(uintptr_t virtual_address);

SYSCALL_HANDLER int memmanager_free_pages(void* page, size_t num_pages);
SYSCALL_HANDLER void* memmanager_virtual_alloc(void* virtual_address, size_t n, uint32_t flags);

uint32_t memmanager_get_page_flags(void* virtual_address);
void memmanager_set_page_flags(void* virtual_address, size_t num_pages, uint32_t flags);
void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, uint32_t flags);

uintptr_t memmanager_new_memory_space();
void memmanager_enter_memory_space(uintptr_t memspace);
bool memmanager_destroy_memory_space(uintptr_t memspace);

extern void set_page_directory(uintptr_t* address);
extern void enable_paging(void);
extern void disable_paging(void);
extern void* get_page_directory(void);

enum page_flags
{
    PAGE_PRESENT = 0x01,
    PAGE_RW = 0x02,
    PAGE_USER = 0x04
};

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 1024

SYSCALL_HANDLER size_t memmanager_num_bytes_free(void);
size_t memmanager_mem_size(void);

uintptr_t memmanager_allocate_physical_in_range(uintptr_t start, uintptr_t end, size_t size, size_t align);

#ifdef __cplusplus
}
#endif
#endif