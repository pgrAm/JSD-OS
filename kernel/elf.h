#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_BIT_WIDTH_64 0x02
#define ELF_BIT_WIDTH_32 0x01
#define ELF_LITTLE_ENDIAN 0x01

#define ELF_ARCH_X86 0x03
#define ELF_ARCH_X64 0x3E

#define ELF_FLAG_EXECUTABLE 0x01
#define ELF_FLAG_WRITABLE 0x02
#define ELF_FLAG_READABLE 0x04

enum ELF_type
{
	ELF_TYPE_NONE = 0x00,
	ELF_TYPE_RELOCATE = 0x01,
	ELF_TYPE_EXEC = 0x02,
	ELF_TYPE_DYN = 0x03
};

enum ELF_pgheader_types
{
	ELF_PTYPE_NULL 		= 0x00000000, 	//Program header table entry unused
	ELF_PTYPE_LOAD 		= 0x00000001,	//Loadable segment
	ELF_PTYPE_DYNAMIC	= 0x00000002, 	//Dynamic linking information
	ELF_PTYPE_INTERP 	= 0x00000003, 	//Interpreter information
	ELF_PTYPE_NOTE 		= 0x00000004, 	//Auxiliary information
	ELF_PTYPE_SHLIB		= 0x00000005, 	//reserved
	ELF_PTYPE_PHDR 		= 0x00000006, 	//segment containing program header table itself
	ELF_PTYPE_LOOS 		= 0x60000000, 	
	ELF_PTYPE_HIOS		= 0x6FFFFFFF, 	
	ELF_PTYPE_LOPROC	= 0x70000000, 	
	ELF_PTYPE_HIPROC	= 0x7FFFFFFF 	
};

enum ELF_secheader_types
{
	SHT_NULL			= 0x0, 	//Section header table entry unused
	SHT_PROGBITS		= 0x1, 	//Program data
	SHT_SYMTAB			= 0x2, 	//Symbol table
	SHT_STRTAB			= 0x3, 	//String table
	SHT_RELA			= 0x4, 	//Relocation entries with addends
	SHT_HASH			= 0x5, 	//Symbol hash table
	SHT_DYNAMIC			= 0x6, 	//Dynamic linking information
	SHT_NOTE			= 0x7, 	//Notes
	SHT_NOBITS			= 0x8, 	//Program space with no data(bss)
	SHT_REL				= 0x9, 	//Relocation entries, no addends
	SHT_SHLIB			= 0x0A, //Reserved
	SHT_DYNSYM			= 0x0B, //Dynamic linker symbol table
	SHT_INIT_ARRAY		= 0x0E, //Array of constructors
	SHT_FINI_ARRAY		= 0x0F, //Array of destructors
	SHT_PREINIT_ARRAY	= 0x10, //Array of pre - constructors
	SHT_GROUP			= 0x11, //Section group
	SHT_SYMTAB_SHNDX	= 0x12, //Extended section indices
	SHT_NUM				= 0x13, //Number of defined types.
	SHT_LOOS			= 0x60000000 	//Start OS - specific.
};

typedef struct 
{
	char 		magic[4]; 		//should be 0x7F followed by "ELF"
	uint8_t 	bit_width; 		//1 for 32 bit, 2 for 64 
	uint8_t 	endianness; 	//1 for little endian, 2 for big
	uint8_t 	elf_version; 	//version of the elf format
	uint8_t 	abi;			//don't really care, set to 0
	uint8_t 	padding[8];
} ELF_ident;

typedef struct 
{
	uint16_t 	type;			//what kind of file is this, 0x02 for executable
	uint16_t	machine_arch;	//what kind of instruction set, 0x03 for x86, 0x3E for x64
	uint32_t 	e_version;		//another verion field for some reason
	uint32_t	entry_point;	//where to start execution, (memory address)
	uint32_t	pgh_offset;		//pointer to the program header table
	uint32_t	sec_offset;		//pointer to the section header table
	uint32_t	flags;			//not sure
	uint16_t	header_size;	//size of this header in bytes
	uint16_t	pgh_size;		//size of a program header in bytes
	uint16_t	pgh_entries;	//number of entries in the program header
	uint16_t	sec_size;		//size of the section header in bytes
	uint16_t	sec_entries;	//number of entries in the section header
	uint16_t	sec_names_index;//index of the section header containing section names
}
ELF_header32;

