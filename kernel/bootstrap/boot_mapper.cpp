//must compile with -fPIC
#include <stdint.h>
#include <kernel/memorymanager.h>

alignas(4096) RECLAIMABLE_BSS static constinit uintptr_t pd[PAGE_TABLE_SIZE];
alignas(4096) RECLAIMABLE_BSS static constinit uintptr_t pt[PAGE_TABLE_SIZE];

RECLAIMABLE_BSS static uintptr_t pt_base = 0;

#define PAGE_MASK (PAGE_SIZE - 1)

extern "C" RECLAIMABLE void boot_mapper_remap_mem(uintptr_t virt, uintptr_t phys)
{
	pt[(virt >> 12) & (PAGE_TABLE_SIZE - 1)] = phys | PAGE_PRESENT | PAGE_RW;
}

extern "C" RECLAIMABLE uintptr_t boot_mapper_map_mem(uintptr_t addr,
													size_t bytes)
{
	auto aligned_addr = addr & ~PAGE_MASK;
	auto offset = addr & PAGE_MASK;

	size_t num_pages = memmanager_minimum_pages(offset + bytes);
	size_t pages_found = 0;

	for(size_t i = PAGE_TABLE_SIZE - 1; i > 0; --i)
	{
		if(pt[i] == 0)
		{
			if(pages_found == (num_pages - 1))
			{
				for(uintptr_t page = i;
					page < (i + num_pages); page++)
				{
					pt[page] = aligned_addr | PAGE_PRESENT | PAGE_RW;
					aligned_addr += PAGE_SIZE;
				}
				__asm__ volatile(
					"mov %%cr3, %%eax\n"
					"mov %%eax, %%cr3"
					:
					:
					: "%eax", "memory");
				return pt_base + offset + i * PAGE_SIZE;
			}
			++pages_found;
		}
		else
		{
			pages_found = 0;
		}
	}

	return 0;
}

extern "C" RECLAIMABLE uintptr_t boot_remap_addresses(uintptr_t kernel_VM,
													  size_t kernel_size,
													  uintptr_t kernel_offset)
{
	//the last entry in the page dir is mapped to the page dir
	pd[PAGE_TABLE_SIZE - 1] = (uintptr_t)pd | PAGE_PRESENT | PAGE_RW;

	pt_base = (kernel_VM >> 22) << 22;

	pd[kernel_VM >> 22] = (uintptr_t)pt | PAGE_PRESENT | PAGE_RW;

	auto pg_start = (kernel_VM >> 12) & (PAGE_TABLE_SIZE - 1);

	size_t num_k_pages = memmanager_minimum_pages(kernel_size);

	uintptr_t kernel_addr = kernel_offset & ~PAGE_MASK;

	for(size_t i = 0; i < num_k_pages; i++)
	{
		//uintptr_t* pd_entry = &pd[pg_start >> 22];

		pt[pg_start] = kernel_addr | PAGE_PRESENT | PAGE_RW;

		kernel_addr += PAGE_SIZE;
		pg_start++;
	}

	return (uintptr_t)&pd[0];
}