#include "ramdisk.h"
#include <memorymanager.h>
#include <stdio.h>

struct ramdisk_drive
{
	uint8_t* base;
	size_t size;
};

ramdisk_drive init_disk = {NULL, 0};

void ramdisk_read_blocks(const filesystem_drive* d, size_t offset, uint8_t* buf, size_t num_bytes)
{
	ramdisk_drive* rd = (ramdisk_drive*)d->dsk_impl_data;

	if(offset < rd->size)
	{
		memcpy(buf, rd->base + offset, num_bytes);
	}
}

#include "multiboot.h"
extern multiboot_info* _multiboot;

ramdisk_drive* ramdisk_get_drive(size_t index)
{
	if(init_disk.base == NULL)
	{
		uintptr_t rd_begin = ((multiboot_modules*)_multiboot->m_modsAddr)[0].begin;
		uintptr_t rd_end = ((multiboot_modules*)_multiboot->m_modsAddr)[0].end;

		size_t rd_size = rd_end - rd_begin;

		uintptr_t rd_page_begin = rd_begin & ~(PAGE_SIZE - 1);
		uintptr_t rd_pages = (rd_end - rd_page_begin + PAGE_SIZE - 1) / PAGE_SIZE;

		size_t rd_offset = rd_begin - rd_page_begin;

		init_disk.base = rd_offset + (uint8_t*)memmanager_map_to_new_pages(rd_page_begin, rd_pages, PAGE_PRESENT | PAGE_RW);
		init_disk.size = rd_size;
	}

	return &init_disk;
}
