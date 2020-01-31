#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <filesystem.h>
#include <memorymanager.h>
#include <elf.h>

#include <util/hash.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

typedef struct ELF_linker_data
{
	ELF_dyn32* dynamic_section;
	void* base_address;

	void (*init_func)(void);
	void (**array_init_funcs)(void);
	size_t num_array_init_funcs;

	uint32_t* hash_table;
	ELF_sym32* symbol_table;
	size_t symbol_table_size;
	char* string_table;
	size_t string_table_size;

	ELF_rel32* relocation_addr;
	size_t relocation_entries;

	hash_map* lib_set;
	hash_map* symbol_map;
	hash_map* glob_data_symbol_map;
	bool userspace;
} ELF_linker_data;


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

int elf_read_symbols(ELF_linker_data* object);
void elf_process_relocation_section(ELF_linker_data* object, ELF_rel32* table, size_t rel_entries);
int elf_process_dynamic_section(ELF_linker_data* object);

int load_elf(const char* path, dynamic_object* object, bool user)
{
	ELF_ident file_identifer;
	ELF_header32 file_header;
	
	file_stream* f = filesystem_open_file(NULL, path, 0);
	
	if(f == NULL)
	{
		printf("could not open elf file %s\n", path);
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
			object->linker_data = NULL;

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
						object->linker_data = malloc(sizeof(ELF_linker_data));
						memset(object->linker_data, 0, sizeof(ELF_linker_data));
						((ELF_linker_data*)object->linker_data)->base_address = base_adress;
						((ELF_linker_data*)object->linker_data)->dynamic_section = base_adress + pg_header.virtual_address;
						((ELF_linker_data*)object->linker_data)->symbol_map = object->symbol_map;
						((ELF_linker_data*)object->linker_data)->glob_data_symbol_map = object->glob_data_symbol_map;
						((ELF_linker_data*)object->linker_data)->lib_set = object->lib_set;
						((ELF_linker_data*)object->linker_data)->userspace = user;
					break;
					case ELF_PTYPE_LOAD:
					{
						size_t num_pages = (pg_header.mem_size + (PAGE_SIZE - 1)) / PAGE_SIZE;
						
						void* virtual_address = base_adress + pg_header.virtual_address;
						
						uint32_t flags = 0;
						if(pg_header.flags & PF_WRITE) { flags |= PAGE_RW; }
						if(user) { flags |= PAGE_USER; }

						memmanager_set_page_flags(virtual_address, num_pages, flags);

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

			elf_process_dynamic_section((ELF_linker_data*)object->linker_data);
		}
	}

	filesystem_close_file(f);
	
	return 1;
}

