#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct 
{
	//all adresses are physical
	uintptr_t kernel_location;
	size_t kernel_size;

	uintptr_t ramdisk_location;
	size_t ramdisk_size;

	uintptr_t memmap_location;
	size_t memmap_size;

	size_t high_memory;
	size_t low_memory;
} boot_info;

void parse_boot_info();

void reserve_boot_mem();

extern boot_info boot_information;

#ifdef __cplusplus
}
#endif
#endif
