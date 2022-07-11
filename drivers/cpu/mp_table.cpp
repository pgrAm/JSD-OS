#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <kernel/memorymanager.h>
#include <bit>
#include <string>
#include <vector>
#include <common/util.h>
#include <drivers/cpu/lapic.h>

enum entry_type : uint8_t
{
	PROCESSOR = 0,
	BUS,
	IO_APIC,
	IO_INTERRUPT,
	LOCAL_INTERRUPT,
};

struct __attribute__((packed)) mp_header
{
	char signature[4]; // should be "_MP_"
	uint32_t table_ptr;
	uint8_t length; // multiply by 16 to get bytes
	uint8_t version;
	uint8_t checksum;
	uint8_t use_default_config;
	uint32_t features;
};

struct __attribute__((packed)) mp_table
{
	char signature[4]; // should be "PCMP"
	uint16_t length;
	uint8_t version;
	uint8_t checksum;
	char oem_id[8];
	char product_id[12];
	uint32_t oem_table;
	uint16_t oem_table_size;
	uint16_t num_entries;
	uint32_t lapic_address;
	uint16_t extended_table_length;
	uint8_t extended_table_checksum;
	uint8_t reserved;
};

struct __attribute__((packed)) mp_processor_entry
{
	uint8_t type;
	uint8_t local_apic_id;
	uint8_t local_apic_version;
	uint8_t flags;
	uint32_t signature;
	uint32_t feature_flags;
	uint64_t reserved;
};

struct __attribute__((packed)) mp_io_apic_entry
{
	uint8_t type;
	uint8_t id;
	uint8_t version;
	uint8_t flags;
	uint32_t address;
};


static uintptr_t find_header_addr(uintptr_t start, size_t num_bytes)
{
	for(uintptr_t addr = start; addr < start + num_bytes; addr += 0x10)
	{
		if(read_addr<uint32_t>(addr) == 0x5F504D5Fu) //"_MP_"
		{
			return addr;
		}
	}
	return 0;
}

uintptr_t create_mapping(uintptr_t physical, size_t num_pages, page_flags_t flags)
{
	return (uintptr_t)memmanager_map_to_new_pages(physical & ~(PAGE_SIZE - 1),
												  num_pages, flags) +
		   (physical & (PAGE_SIZE - 1));
}

void parse_header(uintptr_t addr)
{
	mp_header* header = std::bit_cast<mp_header*>(addr);

	std::vector<cpu_core> cores;

	if(header->use_default_config == 0)
	{
		printf("%X\n", header->table_ptr);
		printf("length: %X\n", header->length);
		printf("version: %X\n", header->version);

		auto table_addr = create_mapping(header->table_ptr, 1, PAGE_PRESENT);
		mp_table* table = std::bit_cast<mp_table*>(table_addr);

		printf("%X\n", table);
		printf("signature: %s\n", std::string{table->signature, 4}.c_str());
		printf("OEM string: %s\n", std::string{table->oem_id, 8}.c_str());
		printf("%d entries in table\n", table->num_entries);

		auto entry_addr = table_addr + sizeof(mp_table);

		for(size_t i = 0; i < table->num_entries; i++)
		{
			switch(read_addr<uint8_t>(entry_addr))
			{
			case entry_type::PROCESSOR:
			{
				auto entry = read_addr<mp_processor_entry>(entry_addr);

				printf("Processor %d found\n", entry.local_apic_id);
				cores.emplace_back(entry.local_apic_id);
				entry_addr += sizeof(mp_processor_entry);
			}
				break;
			default:
				entry_addr += sizeof(mp_io_apic_entry);
				break;
			}
		}

		init_smp(table->lapic_address, cores);
	}
	else
	{
		printf("There is no table\n");
	}
}

static bool find_mp_header()
{
	auto bda =
		(uintptr_t)memmanager_map_to_new_pages(0, 1, PAGE_PRESENT | PAGE_RW);

	auto ebda_addr = *(uint16_t*)(bda + 0x40e) * 16;

	//if(ebda_addr)
	{
		printf("Searching EBDA %X\n", ebda_addr);

		auto ebda = create_mapping(ebda_addr, 1, PAGE_PRESENT);
		if(auto addr = find_header_addr(ebda, 0x400))
		{
			printf("MP structure found in ebda at %X\n",
				   ebda_addr + (addr - ebda));
			parse_header(addr);
			return true;
		}
	}

	if(auto base_end = *(uint16_t*)(bda + 0x413) * 0x400; base_end != ebda_addr)
	{
		printf("Searching last KB %X\n", base_end);

		auto last_1k = create_mapping(base_end, 1, PAGE_PRESENT);
		if(auto addr = find_header_addr(last_1k, 0x400))
		{
			printf("MP structure found at %X\n", base_end + (addr - last_1k));
			parse_header(addr);
			return true;
		}
	}

	printf("Searching bios ROM %X\n", 0xE0000);

	auto bios_rom = create_mapping(0xE0000, 0x20, PAGE_PRESENT);
	if(auto addr = find_header_addr(bios_rom, 0x20 * 0x1000))
	{
		printf("MP structure found in rom at %X\n",
			   0xE0000 + (addr - bios_rom));
		parse_header(addr);
		return true;
	}

	printf("MP table not found\n");
	return false;
}

extern "C" void mp_table_init()
{
	find_mp_header();
}