int elf_process_dynamic_section(ELF_linker_data* object)
{
	if (object == NULL)
	{
		return 0;
	}

	if (object->dynamic_section)
	{
		size_t relocation_entries = 0;
		ELF_rel32* relocation_addr = NULL;

		size_t plt_relocation_entries = 0;
		ELF_rel32* plt_relocation_addr = NULL;

		for (ELF_dyn32* entry = object->dynamic_section; entry->d_tag != DT_NULL; entry++)
		{
			switch (entry->d_tag)
			{
			case DT_PLTRELSZ:
				plt_relocation_entries = entry->d_un.d_val / sizeof(ELF_rel32);
				break;
			case DT_JMPREL:
				plt_relocation_addr = (ELF_rel32*)(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_REL:
				relocation_addr = (ELF_rel32*)(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_RELSZ:
				relocation_entries = entry->d_un.d_val / sizeof(ELF_rel32);
				break;
			case DT_HASH:
				object->hash_table = (uint32_t*)(object->base_address + entry->d_un.d_ptr);
				object->symbol_table_size = object->hash_table[1];
				break;
			case DT_STRTAB:
				object->string_table = (char*)(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_SYMTAB:
				object->symbol_table = (ELF_sym32*)(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_STRSZ:
				object->string_table_size = entry->d_un.d_val;
				break;
			case DT_INIT:
				object->init_func = (void (*)(void))(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_INIT_ARRAY:
				object->array_init_funcs = (void (**)(void))(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_INIT_ARRAYSZ:
				object->num_array_init_funcs = entry->d_un.d_val / sizeof(uintptr_t);
				break;
			case DT_NEEDED:
				break;
			}
		}

		elf_read_symbols(object);
		
		for(ELF_dyn32* entry = object->dynamic_section; entry->d_tag != DT_NULL; entry++)
		{
			if (entry->d_tag == DT_NEEDED)
			{
				const char* lib_name = object->string_table + entry->d_un.d_val;

				uint32_t val;
				if(!hashmap_lookup(object->lib_set, lib_name, &val))
				{
					dynamic_object lib;
					lib.lib_set = object->lib_set;
					lib.symbol_map = object->symbol_map;
					lib.glob_data_symbol_map = object->glob_data_symbol_map;

					if(load_elf(lib_name, &lib, object->userspace))
					{
						hashmap_insert(lib.lib_set, lib_name, 1);
					}
				}
			}
		}

		if (relocation_addr)
		{
			elf_process_relocation_section(object, relocation_addr, relocation_entries);
		}

		//this should wait till runtime, but I'll get lazy linking done later
		if (plt_relocation_addr)
		{
			elf_process_relocation_section(object, plt_relocation_addr, plt_relocation_entries);
		}
	}

	return 0;
}

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8_t)(i))

int elf_read_symbols(ELF_linker_data* object)
{
	if (object->symbol_table) 
	{
		for (size_t i = 0; i < object->symbol_table_size; i++)
		{
			ELF_sym32* symbol = &object->symbol_table[i];

			const char* symbol_name = object->string_table + symbol->name;

			uint32_t val;
			if (!hashmap_lookup(object->symbol_map, symbol_name, &val))
			{
				//if this symbol exists in the image
				if (symbol->section_index)
				{
					hashmap_insert(object->symbol_map, symbol_name, (uintptr_t)(object->base_address + symbol->value));
				}
			}
		}
	}

	return 0;
}

static inline bool elf_relocation_uses_symbol(uint8_t type)
{
	switch (type) 
	{
	case R_386_32:
	case R_386_PC32:
	case R_386_COPY:
	case R_386_GLOB_DAT:
	case R_386_JMP_SLOT:
		return true;
	default:
		return false;
	}
}

void elf_process_relocation_section(ELF_linker_data* object, ELF_rel32* table, size_t rel_entries)
{
	for(size_t entry = 0; entry < rel_entries; entry++)
	{
		uint32_t symbol_index = ELF32_R_SYM(table[entry].info);
		uint8_t relocation_type = ELF32_R_TYPE(table[entry].info);

		ELF_sym32* symbol = &object->symbol_table[symbol_index];
		uintptr_t symbol_val = (uintptr_t)object->base_address + symbol->value;
		const char* symbol_name = NULL;

		if (elf_relocation_uses_symbol(relocation_type))
		{
			symbol_name = object->string_table + symbol->name;
			if (!symbol_name || !hashmap_lookup(object->symbol_map, symbol_name, &symbol_val))
			{
				printf("Unable to locate symbol %d \"%s\"\n", symbol_index, symbol_name);
				symbol_val = 0;
			}
		}

		void* address = object->base_address + table[entry].offset;

		switch (relocation_type)
		{
		case R_386_GLOB_DAT:
			if (symbol_name)
			{
				hashmap_lookup(object->glob_data_symbol_map, symbol_name, &symbol_val);
			}
			*(uintptr_t*)(address) = symbol_val;
			break;
		case R_386_JMP_SLOT:
			*(uintptr_t*)(address) = symbol_val;
			break;
		case R_386_32:
			*(uintptr_t*)(address) = symbol_val + *((size_t*)(address));
			break;
		case R_386_PC32:
			*(uintptr_t*)(address) = symbol_val + *((size_t*)(address)) - (uintptr_t)(address);
			break;
		case R_386_RELATIVE:
			*(uintptr_t*)(address) = (uintptr_t)object->base_address + *((size_t*)(address));
			break;
		case R_386_COPY:
			memcpy(address, (void*)symbol_val, symbol->size);
			break;
		default:
			printf("Unsupported relocation type: %d", relocation_type);
		}
	}
}