#include <stdio.h>

#include <kernel/memorymanager.h>
#include <kernel/boot_info.h>
#include <kernel/filesystem/fs_driver.h>

#include "ramdisk.h"

typedef struct ramdisk_drive
{
	uint8_t* base;
	size_t size;
} ramdisk_drive;

static ramdisk_drive init_disk = {nullptr, 0};

static void ramdisk_read_blocks(void* d, size_t block_number, uint8_t* buf, size_t num_bytes);
static ramdisk_drive* ramdisk_get_drive(size_t index);

static disk_driver ramdisk_driver = {
	ramdisk_read_blocks,
	nullptr,
	nullptr,
	nullptr
};

static void ramdisk_read_blocks(void* drv_data, size_t offset, uint8_t* buf, size_t num_bytes)
{
	ramdisk_drive* rd = (ramdisk_drive*)drv_data;

	if(offset < rd->size)
	{
		if(offset + num_bytes > rd->size)
		{
			printf("read overrun on ramdisk\n");
		}

		memcpy(buf, rd->base + offset, num_bytes);
	}
}

static ramdisk_drive* ramdisk_get_drive(size_t index)
{
	if(init_disk.base == nullptr)
	{
		uintptr_t rd_begin = boot_information.ramdisk_location;
		uintptr_t rd_end = rd_begin + boot_information.ramdisk_size;

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
	ramdisk_drive* drive = ramdisk_get_drive(0);
	filesystem_add_drive(&ramdisk_driver, drive, 1, drive->size);
}
