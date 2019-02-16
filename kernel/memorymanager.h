#ifndef _MEMORYMANAGER_H
#define _MEMORYMANAGER_H

#include "string.h"

void memmanager_init(void);
void* memmanager_allocate_pages(size_t num_pages, uint32_t flags);
int memmanager_free_pages(void* page, size_t num_pages);
void* memmanager_virtual_alloc(uint32_t virtual_address, size_t n, uint32_t flags);
uint32_t memmanager_get_physical(uint32_t virtual_address);

extern void set_page_directory(uint32_t* address);
extern void enable_paging(void);
extern void disable_paging();
extern void* get_page_directory(void);

#include "filesystem.h"

int load_exe(file_handle* file_name);

#define PAGE_PRESENT 0x01
#define PAGE_RW 0x02
#define PAGE_USER 0x04

#endif