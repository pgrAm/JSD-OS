#include <stdio.h>
#include <string.h>

#include "filesystem.h"
#include "memorymanager.h"

#include "elf.h"

int elf_is_readable(ELF_ident* file_identifer)
{
	static const char elf_magic[4] = {0x7f, 'E', 'L', 'F'};
	
	if(memcmp(file_identifer->magic, elf_magic, 4) != 0)
	{
		printf("File is not a valid executable\n");	
		return 0;
	}
	
	if(file_identifer->endianness != ELF_LITTLE_ENDIAN)
	{
		printf("Can't load big endian file\n");
		return 0;
	}
	
	if(file_identifer->bit_width != ELF_BIT_WIDTH_32)
	{
		if(file_identifer->bit_width == ELF_BIT_WIDTH_64)
		{
			printf("Can't load 64 bit file\n");
		}
		else
		{
			printf("Unknown format\n");	
		}
		
		return 0;
	}
	
	return 1;
}

int elf_is_compatible(ELF_header32* file_header)
{
	if(file_header->machine_arch != ELF_ARCH_X86)
	{
		printf("Cant load, incompatible architecture\n");
		return 0;
	}	
	return 1;
}

extern void run_user_code(void* address, void* stack);

int read_elf(file_handle* file)
{
	ELF_ident file_identifer;
	ELF_header32 file_header;
	
	file_stream* f = filesystem_open_handle(file, 0);
	
	//printf("elf file opened\n");
	
	filesystem_read_file(&file_identifer, sizeof(ELF_ident), f);
	
	if(elf_is_readable(&file_identifer))
	{
		filesystem_read_file(&file_header, sizeof(ELF_header32), f);
		
		if(elf_is_compatible(&file_header))
		{
			for(int i = 0; i < file_header.pgh_entries; i++)
			{
				//seek ahead to the program header table
				filesystem_seek_file(f, file_header.pgh_offset + i * sizeof(ELF_program_header32));
				
				ELF_program_header32 pg_header;
				filesystem_read_file(&pg_header, sizeof(ELF_program_header32), f);
				
				switch(pg_header.type)
				{
					case ELF_PTYPE_LOAD:
					{
						printf("loading segment at %X\n", pg_header.virtual_address);
						
						//clear mem_size bytes at virtual_address to 0
						size_t num_pages = (pg_header.mem_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
						memmanager_virtual_alloc(pg_header.virtual_address, num_pages, PAGE_USER | PAGE_PRESENT | PAGE_RW);
						
						//copy file_size bytes from offset to virtual_address
						filesystem_seek_file(f, pg_header.offset);
						filesystem_read_file((void*)pg_header.virtual_address, pg_header.file_size, f);
					}
					break;
					default:
					break;
				}
			}
			
			filesystem_close_file(f);
			
			if(file_header.type == ELF_TYPE_EXEC)
			{
				void* stack = memmanager_allocate_pages(1, PAGE_USER | PAGE_PRESENT | PAGE_RW);
				
				run_user_code((void*)file_header.entry_point, stack + PAGE_SIZE);
				
				//printf("Process returned %d\n", r);
			}
		}
	}

	filesystem_close_file(f);
	
	return -1;
}

int run_elf(file_handle* file)
{
	uint32_t p = memmanager_new_memory_space();
	
	int r = read_elf(file);
	
	memmanager_exit_memory_space(p);
	
	return r;
}