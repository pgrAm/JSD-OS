//must compile with -fPIC
#include <stdint.h>
#include <kernel/memorymanager.h>

alignas(4096) static uintptr_t pd[PAGE_TABLE_SIZE];
alignas(4096) static uintptr_t pt[PAGE_TABLE_SIZE];

#define PAGE_MASK (PAGE_SIZE - 1)

extern "C" uintptr_t boot_remap_addresses(uintptr_t kernel_VM,
										  size_t kernel_size,
										  uintptr_t kernel_offset)
{
	//kernel_offset = 0x9000;

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

	for(size_t i = 0; i < 16; i++)
	{
		//uintptr_t* pd_entry = &pd[pg_start >> 22];

		pt[pg_start >> 12] = kernel_addr | PAGE_PRESENT | PAGE_RW;

		kernel_addr += PAGE_SIZE;
		pg_start += PAGE_SIZE;
	}

	/*auto pg = kernel_VM;// ((uintptr_t)&_KERNEL_START_)& PAGE_MASK;

	pt[(pg >> 12) + 0x0] = kernel_offset + 0x0000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x1] = kernel_offset + 0x1000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x2] = kernel_offset + 0x2000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x3] = kernel_offset + 0x3000 | PAGE_PRESENT | PAGE_RW;

	pt[(pg >> 12) + 0x4] = kernel_offset + 0x4000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x5] = kernel_offset + 0x5000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x6] = kernel_offset + 0x6000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x7] = kernel_offset + 0x7000 | PAGE_PRESENT | PAGE_RW;

	pt[(pg >> 12) + 0x8] = kernel_offset + 0x8000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0x9] = kernel_offset + 0x9000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0xA] = kernel_offset + 0xA000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0xB] = kernel_offset + 0xB000 | PAGE_PRESENT | PAGE_RW;

	pt[(pg >> 12) + 0xC] = kernel_offset + 0xC000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0xD] = kernel_offset + 0xD000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0xE] = kernel_offset + 0xE000 | PAGE_PRESENT | PAGE_RW;
	pt[(pg >> 12) + 0xF] = kernel_offset + 0xF000 | PAGE_PRESENT | PAGE_RW;*/

	return (uintptr_t)&pd[0];
}