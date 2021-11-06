#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct 
{
	uintptr_t ramdisk_location;
	size_t ramdisk_size;

	size_t high_memory;
	size_t low_memory;
} boot_info;

void parse_boot_info();

extern boot_info boot_information;

#ifdef __cplusplus
}
#endif
#endif
