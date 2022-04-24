//must compile with -fPIC
#include <stdint.h>
#include <kernel/memorymanager.h>

alignas(4096) static constinit uintptr_t pd[PAGE_TABLE_SIZE];
alignas(4096) static constinit uintptr_t pt[PAGE_TABLE_SIZE];
alignas(4096) constinit uint8_t init_stack[PAGE_SIZE];

#define PAGE_MASK (PAGE_SIZE - 1)

extern "C" uintptr_t boot_remap_addresses(uintptr_t kernel_VM,
										  size_t kernel_size,
										  uintptr_t kernel_offset)
{
	//Mark pages not present
	memset(pd, 0, PAGE_SIZE);

	//the last entry in the page dir is mapped to the page dir
	pd[PAGE_TABLE_SIZE - 1] = (uintptr_t)pd | PAGE_PRESENT | PAGE_RW;

	//map the first 4 MiB of memory to itself
	for(size_t i = 0; i < PAGE_TABLE_SIZE; i++)
	{
		pt[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
	}
	//Add the first 4 MiB to the page dir
	pd[0] = (uintptr_t)pt | PAGE_PRESENT | PAGE_RW;

	auto pg_start = kernel_VM & ~PAGE_MASK;

	size_t num_k_pages = memmanager_minimum_pages(kernel_size);

	uintptr_t kernel_addr = kernel_offset & ~PAGE_MASK;

	for(size_t i = 0; i < num_k_pages; i++)
	{
		//uintptr_t* pd_entry = &pd[pg_start >> 22];

		pt[pg_start >> 12] = kernel_addr | PAGE_PRESENT | PAGE_RW;

		kernel_addr += PAGE_SIZE;
		pg_start += PAGE_SIZE;
	}

	return (uintptr_t)&pd[0];
}