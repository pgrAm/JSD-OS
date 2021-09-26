#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "portio.h"

#include <kernel/memorymanager.h>

#include "isa_dma.h" 

// lookup registers for each channel
static const uint8_t mask_register[8]	= {0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4};
static const uint8_t mode_register[8]	= {0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6};
static const uint8_t clear_register[8]	= {0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8};

// lookup ports for each channel
static const uint8_t page_port[8]	= {0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A};
static const uint8_t adress_port[8]	= {0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC};
static const uint8_t count_port[8]	= {0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE};

static bool check_continuity(uintptr_t v, size_t size)
{
	uintptr_t start = (uint32_t)v & ~(PAGE_SIZE - 1);
	uintptr_t end = (uint32_t)(v + size) & ~(PAGE_SIZE - 1);

	uintptr_t last_address = memmanager_get_physical(start);
	for(uintptr_t i = start + PAGE_SIZE; i < end; i += PAGE_SIZE)
	{
		uintptr_t physical = memmanager_get_physical(i);
		if(physical - last_address > PAGE_SIZE)
		{
			printf("Buffer must be physically contiguous\n");
			return false;
		}
		last_address = physical;
	}

	return true;
}

uint8_t* isa_dma_allocate_buffer(size_t size)
{
	size_t size_in_pages = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;

	for(uintptr_t address = 0; address < 0xFFFFFF; address += 0x10000)
	{
		uintptr_t physical = memmanager_allocate_physical_in_range(	address,
																	address + 0x10000,
																	size_in_pages * PAGE_SIZE,
																	PAGE_SIZE);
		if(physical != (uintptr_t)NULL)
		{
			return memmanager_map_to_new_pages(physical,
											   size_in_pages, PAGE_PRESENT | PAGE_RW);
		}
	}

	return NULL;
}

int isa_dma_free_buffer(uint8_t* buffer, size_t size)
{
	return memmanager_free_pages(buffer, (size + (PAGE_SIZE - 1)) / PAGE_SIZE);
}

void isa_dma_begin_transfer(uint8_t channel, uint8_t mode, uint8_t* buf, size_t size)
{
	//dma don't know wtf virtual adresses are, it needs the real deal (physical address)
	uint32_t physbuf = memmanager_get_physical((uint32_t)buf);

	if(!check_continuity((uintptr_t)buf, size))
	{
		printf("Buffer must be physically contiguous\n");
	}

	if((physbuf >> 16) != ((physbuf + size) >> 16))
	{
		printf("DMA Cannot cross 64k boundary\n");
		printf("%X - %X\n", physbuf, physbuf + size);
	}

	if(physbuf > 0xFFFFFF)
	{
		printf("Cannot use ISA dma above 16MB\n");
	}

	//printf("dma begin to physical address %X\n", (uint32_t)buf);

	size_t offset = physbuf & 0xFFFF;

	size--;

	outb(mask_register[channel],	0x04 | channel);
	outb(clear_register[channel],	0x00);
	outb(mode_register[channel],	mode + channel);
	outb(adress_port[channel],		offset & 0x00FF);
	outb(adress_port[channel],		(offset & 0xFF00) >> 8);
	outb(page_port[channel],		physbuf >> 16);
	outb(count_port[channel],		size & 0x00FF);
	outb(count_port[channel],		(size & 0xFF00) >> 8);
	outb(mask_register[channel],	channel);
}