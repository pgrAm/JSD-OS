#include <string.h>
#include <stdio.h>

#include <kernel/boot_info.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/locks.h>
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

//this mutex must be locked when accessing/modfying kernel address space mappings
static constinit sync::mutex kernel_addr_mutex{};
static uintptr_t* kernel_page_directory;

extern "C" void memmanager_print_all_mappings_to_physical_DEBUG();

#define PT_INDEX_MASK (PAGE_TABLE_SIZE - 1)
#define PAGE_FLAGS_MASK (PAGE_SIZE - 1)
#define PAGE_ADDRESS_MASK (~PAGE_FLAGS_MASK)

#define MAXIMUM_ADDRESS (~(uintptr_t)0)

#define KERNEL_SPLIT (MAXIMUM_ADDRESS - MAXIMUM_ADDRESS/8)

uintptr_t* const last_pde_address = (uintptr_t*)((uintptr_t)(PAGE_TABLE_SIZE - 1) * (uintptr_t)PAGE_TABLE_SIZE * (uintptr_t)PAGE_SIZE);
uintptr_t* const current_page_directory = (uintptr_t*)((uintptr_t)MAXIMUM_ADDRESS - (uintptr_t)~PAGE_ADDRESS_MASK);

#define GET_PAGE_TABLE_ADDRESS(pd_index) ((uintptr_t*)(last_pde_address + (PAGE_TABLE_SIZE * pd_index)))

inline uintptr_t memmanager_allocate_physical_page()
{
	return physical_memory_allocate(PAGE_SIZE, PAGE_SIZE);
}

inline constexpr size_t get_page_dir_index(uintptr_t virtual_address)
{
	return virtual_address >> 22;
}

inline constexpr size_t get_page_tbl_index(uintptr_t virtual_address)
{
	return (virtual_address >> 12) & PT_INDEX_MASK;
}

static uintptr_t& memmanager_get_pt_entry(uintptr_t virtual_address, size_t pd_index)
{
	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	return page_table[get_page_tbl_index(virtual_address)];
}

static uintptr_t memmanager_get_pt_entry(uintptr_t virtual_address)
{
	size_t pd_index = get_page_dir_index(virtual_address);
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(!(pd_entry & PAGE_PRESENT)) //page table is not present
	{
		return pd_entry;
	}

	return memmanager_get_pt_entry(virtual_address, pd_index);
}

uintptr_t memmanager_get_physical(uintptr_t virtual_address)
{
	return (memmanager_get_pt_entry(virtual_address) & PAGE_ADDRESS_MASK) +
		(virtual_address & ~PAGE_ADDRESS_MASK);
}

void memmanager_update_pt(uintptr_t* pt_ptr, uintptr_t new_value, uintptr_t v_address)
{
	__atomic_store(pt_ptr, &new_value, __ATOMIC_RELAXED);
	__flush_tlb_page(v_address);
}

static void memmanager_create_new_page_table(size_t pd_index, page_flags_t flags)
{
	flags &= ~(PAGE_RESERVED | PAGE_MAP_ON_ACCESS);

	current_page_directory[pd_index] = memmanager_allocate_physical_page() | PAGE_PRESENT | PAGE_RW | flags;

	__flush_tlb();

	//printf("added new page table, %X\n", current_page_directory[pd_index]);

	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

	memset(page_table, 0, PAGE_SIZE);
}

static uintptr_t memmanager_get_unmapped_pages(const size_t num_pages, page_flags_t flags)
{
	const uintptr_t first_pt = (flags & PAGE_USER) ? 0 : get_page_dir_index(KERNEL_SPLIT);
	const uintptr_t last_pt = get_page_dir_index(MAXIMUM_ADDRESS);

	size_t pages_found = 0;

	for(size_t pd_index = first_pt; pd_index < last_pt; pd_index++)
	{
		if(!(current_page_directory[pd_index] & PAGE_PRESENT))
		{
			memmanager_create_new_page_table(pd_index, flags);

			if(num_pages <= PAGE_TABLE_SIZE)
			{
				return pd_index << 22;
			}
		}

		//if were looking for a user page, we need a user page table
		//if were looking for a kernel page, we need a kernel page table
		if((bool)(flags & PAGE_USER) != (bool)(current_page_directory[pd_index] & PAGE_USER))
		{
			pages_found = 0;
			continue;
		}

		const uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);

		for(size_t i = 0; i < PAGE_TABLE_SIZE; i++)
		{
			if(page_table[i] & PAGE_ALLOCATED)
			{
				pages_found = 0;
				continue;
			}

			pages_found++;

			if(pages_found == num_pages)
			{
				return (pd_index << 22) + (i - (num_pages - 1)) * PAGE_SIZE;
			}
		}
	}

	printf("couldn't find page\n");
	k_assert(false);

	return (uintptr_t)nullptr;
}

