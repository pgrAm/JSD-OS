#include <kernel/physical_manager.h>
#include <kernel/boot_info.h>
#include <stdio.h>

typedef struct
{
	uintptr_t offset;
	size_t length;
} memory_block;

#define MAX_NUM_MEMORY_BLOCKS 128
static size_t num_memory_blocks = 0;
static memory_block memory_map[MAX_NUM_MEMORY_BLOCKS] = {};

void physical_memory_add_block(size_t block_index, size_t address, size_t length)
{
	if(block_index >= MAX_NUM_MEMORY_BLOCKS)
	{
		puts("error maximum memory blocks reached");
		return;
	}
	if(block_index < num_memory_blocks)
	{
		memmove(&memory_map[block_index + 1], &memory_map[block_index], sizeof(memory_block) * (num_memory_blocks - block_index));
	}
	memory_map[block_index].offset = address;
	memory_map[block_index].length = length;
	num_memory_blocks++;
}

void physical_memory_remove_block(size_t block_index)
{
	if(block_index >= MAX_NUM_MEMORY_BLOCKS)
	{
		return;
	}
	if(block_index < (num_memory_blocks - 1))
	{
		memmove(&memory_map[block_index], &memory_map[block_index + 1], sizeof(memory_block) * ((num_memory_blocks - 1) - block_index));
	}
	num_memory_blocks--;
}

bool physical_claim_from_block(uintptr_t address, size_t size, size_t* block_index)
{
	size_t padding = address - memory_map[*block_index].offset;

	if(memory_map[*block_index].length >= size + padding)
	{
		//printf("%X padding\n", padding);

		if(padding > 0)
		{
			//add a new block to the list
			physical_memory_add_block(*block_index, memory_map[*block_index].offset, padding);
			(*block_index)++;
		}

		//printf("%X bytes at %X\n", memory_map[*block_index].length, memory_map[*block_index].offset);

		memory_map[*block_index].offset += size + padding;
		memory_map[*block_index].length -= size + padding;

		if(memory_map[*block_index].length == 0)
		{
			//remove the memory block from the list
			physical_memory_remove_block(*block_index);
			(*block_index)--;
		}

		return true;
	}

	return false;
}

void physical_memory_reserve(uintptr_t address, size_t size)
{
	//printf("%X bytes reserved at %X\n", size, address);

	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		if(memory_map[i].offset > address + size)
		{
			printf(">= %d bytes already reserved at %X\n", size, address);
			return;
		}

		if(memory_map[i].offset > address)
		{
			size -= memory_map[i].offset - address;
			address = memory_map[i].offset;
		}

		size_t claimed_space = size;
		size_t available_space = memory_map[i].length - (address - memory_map[i].offset);
		if(available_space < claimed_space)
		{
			claimed_space = available_space;
		}

		if(physical_claim_from_block(address, claimed_space, &i))
		{
			printf("%X bytes reserved at %X\n", claimed_space, address);

			address += claimed_space;
			size -= claimed_space;
		}

		if(size == 0)
			return;
	}
}

void physical_memory_free(uintptr_t physical_address, size_t size)
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		if(memory_map[i].offset + memory_map[i].length == physical_address) //we have an adjacent block
		{
			//physical_address = memory_map[i].offset;
			memory_map[i].length += size;
			break;
		}
		else if(memory_map[i].offset == physical_address + size) //we have an adjacent block
		{
			memory_map[i].offset = physical_address;
			memory_map[i].length += size;
			break;
		}
		else if(memory_map[i].offset + memory_map[i].length > physical_address)
		{
			//otherwise we need to add a new block
			physical_memory_add_block(i, physical_address, size);
			break;
		}
	}
}

uintptr_t physical_memory_allocate(size_t size, size_t align)
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		uintptr_t aligned_addr = align_addr(memory_map[i].offset, align);

		if(physical_claim_from_block(aligned_addr, size, &i))
		{
			return aligned_addr;
		}
	}

	printf("could not allocate enough pages\n");

	return 0;
}

uintptr_t physical_memory_allocate_from(size_t size, size_t align, size_t* block)
{
	for(int i = *block; i < num_memory_blocks; i++)
	{
		while(memory_map[i].length >= size) //contigous memory large enough
		{
			if(memory_map[i].offset % align != 0) //make sure memory is aligned to a page
			{
				uintptr_t aligned_addr = align_addr(memory_map[i].offset, align);
				size_t padding = aligned_addr - memory_map[i].offset;

				if(memory_map[i].length < size + padding)
				{
					break;
				}

				if(padding > 0)
				{
					//add a new block to the list
					physical_memory_add_block(i, memory_map[i].offset, padding);
					i++;
				}

				memory_map[i].offset = aligned_addr;
				memory_map[i].length -= padding;
			}

			uintptr_t physical_address = memory_map[i].offset;

			memory_map[i].offset += size;
			memory_map[i].length -= align;

			if(memory_map[i].length == 0)
			{
				//remove this block from the list
				physical_memory_remove_block(i);
				i--;
			}

			*block = i;
			return physical_address;
		}
	}

	return 0;
}

uintptr_t physical_memory_allocate_in_range(uintptr_t start, uintptr_t end, size_t size, size_t align)
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		uintptr_t aligned_addr = align_addr(memory_map[i].offset, align);

		if(aligned_addr < start)
		{
			if(memory_map[i].offset + memory_map[i].length < start)
			{
				continue;
			}
			aligned_addr = align_addr(start, align);
		}

		if(aligned_addr + size > end)
			return 0;

		if(physical_claim_from_block(aligned_addr, size, &i))
		{
			return aligned_addr;
		}
	}

	printf("Could not allocate enough physical memory\n");

	return 0;
}

SYSCALL_HANDLER size_t physical_num_bytes_free(void)
{
	size_t sum = 0;

	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		sum += memory_map[i].length;
	}

	return sum;
}

size_t total_mem_size = 0;

size_t physical_mem_size(void)
{
	return total_mem_size;
}

void print_free_map()
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		printf("Available \t%8X - %8X\n",
			   memory_map[i].offset,
			   memory_map[i].offset + memory_map[i].length);
	}
}

void physical_memory_init(void) 
{
	total_mem_size = boot_information.low_memory * 1024 + boot_information.high_memory * 1024;

	physical_memory_add_block(0, 0x500, (boot_information.low_memory * 1024) - 0x500);
	physical_memory_add_block(1, 0x00100000, boot_information.high_memory * 1024);

	//reserve kernel
	physical_memory_reserve(boot_information.kernel_location, boot_information.kernel_size);

	//reserve modules
	physical_memory_reserve(boot_information.ramdisk_location, boot_information.ramdisk_size);

	//reserve BIOS & VRAM
	physical_memory_reserve(0x80000, 0x100000 - 0x80000);
}