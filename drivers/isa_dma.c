#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "portio.h"
#include "isa_dma.h" 

#include "../kernel/memorymanager.h" 

// lookup registers for each channel
uint8_t mask_register[8]	= {0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4};
uint8_t mode_register[8]	= {0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6};
uint8_t clear_register[8]	= {0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8};

// lookup ports for each channel
uint8_t page_port[8]	= {0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A};
uint8_t adress_port[8]	= {0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC};
uint8_t count_port[8]	= {0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE};

void isa_dma_begin_transfer(uint8_t channel, uint8_t mode, uint8_t* buf, size_t length)
{
	//printf("dma begin to virtual address %X\n", (uint32_t)buf);
	
	//dma don't know wtf virtual adresses are, it needs the real deal (physical address)
	uint32_t physbuf = memmanager_get_physical((uint32_t)buf);
	
	if((physbuf >> 16) != ((physbuf + length) >> 16))
	{
		printf("DMA Cannot cross 64k boundary\n");
	}
	
	if(physbuf > 0xFFFFFF)
	{
		printf("Cannot use ISA dma above 16MB\n");
	}
	
	//printf("dma begin to physical address %X\n", (uint32_t)buf);
	
	size_t offset = physbuf & 0xFFFF;
	
	length--;
	
	outb(mask_register[channel],	0x04 | channel);
    outb(clear_register[channel],	0x00);
    outb(mode_register[channel],	mode + channel);
    outb(adress_port[channel],		offset & 0x00FF);
    outb(adress_port[channel],		(offset & 0xFF00) >> 8);
    outb(page_port[channel],		physbuf >> 16);
    outb(count_port[channel],		length & 0x00FF);
    outb(count_port[channel],		(length & 0xFF00) >> 8);
    outb(mask_register[channel],	channel);
}