#ifndef CMOS_H
#define CMOS_H

#include <time.h>
#include <drivers/portio.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int cmos_get_update_flag() 
{
	  outb(0x70, 0x0A);
	  return (inb(0x71) & 0x80);
}

inline uint8_t cmos_get_register(uint8_t reg) 
{
	  outb(0x70, reg);
	  return inb(0x71);
}

clock_t cmos_get_date_time(struct tm* result);

#ifdef __cplusplus
}
#endif
#endif