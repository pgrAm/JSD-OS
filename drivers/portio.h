#ifndef PORTIO_H
#define PORTIO_H

#include <stddef.h>
#include <stdint.h>

static inline uint8_t inb(uint16_t port)
{
	uint8_t retval;
	__asm__ volatile ("inb %1, %0"
					  : "=a"(retval)
					  : "Nd"(port));
	return retval;
}

static inline void outb(uint16_t port, uint8_t val)
{
	__asm__ volatile ("outb %0, %1" 
					  :
					  : "a"(val), "Nd"(port));
}

static inline void outsw(uint16_t port, uint8_t* data, size_t size)
{
	__asm__ volatile ("rep outsw" 
					  : "+S" (data), "+c" (size) 
					  : "d" (port));
}

static inline void insw(uint16_t port, uint8_t* data, size_t size)
{
	__asm__ volatile ("rep insw" 
					  : "+D" (data), "+c" (size) 
					  : "d" (port) 
					  : "memory");
}

static inline void outsd(uint16_t port, uint8_t* data, size_t size)
{
	__asm__ volatile ("rep outsl"
					  : "+S" (data), "+c" (size)
					  : "d" (port));
}

static inline void insd(uint16_t port, uint8_t* data, size_t size)
{
	__asm__ volatile ("rep insl"
					  : "+D" (data), "+c" (size)
					  : "d" (port)
					  : "memory");
}


#endif