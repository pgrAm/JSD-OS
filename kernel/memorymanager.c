#include <string.h>
#include <stdio.h>

#include "memorymanager.h"

typedef struct 
{
	uint32_t offset;
	uint32_t length;
} memory_block;

#define PT_INDEX_MASK (PAGE_TABLE_SIZE - 1)
#define PAGE_ADDRESS_MASK (PAGE_SIZE - 1)

#define ALIGN_TO_PAGE(addr) (((addr) + PAGE_ADDRESS_MASK) & ~(PAGE_ADDRESS_MASK))
#define MAXIMUM_ADDRESS 0xFFFFFFFF

uint32_t* const last_pde_address = (uint32_t*)((uint32_t)(PAGE_TABLE_SIZE - 1) * (uint32_t)PAGE_TABLE_SIZE * (uint32_t)PAGE_SIZE);
uint32_t* const current_page_directory = (uint32_t*)((uint32_t)MAXIMUM_ADDRESS - (uint32_t)PAGE_ADDRESS_MASK);

uint32_t* kernel_page_directory;

#define GET_PAGE_TABLE_ADDRESS(pd_index) ((uint32_t*)(last_pde_address + (PAGE_TABLE_SIZE * pd_index)))

size_t num_memory_blocks = 2;
memory_block memory_map[2] = {{0x00100000, 0}};//,{0x0000E000, 0x0007FFFF - 0x0000E000}};
//memory_block memory_map[1] = {{0x00100000, 0xFFFFFFFF - 0x00100000}};

uint32_t memmanager_get_pt_entry(uint32_t virtual_address)
{
	uint32_t pd_index = virtual_address >> 22;
	uint32_t pd_entry = current_page_directory[pd_index];
	
	if(!(pd_entry & PAGE_PRESENT)) //page table is not present
	{
		return pd_entry;
	}
	
	uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
	
	return page_table[(virtual_address >> 12) & PT_INDEX_MASK];
}

uint32_t memmanager_get_physical(uint32_t virtual_address)
{
	return (memmanager_get_pt_entry(virtual_address) & ~PAGE_ADDRESS_MASK) + (virtual_address & PAGE_ADDRESS_MASK);
}

uint32_t memmanager_allocate_physical_page()
{
	for(size_t i = 0; i < num_memory_blocks; i++)
	{
		uint32_t aligned_addr = ALIGN_TO_PAGE(memory_map[i].offset);
		uint32_t padding = aligned_addr - memory_map[i].offset;
	
		if(memory_map[i].length >= PAGE_SIZE + padding) //contigous memory large enough
		{	
			if(padding > 0)
			{
				//add a new block to the list
			}
		
			memory_map[i].offset += PAGE_SIZE + padding;
			memory_map[i].length -= PAGE_SIZE + padding;
			
			if(memory_map[i].length == 0)
			{
				//remove the memory block from the list
			}
			
			return aligned_addr;
		}
	}
	
	printf("could not allocate enough pages\n");
	
	return 0;
}

void memmanager_create_new_page_table(uint32_t pd_index)
{
	current_page_directory[pd_index] = memmanager_allocate_physical_page() | PAGE_USER | PAGE_PRESENT | PAGE_RW;
	
	printf("added new page table, %X\n", current_page_directory[pd_index]);
	
	uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
	
	memset(page_table, 0, PAGE_SIZE);
}

uint32_t memmanager_get_unmapped_pages(size_t num_pages, uint32_t flags)
{
	for(size_t pd_index = 0; pd_index < PAGE_TABLE_SIZE; pd_index++)
	{
		if(!(current_page_directory[pd_index] & PAGE_PRESENT))
		{
			memmanager_create_new_page_table(pd_index);	
			
			if(num_pages < PAGE_TABLE_SIZE)
			{
				return (pd_index << 22);
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
				uint32_t virtual_address = (pd_index << 22) + ((pt_index - pages_found) << 12);
				
				return virtual_address;
			}
		}
	}
	
	//printf("couldn't find page\n");
	
	return (uint32_t)NULL;
}