typedef struct 
{
	uint16_t 	type;			//what kind of file is this, 0x02 for executable
	uint16_t	machine_arch;	//what kind of instruction set, 0x03 for x86, 0x3E for x64
	uint32_t 	e_version;		//another verion field for some reason
	uint64_t	entry_point;	//where to start execution, (memory address)
	uint64_t	pgh_offset;		//pointer to the program header table
	uint64_t	sec_offset;		//pointer to the section header table
	uint32_t	flags;			//not sure
	uint16_t	header_size;	//size of this header in bytes
	uint16_t	pgh_size;		//size of a program header in bytes
	uint16_t	pgh_entries;	//number of entries in the program header
	uint16_t	sec_size;		//size of the section header in bytes
	uint16_t	sec_entries;	//number of entries in the section header
	uint16_t	sec_names_index;//index of the section header containing section names
}
ELF_header64;

typedef struct 
{
	uint32_t 	type; 				//type of segment
	uint32_t 	offset;				//offset into the file containing this segment
	uint32_t	virtual_address;	//adress in virtual memory to load this segment to
	uint32_t	physical_address;	//physical address to load segment if neccessary
	uint32_t	file_size;			//size of this segment in the file 
	uint32_t	mem_size;			//size of this segment in memory
	uint32_t	flags;
	uint32_t	alignment;			//alignment requirements for segment
} ELF_program_header32;

typedef struct 
{
	uint32_t 	type; 				//type of segment
	uint32_t	flags;
	uint64_t 	offset;				//offset into the file containing this segment
	uint64_t	virtual_address;	//adress in virtual memory to load this segment to
	uint64_t	physical_address;	//physical address to load segment if neccessary
	uint64_t	file_size;			//size of this segment in the file 
	uint64_t	mem_size;			//size of this segment in memory
	uint64_t	alignment;			//alignment requirements for segment
} ELF_program_header64;

typedef struct
{
	uint32_t 	sh_name; 			//name of section
	uint32_t 	sh_type;			//type of section
	uint32_t	sh_flags;			//section flags
	uint32_t	sh_addr;			//virtual address of the section in memory, for sections that are loaded. 
	uint32_t	sh_offset;			//size of this section in the file 
	uint32_t	sh_size;			//size of this section in memory
	uint32_t	sh_link;			//index of associated section
	uint32_t	sh_info;			//info depending on section type
	uint32_t	sh_addralign;		
	uint32_t	sh_entsize;			//size of each entry
} ELF_section_header32;

typedef struct {
	uint32_t offset;
	uint32_t info;
} ELF_Rel32;

typedef struct {
	uint32_t offset;
	uint32_t info;
	uint32_t addend;
} ELF_Rela32;

typedef struct {
	int32_t d_tag;
	union {
		uint32_t d_val;
		uint32_t d_ptr;
		uint32_t d_off;
	} d_un;
} ELF_dyn32;

enum ELF_dyn_section_tags
{
	DT_NEEDED = 1,
	DT_HASH = 4,
	DT_STRTAB = 5, // Dynamic String Table
	DT_SYMTAB = 6, // Dynamic Symbol Table
	DT_RELA = 7,
	DT_RELASZ = 8,
	DT_RELAENT = 9,
	DT_STRSZ = 10, // Size of string table
	DT_INIT = 11, // initialization function
	DT_INIT_ARRAY = 25, // array of constructors
	DT_INIT_ARRAYSZ = 26 // size of the table of constructors
};

#include <dynamic_object.h>

int load_elf(const char* path, dynamic_object* newTask);

#endif