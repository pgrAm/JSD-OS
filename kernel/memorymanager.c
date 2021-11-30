#include <string.h>
#include <stdio.h>

#include <kernel/boot_info.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/kassert.h>

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
#define PAGE_FLAGS_MASK (PAGE_SIZE - 1)
#define PAGE_ADDRESS_MASK (~PAGE_FLAGS_MASK)

#define MAXIMUM_ADDRESS (~(uintptr_t)0)

uintptr_t* const last_pde_address = (uintptr_t*)((uintptr_t)(PAGE_TABLE_SIZE - 1) * (uintptr_t)PAGE_TABLE_SIZE * (uintptr_t)PAGE_SIZE);
uintptr_t* const current_page_directory = (uintptr_t*)((uintptr_t)MAXIMUM_ADDRESS - (uintptr_t)~PAGE_ADDRESS_MASK);

#define GET_PAGE_TABLE_ADDRESS(pd_index) ((uintptr_t*)(last_pde_address + (PAGE_TABLE_SIZE * pd_index)))

inline uintptr_t memmanager_allocate_physical_page()
{
	return physical_memory_allocate(PAGE_SIZE, PAGE_SIZE);
}

inline size_t get_page_dir_index(uintptr_t virtual_address)
{
	return virtual_address >> 22;
}

inline size_t get_page_tbl_index(uintptr_t virtual_address)
{
	return (virtual_address >> 12) & PT_INDEX_MASK;
}

static uintptr_t memmanager_get_pt_entry(uintptr_t virtual_address)
{
	size_t pd_index = get_page_dir_index(virtual_address);
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(!(pd_entry & PAGE_PRESENT)) //page table is not present
	{
		return pd_entry;
	}

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	return page_table[get_page_tbl_index(virtual_address)];
}

uintptr_t memmanager_get_physical(uintptr_t virtual_address)
{
	return (memmanager_get_pt_entry(virtual_address) & PAGE_ADDRESS_MASK) +
		(virtual_address & ~PAGE_ADDRESS_MASK);
}

static void memmanager_create_new_page_table(size_t pd_index, page_flags_t flags)
{
	flags &=  ~(PAGE_RESERVED | PAGE_MAP_ON_ACCESS);

	current_page_directory[pd_index] = memmanager_allocate_physical_page() | PAGE_PRESENT | PAGE_RW | flags;

	__flush_tlb();

	//printf("added new page table, %X\n", current_page_directory[pd_index]);

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	memset(page_table, 0, PAGE_SIZE);
}

static void* memmanager_get_unmapped_pages(const size_t num_pages, page_flags_t flags)
{
	//size_t start = (flags & PAGE_USER) ? 0 : KERNEL_SPLIT / (PAGE_SIZE*PAGE_TABLE_SIZE);

	for(size_t pd_index = 0; pd_index < PAGE_TABLE_SIZE; pd_index++)
	{
		if(!(current_page_directory[pd_index] & PAGE_PRESENT))
		{
			memmanager_create_new_page_table(pd_index, flags);

			if(num_pages <= PAGE_TABLE_SIZE)
			{
				return (void*)(pd_index << 22);
			}
		}

		//if were looking for a user page, we need a user page table
		//if were looking for a kernel page, we need a kernel page table
		if((bool)(flags & PAGE_USER) != (bool)(current_page_directory[pd_index] & PAGE_USER))
		{
			continue;
		}

		uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

		size_t pages_found = 0;
		size_t pt_index = 0;

		while(pt_index < PAGE_TABLE_SIZE)
		{
			pages_found = (page_table[pt_index++] & PAGE_ALLOCATED) ? 0 : pages_found + 1;

			if(pages_found == num_pages)
			{
				return (void*)((pd_index << 22) + ((pt_index - num_pages) << 12));
			}
		}
	}

	printf("couldn't find page\n");
	k_assert(false);

	return NULL;
}

void memmanager_unmap_page(uintptr_t virtual_address)
{
	size_t pd_index = get_page_dir_index(virtual_address);
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(pd_entry & PAGE_PRESENT)
	{
		uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

		page_table[get_page_tbl_index(virtual_address)] = 0;

		__flush_tlb_page(virtual_address);
	}
	else
	{
		printf("warning pdir not exists for %X!\n", virtual_address);
		while(true);
	}
}

bool memmanager_map_page(uintptr_t virtual_address, uintptr_t physical_address, page_flags_t flags)
{
	size_t pd_index = get_page_dir_index(virtual_address);
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(!(pd_entry & PAGE_PRESENT)) //page table is not present
	{
		memmanager_create_new_page_table(pd_index, flags);
	}
	else if((flags & PAGE_USER) && !(pd_entry & PAGE_USER))
	{
		printf("Page at %X does not match requested flags %X\n", virtual_address, pd_entry & PAGE_FLAGS_MASK);
		return false;
	}

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	size_t pt_index = get_page_tbl_index(virtual_address) ;

	if(page_table[pt_index] & PAGE_ALLOCATED)
	{
		printf("warning page already exists at %X!\n", virtual_address);
		return false;
	}

	page_table[pt_index] = (physical_address & PAGE_ADDRESS_MASK) | flags;

	__flush_tlb_page(virtual_address);

	return true;
	//printf("%X mapped to physical address %X\n", virtual_address, physical_address);	
}

