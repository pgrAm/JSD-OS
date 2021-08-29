#include <stdio.h>

#include <kernel/memorymanager.h>
#include <kernel/multiboot.h>

#include "ramdisk.h"

extern multiboot_info* _multiboot;

struct ramdisk_drive
{
	uint8_t* base;
	size_t size;
};

ramdisk_drive init_disk = {NULL, 0};

disk_driver ramdisk_driver = {
	ramdisk_read_blocks
};

void ramdisk_read_blocks(const filesystem_drive* d, size_t offset, uint8_t* buf, size_t num_bytes)
{
	ramdisk_drive* rd = (ramdisk_drive*)d->dsk_impl_data;

	if(offset < rd->size)
	{
		if(offset + num_bytes > rd->size)
		{
			printf("read overrun on ramdisk\n");
		}

		memcpy(buf, rd->base + offset, num_bytes);
	}
}

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

void ramdisk_init()
{
	filesystem_add_drive(&ramdisk_driver, ramdisk_get_drive(0), 1);
}
