#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdint.h>
#include <string.h>

#include <kernel/filesystem.h>

void floppy_init();

struct floppy_drive;
typedef struct floppy_drive floppy_drive;

void floppy_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);
uint8_t* floppy_allocate_buffer(size_t size);
int floppy_free_buffer(uint8_t* buffer, size_t size);

floppy_drive* floppy_get_drive(size_t index);
#endif