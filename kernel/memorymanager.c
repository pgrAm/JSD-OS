#include <string.h>
#include <stdio.h>

#include "memorymanager.h"

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

typedef struct 
{
	uintptr_t offset;
	size_t length;
} memory_block;

#define PT_INDEX_MASK (PAGE_TABLE_SIZE - 1)
#define PAGE_ADDRESS_MASK (PAGE_SIZE - 1)

#define ALIGN_TO_PAGE(addr) (((addr) + PAGE_ADDRESS_MASK) & ~(PAGE_ADDRESS_MASK))
#define MAXIMUM_ADDRESS 0xFFFFFFFF

uintptr_t* const last_pde_address = (uintptr_t*)((uintptr_t)(PAGE_TABLE_SIZE - 1) * (uintptr_t)PAGE_TABLE_SIZE * (uintptr_t)PAGE_SIZE);
uintptr_t* const current_page_directory = (uintptr_t*)((uintptr_t)MAXIMUM_ADDRESS - (uintptr_t)PAGE_ADDRESS_MASK);

#define GET_PAGE_TABLE_ADDRESS(pd_index) ((uintptr_t*)(last_pde_address + (PAGE_TABLE_SIZE * pd_index)))

extern void _IMAGE_END_;

#define MAX_NUM_MEMORY_BLOCKS 128
size_t num_memory_blocks = 2;
memory_block memory_map[MAX_NUM_MEMORY_BLOCKS] = {{(uintptr_t)&_IMAGE_END_, 0}, {0x00100000, 0}};

void memmanager_add_memory_block(size_t block_index, size_t address, size_t length)
{
	if (block_index >= MAX_NUM_MEMORY_BLOCKS)
	{
		puts("error maximum memory blocks reached");
		return;
	}
	if (block_index < num_memory_blocks)
	{
		memmove(&memory_map[block_index + 1], &memory_map[block_index], sizeof(memory_block) * (num_memory_blocks - block_index));
	}
	memory_map[block_index].offset = address;
	memory_map[block_index].length = length;
	num_memory_blocks++;
}

void memmanager_remove_memory_block(size_t block_index)
{
	if (block_index >= MAX_NUM_MEMORY_BLOCKS) 
	{
		return;
	}
	if (block_index < (num_memory_blocks - 1))
	{
		memmove(&memory_map[block_index], &memory_map[block_index + 1], sizeof(memory_block) * ((num_memory_blocks - 1) - block_index));
	}
	num_memory_blocks--;
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

uintptr_t memmanager_allocate_physical_page()
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		uintptr_t aligned_addr = ALIGN_TO_PAGE(memory_map[i].offset);
		size_t padding = aligned_addr - memory_map[i].offset;
	
		if(memory_map[i].length >= PAGE_SIZE + padding) //contigous memory large enough
		{	
			if(padding > 0)
			{
				//add a new block to the list
				memmanager_add_memory_block(i, memory_map[i].offset, padding);
				i++;
			}
		
			memory_map[i].offset += PAGE_SIZE + padding;
			memory_map[i].length -= PAGE_SIZE + padding;
			
			if(memory_map[i].length == 0)
			{
				//remove the memory block from the list
				memmanager_remove_memory_block(i);
				i--;
			}

			return aligned_addr;
		}
	}
	
	printf("could not allocate enough pages\n");
	
	return 0;
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
		printf("page allocation error\n");
		return;
	}
	
	uintptr_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
	
	size_t pt_index = (virtual_address >> 12) & PT_INDEX_MASK;
	
	if(page_table[pt_index] & PAGE_PRESENT && physical_address != 0)
	{
		printf("warning page already exists!\n");
	}
	
	page_table[pt_index] = (physical_address & ~PAGE_ADDRESS_MASK) | flags;
	
	__flush_tlb_page(virtual_address);
	
	//printf("%X mapped to physical address %X\n", virtual_address, physical_address);	
}

void* memmanager_map_to_new_pages(uintptr_t physical_address, size_t n, uint32_t flags)
{
	void* virtual_address = memmanager_get_unmapped_pages(n, flags);

	for (size_t i = 0; i < n; i++)
	{
		memmanager_map_page((uintptr_t)virtual_address + i * PAGE_SIZE, physical_address + i * PAGE_SIZE, flags);
	}

	return virtual_address;
}