void memmanager_map_page(uint32_t virtual_address, uint32_t physical_address, uint32_t flags)
{
	uint32_t pd_index = virtual_address >> 22;
	uint32_t pt_entry = current_page_directory[pd_index];
	
	if(!(pt_entry & PAGE_PRESENT)) //page table is not present
	{
		memmanager_create_new_page_table(pd_index);	
	}
	
	uint32_t* page_table = GET_PAGE_TABLE_ADDRESS(pd_index);
	
	page_table[(virtual_address >> 12) & PT_INDEX_MASK] = (physical_address & ~PAGE_ADDRESS_MASK) | flags;
	
	printf("%X mapped to physical address %X\n", virtual_address, physical_address);	
}

void* memmanager_virtual_alloc(uint32_t virtual_address, size_t n, uint32_t flags)
{
	size_t num_pages = n;
	uint32_t page_virtual_address = virtual_address;
	
	printf("Attempting to allocate %d pages\n", num_pages);	
	
	while(num_pages)
	{
		for(size_t i = 0; i < num_memory_blocks; i++)
		{
			while(memory_map[i].length >= PAGE_SIZE) //contigous memory large enough
			{
				if(memory_map[i].offset % PAGE_SIZE != 0) //make sure memory is aligned to a page
				{
					uint32_t aligned_addr = ALIGN_TO_PAGE(memory_map[i].offset);
					uint32_t padding = aligned_addr - memory_map[i].offset;
					
					if(memory_map[i].length < PAGE_SIZE + padding)
					{
						break;
					}
					
					if(padding > 0)
					{
						//add a new block to the list
					}
					
					memory_map[i].offset = aligned_addr;
					memory_map[i].length -= padding;
				}
				
				uint32_t physical_address = memory_map[i].offset;
					
				memory_map[i].offset += PAGE_SIZE;
				memory_map[i].length -= PAGE_SIZE;

				if(memory_map[i].length == 0)
				{
					//remove the memory block from the list
				}
				
				memmanager_map_page(page_virtual_address, physical_address, flags);
				
				page_virtual_address += PAGE_SIZE;
				
				if(num_pages-- == 0)
				{
					memset((uint32_t*)virtual_address, 0, n*PAGE_SIZE);
					
					printf("Successfully allocated %d pages at %X\n", n, virtual_address);		
					return (void*)virtual_address;
				}
			}
		}
	}
	
	//printf("could not allocate enough pages");
	
	return NULL;
}

void* memmanager_allocate_pages(size_t num_pages, uint32_t flags)
{
	uint32_t virtual_address = memmanager_get_unmapped_pages(num_pages, flags);
	
	return memmanager_virtual_alloc(virtual_address, num_pages, flags);
}

int memmanager_free_pages(void* page, size_t num_pages)
{
	while(num_pages--)
	{
		uint32_t virtual_address = (uint32_t)page;
		uint32_t physical_address = memmanager_get_pt_entry(virtual_address);
		
		memmanager_map_page(virtual_address, 0, 0); //unmap the page
		
		page += PAGE_SIZE; //next page
		
		if(!(physical_address & PAGE_PRESENT))
		{
			return -1; //the page was not allocated
		}
		
		physical_address &= ~PAGE_ADDRESS_MASK;
		
		for(size_t i = 0; i < num_memory_blocks; i++)
		{
			if(memory_map[i].offset + memory_map[i].length == physical_address) //we have an adjacent block
			{
				physical_address = memory_map[i].offset;
				memory_map[i].length += PAGE_SIZE;
				
				break;
			}
			else if(memory_map[i].offset == physical_address + PAGE_SIZE) //we have an adjacent block
			{
				memory_map[i].offset = physical_address;
				memory_map[i].length += PAGE_SIZE;
				
				break;
			}
		}
	}
	
	return 0;
}