void memmanager_unmap_page_with_flags(uintptr_t virtual_address, page_flags_t flags)
{
	sync::lock_guard l{kernel_addr_mutex};

	size_t pd_index = get_page_dir_index(virtual_address);
	uintptr_t pd_entry = current_page_directory[pd_index];

	if(pd_entry & PAGE_PRESENT)
	{
		auto& pt_entry = memmanager_get_pt_entry(virtual_address, pd_index);
		if(pt_entry & flags)
		{
			//unmap the page
			memmanager_update_pt(&pt_entry, 0, virtual_address);
		}
	}
	else
	{
		printf("warning pdir not exists for %X!\n", virtual_address);
		k_assert(false);
	}
}

static bool memmanager_map_page(uintptr_t virtual_address, uintptr_t physical_address, page_flags_t flags)
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

	uintptr_t& pt_entry = memmanager_get_pt_entry(virtual_address, pd_index);

	if(pt_entry & PAGE_ALLOCATED)
	{
		printf("warning page already exists at %X!\n", virtual_address);
		return false;
	}

	memmanager_update_pt(&pt_entry,
						 (physical_address & PAGE_ADDRESS_MASK) | flags,
						 virtual_address);

	return true;
}

void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, page_flags_t flags)
{
	sync::lock_guard l{kernel_addr_mutex};

	uintptr_t virtual_address = memmanager_get_unmapped_pages(n, flags);

	for(size_t i = 0; i < n; i++)
	{
		if(!memmanager_map_page(virtual_address + i * PAGE_SIZE, physical_address + i * PAGE_SIZE, flags))
			return NULL;
	}

	return (void*)virtual_address;
}

uintptr_t memmanager_get_page_flags(uintptr_t virtual_address)
{
	return memmanager_get_pt_entry(virtual_address) & PAGE_FLAGS_MASK;
}

void memmanager_set_page_flags(void* virtual_address, size_t num_pages, page_flags_t flags)
{
	uintptr_t preserved = PAGE_PRESENT | PAGE_RESERVED | PAGE_MAP_ON_ACCESS;

	for(size_t i = 0; i < num_pages; i++)
	{
		const uintptr_t v_address = (uintptr_t)virtual_address + i * PAGE_SIZE;

		const size_t pd_index = get_page_dir_index(v_address);
		const uintptr_t pd_entry = current_page_directory[pd_index];

		if(!(pd_entry & PAGE_PRESENT)) //page table is not present
		{
			puts("page table not present, while attempting to set flags");
			return;
		}

		uintptr_t& page_entry = memmanager_get_pt_entry(v_address, pd_index);

		memmanager_update_pt(&page_entry,
							 (page_entry & (PAGE_ADDRESS_MASK | preserved)) | flags,
							 v_address);
	}
}

static void* memmanager_alloc_page(page_flags_t flags)
{
	uintptr_t physical_address = memmanager_allocate_physical_page();

	sync::lock_guard l{kernel_addr_mutex};

	uintptr_t virtual_address = memmanager_get_unmapped_pages(1, flags);

	if(!memmanager_map_page(virtual_address, physical_address, flags | PAGE_PRESENT))
		return NULL;

	return (void*)virtual_address;
}

void* memmanager_virtual_alloc(void* v_address, size_t n, page_flags_t flags)
{
	sync::lock_guard l{kernel_addr_mutex};

	uintptr_t illegal = PAGE_RESERVED | PAGE_MAP_ON_ACCESS;

	flags &= (PAGE_FLAGS_MASK & ~illegal);

	uintptr_t virtual_address = (uintptr_t)v_address;

	if(virtual_address == (uintptr_t)nullptr)
	{
		virtual_address = memmanager_get_unmapped_pages(n, flags);

		//if its still null then there were not enough contiguous unmapped pages
		if(virtual_address == (uintptr_t)nullptr)
		{
			printf("failure to get %d unmapped pages\n", n);
			return nullptr;
		}
	}
	else if((uintptr_t)virtual_address & PAGE_FLAGS_MASK)
	{
		printf("unaligned address %X\n", virtual_address);
		return nullptr;
	}

	uintptr_t page_virtual_address = (uintptr_t)virtual_address;

	if(flags & PAGE_PRESENT)
	{
		page_flags_t pf = flags | PAGE_PRESENT;
		for(size_t i = 0; i < n; i++)
		{
			auto r = memmanager_map_page(page_virtual_address,
										 memmanager_allocate_physical_page(), pf);
			k_assert(r);
		}
	}
	else
	{
		page_flags_t pf = flags | PAGE_RESERVED | PAGE_MAP_ON_ACCESS;
		for(size_t i = 0; i < n; i++)
		{
			auto r = memmanager_map_page(page_virtual_address, 0, pf);
			k_assert(r);

			k_assert(memmanager_get_page_flags(page_virtual_address) & PAGE_RESERVED);
			k_assert(!(memmanager_get_page_flags(page_virtual_address) & PAGE_PRESENT));
			page_virtual_address += PAGE_SIZE;
		}
	}

	return (void*)virtual_address;
}

