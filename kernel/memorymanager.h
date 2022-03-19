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

typedef uintptr_t page_flags_t;
int memmanager_free_pages(void* page, size_t num_pages);
void* memmanager_virtual_alloc(void* virtual_address, size_t n, page_flags_t flags);

SYSCALL_HANDLER int syscall_free_pages(void* page, size_t num_pages);
SYSCALL_HANDLER void* syscall_virtual_alloc(void* virtual_address, size_t n, page_flags_t flags);

int memmanager_unmap_pages(void* page, size_t num_pages);

void memmanager_set_page_flags(void* virtual_address, size_t num_pages, page_flags_t flags);
void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, page_flags_t flags);

uintptr_t memmanager_new_memory_space();
void memmanager_enter_memory_space(uintptr_t memspace);
bool memmanager_destroy_memory_space(uintptr_t memspace);

bool memmanager_handle_page_fault(page_flags_t err, uintptr_t page);

extern void set_page_directory(uintptr_t* address);
extern void enable_paging(void);
extern void disable_paging(void);
extern void* get_page_directory(void);

enum page_flags
{
    PAGE_PRESENT = 0x01,
    PAGE_RW = 0x02,
    PAGE_USER = 0x04,

    // OS specific

    PAGE_RESERVED = 0x800, // bit 11
    PAGE_MAP_ON_ACCESS = 0x400, // bit 10


    PAGE_ALLOCATED = PAGE_RESERVED | PAGE_PRESENT
};

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 1024

inline size_t memmanager_minimum_pages(size_t bytes)
{
    return (bytes + (PAGE_SIZE - 1)) / PAGE_SIZE;
}


#ifdef __cplusplus
}
#endif
#endif