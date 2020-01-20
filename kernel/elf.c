#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <filesystem.h>
#include <memorymanager.h>
#include <elf.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

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

bool elf_is_compatible(ELF_header32* file_header)
{
	if(file_header->machine_arch != ELF_ARCH_X86)
	{
		printf("Can't load, incompatible architecture\n");
		return false;
	}	
	else if (file_header->type != ELF_TYPE_EXEC && file_header->type != ELF_TYPE_DYN)
	{
		printf("Can't load, unsupported executable type\n");
		return false;
	}
	return true;
}

typedef struct span
{
	void* base;
	size_t size;
} span;

span elf_get_size(ELF_header32* file_header, file_stream* f)
{
	uintptr_t min_address = ~(uintptr_t)0;
	uintptr_t max_address = 0;

	for (size_t i = 0; i < file_header->pgh_entries; i++)
	{
		//seek ahead to the program header table
		filesystem_seek_file(f, file_header->pgh_offset + i * sizeof(ELF_program_header32));
		//read the program header
		ELF_program_header32 pg_header;
		filesystem_read_file(&pg_header, sizeof(ELF_program_header32), f);

		if (pg_header.type == ELF_PTYPE_LOAD)
		{
			min_address = MIN(min_address, pg_header.virtual_address);
			max_address = MAX(max_address, pg_header.virtual_address + pg_header.mem_size);
		}
	}

	span retval =  { (void*)min_address, max_address - min_address };

	return retval;
}

int load_elf(const char* path, dynamic_object* object)
{
	ELF_ident file_identifer;
	ELF_header32 file_header;
	
	file_stream* f = filesystem_open_file(NULL, path, 0);
	
	if(f == NULL)
	{
		printf("could not open elf file\n");
		return 0;
	}
	
	filesystem_read_file(&file_identifer, sizeof(ELF_ident), f);

	if(elf_is_readable(&file_identifer))
	{
		filesystem_read_file(&file_header, sizeof(ELF_header32), f);
		
		if(elf_is_compatible(&file_header))
		{
			span s = elf_get_size(&file_header, f);

			size_t object_size = s.size;
			size_t num_pages = (object_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
			void* base_adress = memmanager_virtual_alloc(s.base, num_pages, PAGE_USER | PAGE_PRESENT | PAGE_RW);

			object->num_segments = 1;
			object->segments = (segment*)malloc(sizeof(segment) * object->num_segments);
			object->segments[1].pointer = base_adress;
			object->segments[1].num_pages = num_pages;

			if (s.base != 0) //if the elf cares where its loaded then don't add the base adress
			{
				base_adress = 0;
			}

			for(int i = 0; i < file_header.pgh_entries; i++)
			{
				//seek ahead to the program header table
				filesystem_seek_file(f, file_header.pgh_offset + i * sizeof(ELF_program_header32));
				//read the program header
				ELF_program_header32 pg_header;
				filesystem_read_file(&pg_header, sizeof(ELF_program_header32), f);
				
				switch(pg_header.type)
				{
					case ELF_PTYPE_DYNAMIC:
						object->dynamic_section = base_adress + pg_header.virtual_address;
					break;
					case ELF_PTYPE_LOAD:
					{
						//size_t num_pages = (pg_header.mem_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
						
						void* virtual_address = base_adress + pg_header.virtual_address;
						
						//clear mem_size bytes at virtual_address to 0
						//memmanager_virtual_alloc(virtual_address, num_pages, PAGE_USER | PAGE_PRESENT | PAGE_RW);
						
						//copy file_size bytes from offset to virtual_address
						filesystem_seek_file(f, pg_header.offset);
						filesystem_read_file(virtual_address, pg_header.file_size, f);
					}
					break;
					default:
					break;
				}
			}

			object->entry_point = base_adress + file_header.entry_point;
		}
	}

	filesystem_close_file(f);
	
	return 1;
}