void memmanager_init_page_dir(uint32_t* page_dir, uint32_t physaddr)
{
	for(size_t i = 0; i < PAGE_TABLE_SIZE - 1; i++)
	{
		//Mark page not present
		page_dir[i] = 0;
	}
	
	//the last entry in the page dir is mapped to the page dir
	page_dir[PAGE_TABLE_SIZE - 1] = physaddr | PAGE_PRESENT | PAGE_RW;
}

uint32_t memmanager_new_memory_space()
{
	//were gonna need a new malloc type allocator too
	
	uint32_t* process_page_dir = memmanager_allocate_pages(1, PAGE_PRESENT | PAGE_RW);
	
	memmanager_init_page_dir(process_page_dir, memmanager_get_physical((uint32_t)process_page_dir));
	
	printf("new dir initialized\n");
	
	//process_page_dir[0] = current_page_directory[0] & ~PAGE_USER;
	
	for(size_t i = 0; i < PAGE_TABLE_SIZE - 1; i++)
	{
		process_page_dir[i] = current_page_directory[i] & ~PAGE_USER;
	}
	
	printf("new table initialized\n");
	
	set_page_directory((uint32_t*)memmanager_get_physical((uint32_t)process_page_dir));
	
	printf("page dir set\n");
	
	return (uint32_t)process_page_dir;
}

uint32_t memmanager_exit_memory_space(uint32_t pdir)
{
	set_page_directory(kernel_page_directory);
	memmanager_free_pages((void*)pdir, 1);
	
	return 1;
}

int load_exe(file_handle* file)
{
	file_stream* f = filesystem_open_handle(file);
	
	size_t num_code_pages = (file->size + (PAGE_SIZE - 1)) / PAGE_SIZE;
	
	uint8_t* code_seg = memmanager_allocate_pages(num_code_pages, PAGE_PRESENT | PAGE_RW);
	
	filesystem_read_file(code_seg, file->size, f);
	filesystem_close_file(f);
	
	uint32_t code_seg_physical = memmanager_get_physical((uint32_t)code_seg);
	
	uint32_t* process_page_dir = memmanager_allocate_pages(1, PAGE_PRESENT | PAGE_RW);
	uint32_t* old_page_dir = (uint32_t*)memmanager_get_physical((uint32_t)current_page_directory);
	
	memmanager_init_page_dir(process_page_dir, memmanager_get_physical((uint32_t)process_page_dir));
	
	//printf("page dir setup\n");
	
	//copy PT0 from the kernel's page dir into the process page dir
	process_page_dir[0] = current_page_directory[0] & ~PAGE_USER;
	
	//printf("page dir mapped\n");
	
	set_page_directory((uint32_t*)memmanager_get_physical((uint32_t)process_page_dir));
	
	//printf("Inside process page dir\n");
	
	memmanager_map_page(0x50000, code_seg_physical, PAGE_USER | PAGE_PRESENT | PAGE_RW);
	
	//printf("Code Seg mapped\n");
	
	memmanager_map_page(0xB8000, 0xB8000, PAGE_USER | PAGE_PRESENT | PAGE_RW); //map the video memory
	
	int (*p)(void) = (void*)0x50000;
	
	int r = p();
	
	printf("Process returned %d\n", r);
	
	set_page_directory(old_page_dir);
	
	memmanager_free_pages(code_seg, 1);
	
	return r;
}

#include "multiboot.h"
extern struct multiboot_info* _multiboot;

void memmanager_init(void)
{
	memory_map[0].length = _multiboot->m_memoryHi * 1024;
	
	kernel_page_directory = (uint32_t*)memmanager_allocate_physical_page();
	uint32_t* first_page_table = (uint32_t*)memmanager_allocate_physical_page();
	
	memmanager_init_page_dir(kernel_page_directory, (uint32_t)kernel_page_directory);

	//map the first 4 MiB of memory to itself
	for(size_t i = 0; i < PAGE_TABLE_SIZE; i++)
	{
		first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
	}
	
	//Add the first 4 MiB to the page dir
	kernel_page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;
	
	set_page_directory(kernel_page_directory);
	
	enable_paging();
	
	printf("paging enabled\n");
}