#include <string.h>
#include <stdio.h>

#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>

static inline void __flush_tlb()
{
	__asm__ volatile("mov %%cr3, %%eax\n"
					 "mov %%eax, %%cr3"
					 :
	:
		: "%eax", "memory");
}

static inline void __flush_tlb_page(uintptr_t addr)
{
#ifndef __I386_ONLY
	__asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
#else
	__flush_tlb();
#endif
}

#define PT_INDEX_MASK (PAGE_TABLE_SIZE - 1)
#define PAGE_ADDRESS_MASK (PAGE_SIZE - 1)

#define ALIGN_TO_PAGE(addr) (((addr) + PAGE_ADDRESS_MASK) & ~(PAGE_ADDRESS_MASK))
#define MAXIMUM_ADDRESS 0xFFFFFFFF

uintptr_t* const last_pde_address = (uintptr_t*)((uintptr_t)(PAGE_TABLE_SIZE - 1) * (uintptr_t)PAGE_TABLE_SIZE * (uintptr_t)PAGE_SIZE);
uintptr_t* const current_page_directory = (uintptr_t*)((uintptr_t)MAXIMUM_ADDRESS - (uintptr_t)PAGE_ADDRESS_MASK);

#define GET_PAGE_TABLE_ADDRESS(pd_index) ((uintptr_t*)(last_pde_address + (PAGE_TABLE_SIZE * pd_index)))

inline uintptr_t memmanager_allocate_physical_page()
{
	return physical_memory_allocate(PAGE_SIZE, PAGE_SIZE);
}

uintptr_t memmanager_get_pt_entry(uintptr_t virtual_address)
{
	uintptr_t pd_index = virtual_address >> 22;
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(!(pd_entry & PAGE_PRESENT)) //page table is not present
	{
		return pd_entry;
	}

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	return page_table[(virtual_address >> 12) & PT_INDEX_MASK];
}

uintptr_t memmanager_get_physical(uintptr_t virtual_address)
{
	return (memmanager_get_pt_entry(virtual_address) & ~PAGE_ADDRESS_MASK) + (virtual_address & PAGE_ADDRESS_MASK);
}

void memmanager_create_new_page_table(size_t pd_index)
{
	current_page_directory[pd_index] = memmanager_allocate_physical_page() | PAGE_USER | PAGE_PRESENT | PAGE_RW;

	__flush_tlb();

	//printf("added new page table, %X\n", current_page_directory[pd_index]);

	uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	memset(page_table, 0, PAGE_SIZE);
}

void* memmanager_get_unmapped_pages(const size_t num_pages, uint32_t flags)
{
	for(size_t pd_index = 0; pd_index < PAGE_TABLE_SIZE; pd_index++)
	{
		if(!(current_page_directory[pd_index] & PAGE_PRESENT))
		{
			memmanager_create_new_page_table(pd_index);

			if(num_pages < PAGE_TABLE_SIZE)
			{
				return (void*)(pd_index << 22);
			}
		}

		//if were looking for a user page, we need a user page table
		if((flags & PAGE_USER) && !(current_page_directory[pd_index] & PAGE_USER))
		{
			continue;
		}

		uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

		size_t pages_found = 0;
		size_t pt_index = 0;

		while(pt_index < PAGE_TABLE_SIZE)
		{
			pages_found = (page_table[pt_index++] & PAGE_PRESENT) ? 0 : pages_found + 1;

			if(pages_found == num_pages)
			{
				void* virtual_address = (void*)((pd_index << 22) + ((pt_index - pages_found) << 12));

				return virtual_address;
			}
		}
	}

	printf("couldn't find page\n");

	return NULL;
}

