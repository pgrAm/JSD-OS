#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include "portio.h"

#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/kassert.h>

#include "isa_dma.h" 

// lookup registers for each channel
static constexpr uint8_t mask_register[8] = {0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4};
static constexpr uint8_t mode_register[8] = {0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6};
static constexpr uint8_t clear_register[8] = {0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8};

// lookup ports for each channel
static constexpr uint8_t page_port[8] = {0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A};
static constexpr uint8_t adress_port[8] = {0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC};
static constexpr uint8_t count_port[8] = {0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE};

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

struct dma_buffer {
	struct block {
		size_t offset;
		size_t size;
	};
	std::vector<block> free_blocks;
	std::vector<block> used_blocks;
	uint8_t* buffer;

	dma_buffer() : buffer(nullptr) {}

	dma_buffer(uint8_t* buf, size_t size) : buffer(buf)
	{
		free_blocks.push_back({0, size});
	}

	dma_buffer(uint8_t* buf, size_t size, size_t used) : buffer(buf)
	{
		free_blocks.push_back({used, (size - used)});
		used_blocks.push_back({0, used});
	}

	uint8_t* allocate(size_t size)
	{
		auto it = std::find_if(free_blocks.begin(), free_blocks.end(), 
							   [size](const block& other) { return other.size >= size; });

		if(it == free_blocks.end())
			return nullptr;

		size_t offset = it->offset;

		k_assert(it->size >= size);

		if(it->size == size)
		{
			free_blocks.erase(it);
		}
		else
		{
			it->size -= size;
			it->offset += size;
		}

		used_blocks.push_back({offset, size});

		return buffer + offset;
	}

	int deallocate(uint8_t* ptr)
	{
		auto offset = ptr - buffer;

		auto it = std::find_if(used_blocks.begin(), used_blocks.end(),
							   [offset](const block& other) { return other.offset == offset; });

		if(it == used_blocks.end())
			return -1;

		if(used_blocks.size() == 1)
			return 0;

		for(auto blk_it = free_blocks.begin(); blk_it != free_blocks.begin(); blk_it++)
		{
			if(blk_it->offset + blk_it->size < it->offset)
			{
				continue;
			}
			else if(blk_it->offset == it->offset + it->size)
			{
				blk_it->offset -= it->size;
				return 1;
			}
			else if(blk_it->offset + blk_it->size == it->offset)
			{
				blk_it->size += it->size;
				return 1;
			}
			else if(blk_it->offset + blk_it->size > it->offset)
			{
				free_blocks.insert(blk_it, *it);
				return 1;
			}
		}

		free_blocks.push_back(*it);
		return 1;
	}
};

std::vector<dma_buffer>* buffers;

uint8_t* isa_dma_allocate_buffer(size_t size)
{
	if(!buffers)
	{
		buffers = new std::vector<dma_buffer>;
	}

	for(auto&& buffer : *buffers)
	{
		if(uint8_t* ptr = buffer.allocate(size); ptr != nullptr)
		{
			return ptr;
		}
	}

	size_t size_in_pages = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
	size_t buffer_size = size_in_pages * PAGE_SIZE;

	for(uintptr_t address = 0; address < 0xFFFFFF; address += 0x10000)
	{
		uintptr_t physical = physical_memory_allocate_in_range(address,
															   address + 0x10000,
															   buffer_size,
															   PAGE_SIZE);
		if(physical != (uintptr_t)NULL)
		{
			auto virt = (uint8_t*)memmanager_map_to_new_pages(physical, size_in_pages,
															  PAGE_PRESENT | PAGE_RW);
			buffers->push_back(dma_buffer{virt, buffer_size, size});
			return virt;
		}
	}

	return nullptr;
}

int isa_dma_free_buffer(uint8_t* buf, size_t size)
{
	for(auto&& buffer : *buffers)
	{
		if(auto r = buffer.deallocate(buf); r != -1)
		{
			if(r == 0)
			{
				size_t num_pages = (size + (PAGE_SIZE - 1)) / PAGE_SIZE;
	
				return memmanager_free_pages(buffer.buffer, num_pages);
			}
			return 0;
		}
	}
	return -1;
}

void isa_dma_begin_transfer(uint8_t channel, uint8_t mode, const uint8_t* buf, size_t size)
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