void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, page_flags_t flags)
{
	void* virtual_address = memmanager_get_unmapped_pages(n, flags);

	for(size_t i = 0; i < n; i++)
	{
		if(!memmanager_map_page((uintptr_t)virtual_address + i * PAGE_SIZE, physical_address + i * PAGE_SIZE, flags))
			return NULL;
	}

	return virtual_address;
}

uintptr_t memmanager_get_page_flags(uintptr_t virtual_address)
{
	return memmanager_get_pt_entry(virtual_address) & PAGE_FLAGS_MASK;
}

void memmanager_set_page_flags(void* virtual_address, size_t num_pages, page_flags_t flags)
{
	uintptr_t preserved = PAGE_PRESENT | PAGE_RESERVED | PAGE_MAP_ON_ACCESS;

	//flags &= ~preserved;

	for(size_t i = 0; i < num_pages; i++)
	{
		uintptr_t v_address = (uintptr_t)virtual_address + i * PAGE_SIZE;

		size_t pd_index = get_page_dir_index(v_address);
		uintptr_t pt_entry = current_page_directory[pd_index];

		if(!(pt_entry & PAGE_PRESENT)) //page table is not present
		{
			puts("page table not present, while attempting to set flags");
			return;
		}

		size_t pt_index = get_page_tbl_index(v_address);

		uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
		page_table[pt_index] = (page_table[pt_index] & (PAGE_ADDRESS_MASK | preserved)) | flags;

		__flush_tlb_page(v_address);
	}
}

static void* memmanager_alloc_page(page_flags_t flags)
{
	uintptr_t virtual_address = memmanager_get_unmapped_pages(1, flags);
	uintptr_t physical_address = memmanager_allocate_physical_page();

	if(!memmanager_map_page(virtual_address, physical_address, flags | PAGE_PRESENT))
		return NULL;

	return (void*)virtual_address;
}

SYSCALL_HANDLER void* memmanager_virtual_alloc(void* virtual_address, size_t n, page_flags_t flags)
{
	//printf("\tAllocating %d pages\n", n);

	uintptr_t illegal = PAGE_PRESENT | PAGE_RESERVED | PAGE_MAP_ON_ACCESS;

	flags &= (PAGE_FLAGS_MASK & ~illegal);

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
	else if((uintptr_t)virtual_address & PAGE_FLAGS_MASK)
	{
		printf("unaligned address %X", virtual_address);
		return NULL;
	}

	uintptr_t page_virtual_address = (uintptr_t)virtual_address;

	/*
	//if(!(flags & PAGE_RW))
	{
		size_t block = 0;

		size_t num_pages = n;
		while(num_pages)
		{
			uintptr_t physical_address = physical_memory_allocate_from(PAGE_SIZE, PAGE_SIZE, &block);

			memmanager_map_page(page_virtual_address, physical_address, flags | PAGE_PRESENT);

			page_virtual_address += PAGE_SIZE;

			if(--num_pages == 0)
			{
				memset(virtual_address, 0, n * PAGE_SIZE);

				//printf("Successfully allocated %d pages at %X\n", n, virtual_address);		
				return virtual_address;
			}
		}

		return NULL;
	}*/

	page_flags_t pf = flags | PAGE_RESERVED | PAGE_MAP_ON_ACCESS;
	for(size_t i = 0; i < n; i++)
	{
		k_assert(memmanager_map_page(page_virtual_address, 0, pf));

		k_assert(memmanager_get_page_flags(page_virtual_address) & PAGE_RESERVED);
		k_assert(!(memmanager_get_page_flags(page_virtual_address) & PAGE_PRESENT));

		page_virtual_address += PAGE_SIZE;
	}
	
	return virtual_address;
}

