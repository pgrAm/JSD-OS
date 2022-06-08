#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <kernel/filesystem.h>
#include <kernel/memorymanager.h>
#include <kernel/elf.h>
#include <kernel/util/hash.h>
#include <kernel/dynamic_object.h>
#include <kernel/kassert.h>

#include <string_view>

struct ELF_linker_data
{
	ELF_dyn32* dynamic_section;
	uintptr_t base_address;

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

	dynamic_object::sym_map* lib_set;
	dynamic_object::sym_map* symbol_map;
	dynamic_object::sym_map* glob_data_symbol_map;
	bool is_userspace;
};

static int elf_is_readable(ELF_ident* file_identifer)
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

static bool elf_is_compatible(ELF_header32* file_header)
{
	if(file_header->machine_arch != ELF_ARCH_X86)
	{
		printf("Can't load, incompatible architecture\n");
		return false;
	}
	else if(file_header->type != ELF_TYPE_EXEC && file_header->type != ELF_TYPE_DYN)
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

static span elf_get_size(ELF_header32* file_header, fs::stream_ref f)
{
	uintptr_t min_address = ~(uintptr_t)0;
	uintptr_t max_address = 0;

	for(size_t i = 0; i < file_header->pgh_entries; i++)
	{
		//seek ahead to the program header table
		const auto pos =
			file_header->pgh_offset + i * sizeof(ELF_program_header32);
		//read the program header
		ELF_program_header32 pg_header;
		f.read(pos, &pg_header, sizeof(ELF_program_header32));

		if(pg_header.type == ELF_PTYPE_LOAD)
		{
			min_address = std::min(min_address, pg_header.virtual_address);
			max_address = std::max(max_address, pg_header.virtual_address + pg_header.mem_size);
		}
	}

	span retval = {(void*)min_address, max_address - min_address};

	return retval;
}

static int elf_read_symbols(ELF_linker_data* object);
static void elf_process_relocation_section(ELF_linker_data* object, ELF_rel32* table, size_t rel_entries);
static int elf_process_dynamic_section(dynamic_object* dyn_obj, directory_stream* lib_dir);
int load_elf(const file_handle* file, dynamic_object* object, bool user, directory_stream* lib_dir);

struct seg_info
{
	uintptr_t aligned_addr;
	uintptr_t virtual_addr;
	size_t num_pages;
	uint32_t flags;
};

static seg_info calculate_segment_info(const ELF_program_header32& pg_header,
									   uintptr_t base_adress, bool user)
{
	uintptr_t aligned_address =
		base_adress + (pg_header.virtual_address & ~(PAGE_SIZE - 1));
	uintptr_t virtual_address = base_adress + pg_header.virtual_address;

	size_t num_pages = memmanager_minimum_pages(
		(virtual_address - aligned_address) + pg_header.mem_size);


	uint32_t flags = 0;
	if(pg_header.flags & PF_WRITE)
	{
		flags |= PAGE_RW;
	}
	if(user)
	{
		flags |= PAGE_USER;
	}

	return {aligned_address, virtual_address, num_pages, flags};
}

int load_elf(const file_handle* file, dynamic_object* object, bool user, directory_stream* lib_dir)
{
	k_assert(file);
	k_assert(object);
	k_assert(lib_dir);

	ELF_ident file_identifer;
	ELF_header32 file_header;

	fs::stream f{file, 0};

	file_info info;
	filesystem_get_file_info(&info, file);

	if(!f)
	{
		printf("could not open elf file %s\n", info.name);
		return 0;
	}

	f.read(0, &file_identifer, sizeof(ELF_ident));

	if(elf_is_readable(&file_identifer))
	{
		f.read(0 + sizeof(ELF_ident), &file_header, sizeof(ELF_header32));

		if(elf_is_compatible(&file_header))
		{
			span s = elf_get_size(&file_header, f);

			size_t object_size = s.size;
			size_t num_pages = memmanager_minimum_pages(object_size);

			uint32_t default_flags = user ? PAGE_USER | PAGE_RW : PAGE_RW;

			//printf("ELF alloc %d %s %d bytes\n", num_pages, info.name, s.size);

			uintptr_t base_adress = (uintptr_t)memmanager_virtual_alloc(s.base, num_pages, default_flags);
			object->segments.emplace_back((void*)base_adress, num_pages);
			object->linker_data = nullptr;

			if(s.base != nullptr) //if the elf cares where its loaded then don't add the base adress
			{
				k_assert((uintptr_t)s.base == base_adress);
				base_adress = 0;
			}

			for(size_t i = 0; i < file_header.pgh_entries; i++)
			{
				//seek ahead to the program header table
				const auto pos = file_header.pgh_offset + i * sizeof(ELF_program_header32);
				//read the program header
				ELF_program_header32 pg_header;
				f.read(pos, &pg_header, sizeof(ELF_program_header32));

				switch(pg_header.type)
				{
				case ELF_PTYPE_DYNAMIC:
				{
					object->linker_data = new ELF_linker_data{
						.dynamic_section =
							(ELF_dyn32*)(base_adress +
										 pg_header.virtual_address),
						.base_address		  = base_adress,
						.lib_set			  = object->lib_set,
						.symbol_map			  = object->symbol_map,
						.glob_data_symbol_map = object->glob_data_symbol_map,
						.is_userspace		  = user,
					};

					//printf("found dynamic section at %X\n", ((ELF_linker_data*)object->linker_data)->dynamic_section);
					break;
				}
				case ELF_PTYPE_LOAD:
				{
					auto seg =
						calculate_segment_info(pg_header, base_adress, user);

					memmanager_set_page_flags((void*)seg.aligned_addr,
											  num_pages, seg.flags);

					//copy file_size bytes from offset to virtual_address
					f.read(pg_header.offset, (void*) seg.virtual_addr,
						   pg_header.file_size);		
				}
				break;
				case ELF_PTYPE_TLS:
				{
					object->tls_image = tls_image_data{
						.pointer =
							(void*)(base_adress + pg_header.virtual_address),
						.image_size = pg_header.file_size,
						.total_size = pg_header.mem_size,
						.alignment	= pg_header.alignment,
					};
				}
				break;
				default:
					break;
				}
			}

			object->entry_point = (void*)(base_adress + file_header.entry_point);

			elf_process_dynamic_section(object, lib_dir);
		}
	}
	else
	{
		printf("Error loading %s\n", info.name);
	}

	return 1;
}

void cleanup_elf(dynamic_object* object)
{
	delete static_cast<ELF_linker_data*>(object->linker_data);
}

static int elf_process_dynamic_section(dynamic_object* dyn_obj, directory_stream* lib_dir)
{
	ELF_linker_data* object = (ELF_linker_data*)dyn_obj->linker_data;

	if(object == nullptr)
	{
		return 0;
	}

	if(object->dynamic_section)
	{
		size_t relocation_entries = 0;
		ELF_rel32* relocation_addr = nullptr;

		size_t plt_relocation_entries = 0;
		ELF_rel32* plt_relocation_addr = nullptr;

		for(ELF_dyn32* entry = object->dynamic_section; entry->d_tag != DT_NULL; entry++)
		{
			switch(entry->d_tag)
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
				if(object->hash_table == NULL)
				{
					printf("NULL ptr access\n");
				}
				object->symbol_table_size = object->hash_table[1];
				break;
			case DT_STRTAB:
				object->string_table = (char*)(object->base_address + entry->d_un.d_ptr);
				break;
			case DT_SYMTAB:
				//printf("found symtab at %X\n", entry->d_un.d_ptr);
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
			default:
				//printf("Unknown section type encountered\n");
				break;
			}
		}

		for(ELF_dyn32* entry = object->dynamic_section; entry->d_tag != DT_NULL; entry++)
		{
			if(entry->d_tag == DT_NEEDED)
			{
				std::string_view lib_name = {object->string_table + entry->d_un.d_val};

				if(!object->lib_set->contains(lib_name))
				{
					dynamic_object lib {
						object->lib_set, 
						object->symbol_map, 
						object->glob_data_symbol_map
					};

					if(auto lib_handle = find_file_by_path(lib_dir, lib_name, 0, 0))
					{
						if(load_elf(&(*lib_handle), &lib, object->is_userspace, lib_dir))
						{
							lib.lib_set->insert(lib_name, 1);

							for(auto&& seg : lib.segments)
							{
								dyn_obj->segments.push_back(seg);
							}
						}
					}
				}
			}
		}

		elf_read_symbols(object);

		if(relocation_addr)
		{
			elf_process_relocation_section(object, relocation_addr, relocation_entries);
		}

		//this should wait till runtime, but I'll get lazy linking done later
		if(plt_relocation_addr)
		{
			elf_process_relocation_section(object, plt_relocation_addr, plt_relocation_entries);
		}
	}

	return 0;
}

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8_t)(i))