void memmanager_set_page_flags(void* virtual_address, size_t num_pages, uint32_t flags)
{
	flags &= ~PAGE_PRESENT;

	for(size_t i = 0; i < num_pages; i++)
	{
		uintptr_t v_address = virtual_address + i * PAGE_SIZE;

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

SYSCALL_HANDLER void* memmanager_virtual_alloc(void* virtual_address, size_t n, uint32_t flags)
{
	if(virtual_address == NULL)
	{
		virtual_address = memmanager_get_unmapped_pages(n, flags);

		//if its still null then there were not enough contiguous unmapped pages
		if (virtual_address == NULL)
		{
			printf("failure to get %d unmapped pages", n);
			while (true);
			return NULL;
		}
	}
	
	size_t num_pages = n;
	uintptr_t page_virtual_address = (uintptr_t)virtual_address;
	
	//printf("Attempting to allocate %d pages\n", num_pages);	
	
	while(num_pages)
	{
		for(size_t i = 0; i < num_memory_blocks; i++)
		{
			while(memory_map[i].length >= PAGE_SIZE) //contigous memory large enough
			{
				if(memory_map[i].offset % PAGE_SIZE != 0) //make sure memory is aligned to a page
				{
					uintptr_t aligned_addr = ALIGN_TO_PAGE(memory_map[i].offset);
					size_t padding = aligned_addr - memory_map[i].offset;
					
					if(memory_map[i].length < PAGE_SIZE + padding)
					{
						break;
					}
					
					if(padding > 0)
					{
						//add a new block to the list
						memmanager_add_memory_block(i, memory_map[i].offset, padding);
						i++;
					}
					
					memory_map[i].offset = aligned_addr;
					memory_map[i].length -= padding;
				}
				
				uintptr_t physical_address = memory_map[i].offset;
					
				memory_map[i].offset += PAGE_SIZE;
				memory_map[i].length -= PAGE_SIZE;

				if(memory_map[i].length == 0)
				{
					//remove this block from the list
					memmanager_remove_memory_block(i);
					i--;
				}
				
				memmanager_map_page(page_virtual_address, physical_address, flags);
				
				page_virtual_address += PAGE_SIZE;
				
				if(--num_pages == 0)
				{
					memset(virtual_address, 0, n * PAGE_SIZE);
					
					//printf("Successfully allocated %d pages at %X\n", n, virtual_address);		
					return virtual_address;
				}
			}
		}
	}
	
	printf("could not allocate enough pages");
	
	return NULL;
}

int memmanager_free_pages(void* page, size_t num_pages)
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
		
		physical_address &= ~PAGE_ADDRESS_MASK;
		
		for(size_t i = 0; i < num_memory_blocks; i++)
		{
			if(memory_map[i].offset + memory_map[i].length == physical_address) //we have an adjacent block
			{
				//physical_address = memory_map[i].offset;
				memory_map[i].length += PAGE_SIZE;
				break;
			}
			else if(memory_map[i].offset == physical_address + PAGE_SIZE) //we have an adjacent block
			{
				memory_map[i].offset = physical_address;
				memory_map[i].length += PAGE_SIZE;
				break;
			}
			else if (memory_map[i].offset + memory_map[i].length > physical_address)
			{
				//otherwise we need to add a new block
				memmanager_add_memory_block(i, physical_address, PAGE_SIZE);
				break;
			}
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
	uintptr_t* process_page_dir = (uintptr_t*)memmanager_virtual_alloc(NULL, 1, PAGE_PRESENT | PAGE_RW);
	
	if (process_page_dir == NULL)
	{
		return (uintptr_t)NULL; //not enough free physical memory 
	}

	memmanager_init_page_dir(process_page_dir, memmanager_get_physical((uint32_t)process_page_dir));
	
	for(size_t i = 0; i < 2; i++)
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

#include "multiboot.h"
extern struct multiboot_info* _multiboot;

size_t total_mem_size = 0;

size_t memmanager_num_bytes_free(void)
{
	size_t sum = 0;
	
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		sum += memory_map[i].length;
	}
	
	return sum;
}

size_t memmanager_mem_size(void)
{
	return total_mem_size;
}

void memmanager_init(void)
{
	total_mem_size = _multiboot->m_memoryLo * 1024 + _multiboot->m_memoryHi * 1024;
	
	memory_map[0].length = (_multiboot->m_memoryLo * 1024) - (uintptr_t)&_IMAGE_END_;
	memory_map[1].length = _multiboot->m_memoryHi * 1024;
	
	uintptr_t* kernel_page_directory = (uintptr_t*)memmanager_allocate_physical_page();
	uintptr_t* first_page_table = (uintptr_t*)memmanager_allocate_physical_page();

	if (first_page_table == NULL || kernel_page_directory == NULL)
	{
		//were completelty f'cked in this case
		puts("There's not enough ram to run the OS :(");
		//we're totally hosed so just give up
		while (true) 
		{
			__asm__ volatile ("cli;hlt");
		}
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