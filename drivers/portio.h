#ifndef PORTIO_H
#define PORTIO_H

#include <stdint.h>

uint8_t inb(uint16_t);
void outb(uint16_t, uint8_t);

#endif