static int elf_read_symbols(ELF_linker_data* object)
{
	if(object->symbol_table && object->string_table)
	{
		for(size_t i = 0; i < object->symbol_table_size; i++)
		{
			ELF_sym32* symbol = &object->symbol_table[i];

			if(symbol->section_index)
			{
				std::string_view symbol_name{object->string_table + symbol->name};

				object->symbol_map->insert(symbol_name, object->base_address +
															symbol->value);
			}
		}
	}

	return 0;
}

static inline bool elf_relocation_uses_symbol(uint8_t type)
{
	switch(type)
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

static void elf_process_relocation_section(ELF_linker_data* object, ELF_rel32* table, size_t rel_entries)
{
	if(table == NULL)
	{
		printf("Trying to process NULL section\n");
		return;
	}

	if(object->symbol_table == NULL)
	{
		printf("Object has NULL symbol table\n");
		return;
	}

	for(size_t entry = 0; entry < rel_entries; entry++)
	{
		uint32_t symbol_index = ELF32_R_SYM(table[entry].info);
		uint8_t relocation_type = ELF32_R_TYPE(table[entry].info);

		ELF_sym32* symbol	 = &object->symbol_table[symbol_index];
		uintptr_t symbol_val = object->base_address + symbol->value;
		std::string_view symbol_name;

		if(elf_relocation_uses_symbol(relocation_type))
		{
			auto s_name = object->string_table + symbol->name;

			if(!s_name)
			{
				printf("Unable to locate symbol %u, NULL symbol\n", symbol_index);
				symbol_val = 0;
			}
			else 		
			{
				symbol_name = s_name;
				if(!object->symbol_map->lookup(symbol_name, &symbol_val))
				{
					printf("Unable to locate symbol %u \"%s\"\n", symbol_index, s_name);
					while(true);
					symbol_val = 0;
				}
			}
		}

		const uintptr_t address = object->base_address + table[entry].offset;

		switch(relocation_type)
		{
		case R_386_GLOB_DAT:
			if(!symbol_name.empty())
			{
				object->glob_data_symbol_map->lookup(symbol_name, &symbol_val);
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
			*(uintptr_t*)(address) = symbol_val + *((size_t*)(address)) - address;
			break;
		case R_386_RELATIVE:
			*(uintptr_t*)(address) = object->base_address + *((size_t*)(address));
			break;
		case R_386_COPY:
			if(symbol_val == (uintptr_t)0)
			{
				printf("NULL ptr access\n");
			}
			memcpy((void*)address, (void*)symbol_val, symbol->size);
			break;
		default:
			printf("Unsupported relocation type: %d\n", relocation_type);
		}
	}
}