void memmanager_map_page(uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags)
{
	size_t pd_index = virtual_address >> 22;
	uintptr_t pt_entry = current_page_directory[pd_index];

	if(!(pt_entry & PAGE_PRESENT)) //page table is not present
	{
		memmanager_create_new_page_table(pd_index);
	}
	else if((flags & PAGE_USER) && !(pt_entry & PAGE_USER))
	{
		printf("Page at %X does not match requested flags\n", virtual_address);
		while(true);
		return;
	}

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	size_t pt_index = (virtual_address >> 12) & PT_INDEX_MASK;

	if(page_table[pt_index] & PAGE_PRESENT && physical_address != 0)
	{
		printf("warning page already exists at %X!\n", virtual_address);
		//while(true);
	}

	page_table[pt_index] = (physical_address & ~PAGE_ADDRESS_MASK) | flags;

	__flush_tlb_page(virtual_address);

	//printf("%X mapped to physical address %X\n", virtual_address, physical_address);	
}

void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, uint32_t flags)
{
	void* virtual_address = memmanager_get_unmapped_pages(n, flags);

	for(size_t i = 0; i < n; i++)
	{
		memmanager_map_page((uintptr_t)virtual_address + i * PAGE_SIZE, physical_address + i * PAGE_SIZE, flags);
	}

	return virtual_address;
}

uint32_t memmanager_get_page_flags(void* virtual_address)
{
	return memmanager_get_pt_entry((uintptr_t)virtual_address) & PAGE_ADDRESS_MASK;
}

void memmanager_set_page_flags(void* virtual_address, size_t num_pages, uint32_t flags)
{
	flags &= ~PAGE_PRESENT;

	for(size_t i = 0; i < num_pages; i++)
	{
		uintptr_t v_address = (uintptr_t)virtual_address + i * PAGE_SIZE;

		size_t pd_index = v_address >> 22;
		uintptr_t pt_entry = current_page_directory[pd_index];

		if(!(pt_entry & PAGE_PRESENT)) //page table is not present
		{
			puts("page table not present, while attempting to set flags");
			return;
		}

		size_t pt_index = (v_address >> 12) & PT_INDEX_MASK;

		uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
		page_table[pt_index] = (page_table[pt_index] & (~PAGE_ADDRESS_MASK | PAGE_PRESENT)) | flags;

		__flush_tlb_page(v_address);
	}
}

//reserve virtual adresses without allocating physical memory
void* memmanager_reserve_pages(void* virtual_address, size_t n, uint32_t flags)
{
	if(virtual_address == NULL)
	{
		virtual_address = memmanager_get_unmapped_pages(n, flags);

		//if its still null then there were not enough contiguous unmapped pages
		if(virtual_address == NULL)
		{
			printf("failure to get %d unmapped pages", n);
			return NULL;
		}
	}

	uintptr_t addr = (uintptr_t)virtual_address;
	for(size_t i = 0; i < n; ++i)
	{
		memmanager_map_page(addr, 0, PAGE_PRESENT);
		addr += PAGE_SIZE;
	}
	return virtual_address;
}

//reserve virtual adresses without allocating physical memory
void memmanager_unreserve_pages(void* virtual_address, size_t n)
{
	uintptr_t addr = (uintptr_t)virtual_address;
	for(size_t i = 0; i < n; ++i)
	{
		if(memmanager_get_physical(addr) == 0)
		{
			memmanager_map_page(addr, 0, 0);
		}
		addr += PAGE_SIZE;
	}
}

SYSCALL_HANDLER void* memmanager_virtual_alloc(void* virtual_address, size_t n, uint32_t flags)
{
	if(virtual_address == NULL)
	{
		virtual_address = memmanager_get_unmapped_pages(n, flags);

		//if its still null then there were not enough contiguous unmapped pages
		if(virtual_address == NULL)
		{
			printf("failure to get %d unmapped pages", n);
			return NULL;
		}
	}

	size_t num_pages = n;
	uintptr_t page_virtual_address = (uintptr_t)virtual_address;

	//printf("Attempting to allocate %d pages\n", num_pages);	

	size_t block = 0;

	while(num_pages)
	{
		uintptr_t physical_address = physical_memory_allocate_from(PAGE_SIZE, PAGE_SIZE, &block);

		memmanager_map_page(page_virtual_address, physical_address, flags);

		page_virtual_address += PAGE_SIZE;

		if(--num_pages == 0)
		{
			memset(virtual_address, 0, n * PAGE_SIZE);

			//printf("Successfully allocated %d pages at %X\n", n, virtual_address);		
			return virtual_address;
		}
	}

	printf("could not allocate enough pages");

	return NULL;
}

