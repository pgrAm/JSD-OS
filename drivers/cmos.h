#ifndef CMOS_H
#define CMOS_H

#include "portio.h"

inline int cmos_get_update_flag() 
{
      outb(0x70, 0x0A);
      return (inb(0x71) & 0x80);
}

inline uint8_t cmos_get_register(int reg) 
{
      outb(0x70, reg);
      return inb(0x71);
}
#endif