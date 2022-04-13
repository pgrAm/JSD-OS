#include <kernel/physical_manager.h>
#include <kernel/boot_info.h>
#include <kernel/kassert.h>
#include <utility>
#include <algorithm>
#include <stdio.h>

typedef struct
{
	uintptr_t offset;
	size_t length;
} memory_block;

#define MAX_NUM_MEMORY_BLOCKS 128

template<class Input, class Output>
Output* mem_copy(Input* first, Input* last, Output* d_first)
requires(std::is_trivially_copyable_v<Input>)
{
	return (Output*)memcpy(d_first, first, (last - first)*sizeof(Input));
	//return std::copy(first, last, d_first);
}

struct block_list
{
	using iterator = memory_block*;
	using const_iterator = const memory_block*;

	constexpr iterator begin() noexcept
	{
		return &blocks[0];
	}

	constexpr iterator end() noexcept
	{
		return begin() + size();
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return &blocks[0];
	}

	constexpr const_iterator cend() const noexcept
	{
		return cbegin() + size();
	}

	constexpr void erase(iterator it) noexcept
	{
		k_assert((it - cbegin()) < MAX_NUM_MEMORY_BLOCKS);

		if(it < (end() - 1))
		{
			mem_copy(it + 1, end(), it);
		}
		--num_blocks;
	}

	template <typename... Args>
	constexpr void emplace_back(Args&&... args) noexcept
	{
		*end() = memory_block{std::forward<Args>(args)...};
		++num_blocks;
	}

	template <typename... Args>
	constexpr iterator emplace(iterator it, Args&&... args) noexcept
	{
		k_assert(it < (cbegin() + MAX_NUM_MEMORY_BLOCKS));

		if(it <= end())
		{
			std::copy_backward(it, end(), end() + 1);
		}
		*it = memory_block{std::forward<Args>(args)...};
		++num_blocks;
		return it;
	}

	constexpr void claim_from_block(iterator& it, uintptr_t offset, size_t size) noexcept
	{
		auto length = it->length - (size + offset);
		if(length == 0)
		{
			if(offset > 0)
			{
				it->length = offset;
			}
			else
			{
				//remove the memory block from the list
				erase(it--);
			}
		}
		else
		{
			if(offset > 0)
			{
				//add a new block to the list
				emplace(it, it->offset, offset);
				++it;
			}
			*it = {it->offset + (size + offset), length};
		}
	}

	memory_block& operator[](size_t i) noexcept
	{
		return blocks[i];
	}

	const memory_block& operator[](size_t i) const noexcept
	{
		return blocks[i];
	}

	size_t size() const noexcept
	{
		return num_blocks;
	}

private:
	size_t num_blocks = 0;
	memory_block blocks[MAX_NUM_MEMORY_BLOCKS] = {};
};

constinit block_list memory_map{};

void physical_memory_reserve(uintptr_t address, size_t size)
{
	for(auto it = memory_map.begin(); it != memory_map.end(); it++)
	{
		if(it->offset > address + size)
		{
			printf(">= %d bytes already reserved at %X\n", size, address);
			return;
		}

		if(it->offset > address)
		{
			size -= it->offset - address;
			address = it->offset;
		}

		size_t claimed_space = size;
		size_t available_space = it->length - (address - it->offset);
		if(available_space < claimed_space)
		{
			claimed_space = available_space;
		}

		size_t padding = address - it->offset;
		if(it->length >= claimed_space + padding)
		{
			memory_map.claim_from_block(it, padding, claimed_space);

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
	for(auto it = memory_map.begin(); it != memory_map.end(); it++)
	{
		if(it->offset + it->length < physical_address)
		{
			continue;
		}
		else if(it->offset + it->length == physical_address) //we have an adjacent block
		{
			//we have an adjacent block before
			it->length += size;

			auto next = it + 1;

			if(next < memory_map.end() && next->offset == physical_address + size)
			{
				//we have an adjacent block after too
				//collapse that block into this one
				it->length += next->length;
				memory_map.erase(next);
			}
			return;
		}
		else if(it->offset == physical_address + size)
		{
			//we have an adjacent block after
			it->offset = physical_address;
			it->length += size;
			return;
		}
		else if(it->offset + it->length > physical_address)
		{
			//otherwise we need to add a new block
			memory_map.emplace(it, physical_address, size);
			return;
		}
	}

	memory_map.emplace_back(physical_address, size);
}

uintptr_t physical_memory_allocate(size_t size, size_t align)
{
	for(auto it = memory_map.begin(); it != memory_map.end(); it++)
	{
		uintptr_t aligned_addr = align_addr(it->offset, align);

		size_t padding = aligned_addr - it->offset;
		if(it->length >= size + padding)
		{
			memory_map.claim_from_block(it, padding, size);
			return aligned_addr;
		}
	}

	printf("could not allocate enough pages\n");
	return 0;
}

uintptr_t physical_memory_allocate_in_range(uintptr_t start, uintptr_t end, size_t size, size_t align)
{
	for(auto it = memory_map.begin(); it != memory_map.end(); it++)
	{
		uintptr_t aligned_addr = align_addr(it->offset, align);

		if(aligned_addr < start)
		{
			if(it->offset + it->length < start)
			{
				continue;
			}
			aligned_addr = align_addr(start, align);
		}

		if(aligned_addr + size > end)
			return 0;

		size_t padding = aligned_addr - it->offset;
		if(it->length >= size + padding)
		{
			memory_map.claim_from_block(it, padding, size);
			return aligned_addr;
		}
	}

	printf("Could not allocate enough physical memory\n");
	return 0;
}

SYSCALL_HANDLER size_t physical_num_bytes_free(void)
{
	size_t sum = 0;

	for(size_t i = 0; i < memory_map.size(); i++)
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
	for(size_t i = 0; i < memory_map.size(); i++)
	{
		printf("Available \t%8X - %8X\n",
			   memory_map[i].offset,
			   memory_map[i].offset + memory_map[i].length);
	}
}

void physical_memory_init(void)
{
	total_mem_size = boot_information.low_memory * 1024 + boot_information.high_memory * 1024;

	k_assert(memory_map.size() == 0);

	memory_map.emplace_back(0x500u, (boot_information.low_memory * 1024) - 0x500);
	memory_map.emplace_back(0x00100000u, boot_information.high_memory * 1024);

	//reserve kernel
	physical_memory_reserve(boot_information.kernel_location, boot_information.kernel_size);

	//reserve modules
	physical_memory_reserve(boot_information.ramdisk_location, boot_information.ramdisk_size);

	//reserve BIOS & VRAM
	physical_memory_reserve(0x80000, 0x100000 - 0x80000);
}