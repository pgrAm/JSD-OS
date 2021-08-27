#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdint.h>
#include <string.h>
#include "filesystem.h"

void floppy_init();


struct floppy_drive;
typedef struct floppy_drive floppy_drive;

void floppy_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);

floppy_drive* floppy_get_drive(size_t index);
#endif