SYSCALL_HANDLER int memmanager_free_pages(void* page, size_t num_pages)
{
	uintptr_t virtual_address = (uintptr_t)page;

	while(num_pages--)
	{
		uintptr_t physical_address = memmanager_get_pt_entry(virtual_address);

		memmanager_map_page(virtual_address, 0, 0); //unmap the page

		virtual_address += PAGE_SIZE; //next page

		if(!(physical_address & PAGE_PRESENT))
		{
			return -1; //the page was not allocated
		}

		physical_memory_free(physical_address & ~PAGE_ADDRESS_MASK, PAGE_SIZE);
	}

	return 0;
}

void memmanager_init_page_dir(__attribute__((nonnull)) uintptr_t* page_dir, uintptr_t physaddr)
{
	//Mark pages not present
	memset(page_dir, 0, PAGE_SIZE);

	//the last entry in the page dir is mapped to the page dir
	page_dir[PAGE_TABLE_SIZE - 1] = physaddr | PAGE_PRESENT | PAGE_RW;
}

uintptr_t memmanager_new_memory_space()
{
	uintptr_t* process_page_dir = (uintptr_t*)memmanager_virtual_alloc(NULL, 1, PAGE_PRESENT | PAGE_RW);

	if(process_page_dir == NULL)
	{
		return (uintptr_t)NULL; //not enough free physical memory 
	}

	memmanager_init_page_dir(process_page_dir, memmanager_get_physical((uint32_t)process_page_dir));

	for(size_t i = 0; i < 32; i++)
	{
		process_page_dir[i] = current_page_directory[i] & ~PAGE_USER;
	}

	return (uintptr_t)process_page_dir;
}

void memmanager_enter_memory_space(uintptr_t memspace)
{
	set_page_directory((uintptr_t*)memmanager_get_physical((uintptr_t)memspace));
}

bool memmanager_destroy_memory_space(uintptr_t pdir)
{
	//set_page_directory(kernel_page_directory);
	memmanager_free_pages((void*)pdir, 1);

	return true;
}

void memmanager_print_all_mappings_to_physical_DEBUG(uintptr_t physical)
{
	for(size_t pd_index = 0; pd_index < PAGE_TABLE_SIZE; pd_index++)
	{
		if(current_page_directory[pd_index] & PAGE_PRESENT)
		{
			uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

			for(size_t pt_index = 0; pt_index < PAGE_TABLE_SIZE; pt_index++)
			{
				if(page_table[pt_index] & PAGE_PRESENT)
				{
					uintptr_t physical_address = page_table[pt_index] & ~PAGE_ADDRESS_MASK;
					if((physical & ~PAGE_ADDRESS_MASK) == physical_address)
					{
						uintptr_t v = (pd_index << 22) + pt_index * PAGE_SIZE;

						printf("%X mapped to %X\n", physical_address, v);
					}
				}
			}
		}
	}
}


void memmanager_init(void)
{
	//printf("Lo: %d\n", _multiboot->m_memoryLo);
	//printf("Hi: %d\n", _multiboot->m_memoryHi);

	

	//while(true);

	uintptr_t* kernel_page_directory = (uintptr_t*)memmanager_allocate_physical_page();
	uintptr_t* first_page_table = (uintptr_t*)memmanager_allocate_physical_page();

	if(first_page_table == NULL || kernel_page_directory == NULL)
	{
		//were completelty f'cked in this case
		puts("There's not enough ram to run the OS :(");
		//we're totally hosed so just give up
		while(true)
		{
			__asm__ volatile ("cli;hlt");
		}
		return;
	}

	memmanager_init_page_dir(kernel_page_directory, (uintptr_t)kernel_page_directory);

	//map the first 4 MiB of memory to itself
	for(size_t i = 0; i < PAGE_TABLE_SIZE; i++)
	{
		first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
	}

	//Add the first 4 MiB to the page dir
	kernel_page_directory[0] = ((uintptr_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

	set_page_directory(kernel_page_directory);

	enable_paging();

	printf("paging enabled\n");
}