SYSCALL_HANDLER void* syscall_virtual_alloc(void* v_address, size_t n, page_flags_t flags)
{
	return memmanager_virtual_alloc(v_address, n, flags);
}

static int memmanager_unmap_pages_with_flags(void* addr, size_t num_pages, page_flags_t flags)
{
	uintptr_t v_addr = (uintptr_t)addr;
	while(num_pages--)
	{
		memmanager_unmap_page_with_flags(v_addr, flags); //unmap the page
		v_addr += PAGE_SIZE; //next page
	}
	return 0;
}

SYSCALL_HANDLER int syscall_unmap_user_pages(void* addr, size_t num_pages)
{
	return memmanager_unmap_pages_with_flags(addr, num_pages, PAGE_USER);
}

int memmanager_unmap_pages(void* addr, size_t num_pages)
{
	return memmanager_unmap_pages_with_flags(addr, num_pages, PAGE_ALLOCATED);
}

int memmanager_free_pages_with_flags(void* page, size_t num_pages, page_flags_t flags)
{
	uintptr_t virtual_address = (uintptr_t)page;

	while(num_pages--)
	{
		uintptr_t physical_address = memmanager_get_pt_entry(virtual_address);

		memmanager_unmap_page_with_flags(virtual_address, flags); //unmap the page

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

int memmanager_free_pages(void* page, size_t num_pages)
{
	return memmanager_free_pages_with_flags(page, num_pages, PAGE_ALLOCATED);
}

SYSCALL_HANDLER int syscall_free_pages(void* page, size_t num_pages)
{
	if(!page) 
		return -1;

	return memmanager_free_pages_with_flags(page, num_pages, PAGE_USER);
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
	for(size_t i = 0; i < PAGE_TABLE_SIZE; i++)
	{
		//copy only the kernel page directories
		if(current_page_directory[i] & PAGE_USER)
		{
			physical_memory_free(current_page_directory[i] & PAGE_ADDRESS_MASK,
								 PAGE_SIZE);
		}
	}

	set_page_directory(kernel_page_directory);

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
		uintptr_t& pt_entry = memmanager_get_pt_entry(virtual_address, pd_index);

		if(pt_entry & PAGE_MAP_ON_ACCESS)
		{
			k_assert(pt_entry & PAGE_RESERVED);
			k_assert(!(pt_entry & PAGE_PRESENT));

			uintptr_t physical = memmanager_allocate_physical_page();
			if(!physical)
			{
				printf("Can't allocate physical page\n");
				return false;
			}

			virtual_address &= PAGE_ADDRESS_MASK;

			page_flags_t flags = pt_entry & (PAGE_FLAGS_MASK & ~(PAGE_RESERVED | PAGE_MAP_ON_ACCESS));

			uintptr_t page_value = physical | flags | PAGE_PRESENT;

			memmanager_update_pt(&pt_entry, page_value | PAGE_RW, virtual_address);

			//security measure
			memset((void*)virtual_address, 0, PAGE_SIZE);

			if(!(flags & PAGE_RW))
			{
				memmanager_update_pt(&pt_entry, page_value, virtual_address);
			}

			return true;
		}
	}

	//Can't handle page fault
	return false;
}

extern "C" void memmanager_print_all_mappings_to_physical_DEBUG()
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
	kernel_page_directory = (uintptr_t*)allocate_low_page();
	uintptr_t* first_page_table = (uintptr_t*)allocate_low_page();

	if(first_page_table == nullptr || kernel_page_directory == nullptr)
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

	memset(first_page_table, 0, PAGE_SIZE);

	first_page_table[0] = PAGE_RESERVED; //reserve page but don't map it

	//Add the first pt to the page dir
	kernel_page_directory[0] = ((uintptr_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

	uintptr_t k_pg_start = (uintptr_t)&_KERNEL_START_ & PAGE_ADDRESS_MASK;
	uintptr_t k_pg_end = (uintptr_t)&_IMAGE_END_;
	size_t num_k_pages = memmanager_minimum_pages(k_pg_end - k_pg_start);

	uintptr_t kernel_addr = boot_information.kernel_location & PAGE_ADDRESS_MASK;

	uintptr_t* current_pt = first_page_table;

	for(size_t i = 0; i < num_k_pages; i++)
	{
		uintptr_t* pd_entry = &kernel_page_directory[get_page_dir_index(k_pg_start)];

		if(*pd_entry == (uintptr_t)nullptr)
		{
			current_pt = (uintptr_t*)allocate_low_page();
			memset(current_pt, 0, PAGE_SIZE);

			*pd_entry = (uintptr_t)current_pt | PAGE_PRESENT | PAGE_RW;
		}

		size_t pt_index = get_page_tbl_index(k_pg_start);

		current_pt[pt_index] = kernel_addr | PAGE_PRESENT | PAGE_RW;

		kernel_addr += PAGE_SIZE;
		k_pg_start += PAGE_SIZE;
	}
	set_page_directory(kernel_page_directory);

	enable_paging();

	printf("paging enabled\n");
}