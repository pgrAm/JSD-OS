#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>
#include <string.h>
#include "filesystem.h"

struct ramdisk_drive;
typedef struct ramdisk_drive ramdisk_drive;

void ramdisk_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);

ramdisk_drive* ramdisk_get_drive(size_t index);
#endif