#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdint.h>
#include <string.h>

void floppy_init();

void floppy_sendbyte(uint8_t byte);
uint8_t floppy_getbyte();
void floppy_reset(uint8_t driveNum);
void floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head);
void floppy_read_sectors(uint8_t driveNum, size_t lba, uint8_t* buf, size_t num_sectors);
#endif