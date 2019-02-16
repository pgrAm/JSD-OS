#include <stdint.h>
#include "portio.h"
#include "rs232.h"

void rs232_init(unsigned int port, unsigned int rate)
{
	uint32_t divisor = (uint32_t)(115200 / rate);

	outb(port + 1, 0x00);    // Disable all interrupts
	outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	
	outb(port + 0, divisor & 0xFF);    // Set divisor to 3 (lo byte) 38400 baud
	outb(port + 1, divisor >> 8);    //                  (hi byte)
	
	outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
	
	outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int rs232_data_available(unsigned int port) 
{
   return inb(port + 5) & 1;
}

char rs232_read_byte(unsigned int port) 
{
   while(rs232_data_available(port) == 0);
   return inb(port);
}

int rs232_isclear(unsigned int port) 
{
	return inb(port + 5) & 0x20;
}
 
void rs232_send_byte(unsigned int port, char b) 
{
	while(rs232_isclear(port) == 0); //wait until line is clear
	outb(port, b);
}

void rs232_send_string(unsigned int port, const char* c)
{
	while(c++ != 0) { rs232_send_byte(port, *c); }
}

void rs232_send_string_len(unsigned int port, const char* b, size_t len) 
{
	size_t i;
	for(i = 0; i < len; i++)
	{
		rs232_send_byte(port, b[i]);
	}
}