#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int load_elf(const char* path, process* newTask)
{
	ELF_ident file_identifer;
	ELF_header32 file_header;
	
	//printf("elf file opening\n");
	
	file_stream* f = filesystem_open_file(NULL, path, 0);
	
	if(f == NULL)
	{
		printf("could not open elf file\n");
		return 0;
	}
	
	//printf("elf file opened\n");
	
	filesystem_read_file(&file_identifer, sizeof(ELF_ident), f);
	
	if(elf_is_readable(&file_identifer))
	{
		filesystem_read_file(&file_header, sizeof(ELF_header32), f);
		
		if(elf_is_compatible(&file_header))
		{
			newTask->num_segments = file_header.pgh_entries;
			newTask->segments = (segment*)malloc(sizeof(segment) * newTask->num_segments);
			
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
						size_t num_pages = (pg_header.mem_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
						
						newTask->segments[i].pointer = (void*)pg_header.virtual_address;
						newTask->segments[i].num_pages = num_pages;
						
						//printf("loading %d byte segment at %X\n", pg_header.mem_size, pg_header.virtual_address);
						
						//clear mem_size bytes at virtual_address to 0
						memmanager_virtual_alloc((void*)pg_header.virtual_address, num_pages, PAGE_USER | PAGE_PRESENT | PAGE_RW);
						
						//printf("segment allocated, used %d pages\n", num_pages);
						
						//copy file_size bytes from offset to virtual_address
						filesystem_seek_file(f, pg_header.offset);
						filesystem_read_file((void*)pg_header.virtual_address, pg_header.file_size, f);
					}
					break;
					default:
					break;
				}
			}
			
			if(file_header.type == ELF_TYPE_EXEC)
			{
				newTask->user_stack_top = memmanager_virtual_alloc(NULL, 1, PAGE_USER | PAGE_PRESENT | PAGE_RW);	
				newTask->entry_point = (void*)file_header.entry_point;
			}
			else
			{
				newTask->user_stack_top = NULL;
				newTask->entry_point = NULL;
			}
		}
	}

	filesystem_close_file(f);
	
	return 1;
}