SYSCALL_HANDLER int memmanager_free_pages(void* page, size_t num_pages)
{
	//printf("\tFreeing %d pages\n", num_pages);

	uintptr_t virtual_address = (uintptr_t)page;

	while(num_pages--)
	{
		uintptr_t physical_address = memmanager_get_pt_entry(virtual_address);

		memmanager_unmap_page(virtual_address); //unmap the page

		virtual_address += PAGE_SIZE; //next page

		if(!(physical_address & PAGE_ALLOCATED))
		{
			return -1; //the page was not allocated
		}

		if(physical_address & PAGE_PRESENT)
		{
			physical_memory_free(physical_address & PAGE_ADDRESS_MASK, PAGE_SIZE);
		}
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
	uintptr_t* process_page_dir = (uintptr_t*)memmanager_alloc_page(PAGE_RW);

	if(process_page_dir == NULL)
	{
		return (uintptr_t)NULL; //not enough free physical memory 
	}

	memmanager_init_page_dir(process_page_dir, memmanager_get_physical((uintptr_t)process_page_dir));

	for(size_t i = 0; i < PAGE_TABLE_SIZE - 1; i++)
	{
		//copy only the kernel page directories
		if(!(current_page_directory[i] & PAGE_USER))
		{
			process_page_dir[i] = current_page_directory[i];
		}
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

bool memmanager_handle_page_fault(page_flags_t err, uintptr_t virtual_address)
{
	if(err & PAGE_PRESENT)
	{
		return false;
	}
	size_t pd_index = get_page_dir_index(virtual_address);

	if(current_page_directory[pd_index] & PAGE_PRESENT)
	{
		uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

		size_t pt_index = get_page_tbl_index(virtual_address);
		uintptr_t entry = page_table[pt_index];

		if(entry & PAGE_MAP_ON_ACCESS)
		{
			k_assert(entry & PAGE_RESERVED);
			k_assert(!(entry & PAGE_PRESENT));

			uintptr_t physical = memmanager_allocate_physical_page();
			if(!physical)
			{
				printf("Can't allocate physical page\n");
				return false;
			}

			virtual_address &= PAGE_ADDRESS_MASK;

			page_flags_t flags = entry & (PAGE_FLAGS_MASK & ~(PAGE_RESERVED | PAGE_MAP_ON_ACCESS));

			uintptr_t page_value = physical | flags | PAGE_PRESENT;

			page_table[pt_index] = page_value | PAGE_RW;
			__flush_tlb_page(virtual_address);

			//security measure
			memset((void*)virtual_address, 0, PAGE_SIZE);

			if(!(flags & PAGE_RW))
			{
				page_table[pt_index] = page_value;
				__flush_tlb_page(virtual_address);
			}

			return true;
		}
	}
	printf("Can't handle page fault\n");

	return false;
}

void memmanager_print_all_mappings_to_physical_DEBUG()
{
	for(size_t pd_index = 0; pd_index < PAGE_TABLE_SIZE; pd_index++)
	{
		if(current_page_directory[pd_index] & PAGE_PRESENT)
		{
			uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

			for(size_t pt_index = 0; pt_index < PAGE_TABLE_SIZE; pt_index++)
			{
				if(page_table[pt_index] & PAGE_PRESENT)
				{
					uintptr_t physical_address = page_table[pt_index] & PAGE_ADDRESS_MASK;
					//if((physical & PAGE_ADDRESS_MASK) == physical_address)
					{
						uintptr_t v = (pd_index << 22) + pt_index * PAGE_SIZE;
						if(v != physical_address)
						{
							printf("%X mapped to %X\n", v, physical_address);
						}
					}
				}
			}
		}
	}
}

extern uint8_t _IMAGE_END_;
extern uint8_t _KERNEL_START_;

static uintptr_t* allocate_low_page()
{
	return (uintptr_t*)
		physical_memory_allocate_in_range(0, 0x100000, PAGE_SIZE, PAGE_SIZE);
}

void memmanager_init(void)
{
	//printf("Lo: %d\n", _multiboot->m_memoryLo);
	//printf("Hi: %d\n", _multiboot->m_memoryHi);



	//while(true);

	uintptr_t* kernel_page_directory = allocate_low_page();
	uintptr_t* first_page_table = allocate_low_page();

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

	uintptr_t k_pg_start = (uintptr_t)&_KERNEL_START_ & PAGE_ADDRESS_MASK;
	uintptr_t k_pg_end = (uintptr_t)&_IMAGE_END_;
	size_t num_k_pages = memmanager_minimum_pages(k_pg_end - k_pg_start);

	uintptr_t kernel_addr = boot_information.kernel_location & PAGE_ADDRESS_MASK;

	uintptr_t* current_pt = first_page_table;

	for(size_t i = 0; i < num_k_pages; i++)
	{
		uintptr_t* pd_entry = &kernel_page_directory[get_page_dir_index(k_pg_start)];

		if(*pd_entry == NULL)
		{
			current_pt = (uintptr_t*)allocate_low_page();
			*pd_entry = (uintptr_t)current_pt | PAGE_PRESENT | PAGE_RW;
		}

		size_t pt_index = get_page_tbl_index(k_pg_start);

		current_pt[pt_index] = kernel_addr | PAGE_PRESENT | PAGE_RW;

		kernel_addr += PAGE_SIZE;
		k_pg_start += PAGE_SIZE;
	}

	//while(true);

	set_page_directory(kernel_page_directory);

	enable_paging();

	printf("paging enabled\n");
}