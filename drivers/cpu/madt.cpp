#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <kernel/memorymanager.h>
#include <common/util.h>
#include <algorithm>
#include <bit>
#include <drivers/cpu/lapic.h>
#include <vector>

struct __attribute__((packed)) acpi_rsdp
{
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_addr;
};

struct __attribute__((packed)) acpi_sdt
{
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	char creator_id[4];
	uint32_t creator_revision;
};

struct __attribute__((packed)) acpi_rsdt
{
	acpi_sdt header;
	uint32_t sdt_ptr[];
};

uintptr_t create_mapping(uintptr_t physical, size_t num_pages,
						 page_flags_t flags)
{
	return (uintptr_t)memmanager_map_to_new_pages(physical & ~(PAGE_SIZE - 1),
												  num_pages, flags) +
		   (physical & (PAGE_SIZE - 1));
}

bool checksum_valid(uintptr_t addr, size_t size)
{
	auto ptr = std::bit_cast<uint8_t*>(addr);
	return std::accumulate(ptr, ptr + size, (uint8_t)0) == 0;
}

acpi_rsdp* get_rsdp()
{
	auto bda = create_mapping(0, 1, PAGE_PRESENT | PAGE_RW);

	printf("Searching EBDA\n");

	auto ebda_addr =
		static_cast<uintptr_t>(read_addr<uint16_t>(bda + 0x40e)) * 16;

	auto ebda = create_mapping(ebda_addr, 1, PAGE_PRESENT);

	for(auto addr = ebda; addr < ebda + 0x400; addr += 0x10)
	{
		if(memcmp((void*)addr, "RSD PTR ", 8) == 0 && checksum_valid(addr, 20))
		{
			return std::bit_cast<acpi_rsdp*>(addr);
		}
	}

	printf("Searching bios ROM\n");

	auto bios_rom = create_mapping(0xE0000, 0x20, PAGE_PRESENT);

	for(auto addr = bios_rom; addr < bios_rom + 0x20000; addr += 0x10)
	{
		if(memcmp((void*)addr, "RSD PTR ", 8) == 0 && checksum_valid(addr, 20))
		{
			return std::bit_cast<acpi_rsdp*>(addr);
		}
	}

	printf("Can't find rsdp\n");

	return nullptr;
}

uintptr_t acpi_get_table_addr(const char signature[4], int index)
{
	auto rsdp = get_rsdp();

	if(!rsdp) return 0;

	auto rsdt =
		(acpi_rsdt*)create_mapping((uintptr_t)rsdp->rsdt_addr, 0x1, PAGE_PRESENT);

	size_t num_entries = (rsdt->header.length - sizeof(acpi_sdt)) / 4;

	size_t num_found = 0;

	for(size_t i = 0; i < num_entries; i++)
	{
		auto ptr = (acpi_sdt*)create_mapping((uintptr_t)rsdt->sdt_ptr[i], 1,
											 PAGE_PRESENT);

		if(!memcmp(ptr->signature, &signature[0], sizeof(ptr->signature)) &&
		   checksum_valid(std::bit_cast<uintptr_t>(ptr), ptr->length) && num_found++ == index)
		{
			printf("Found MADT\n");

			return std::bit_cast<uintptr_t>(ptr);
		}
	}

	return 0;
}

struct __attribute__((packed)) madt
{
	acpi_sdt header;
	uint32_t lapic_address;
	uint32_t flags;
	char entries[];
};

struct __attribute__((packed)) madt_header
{
	uint8_t type;
	uint8_t length;
};

struct __attribute__((packed)) madt_lapic
{
	madt_header header;
	uint8_t acpi_processor_uid;
	uint8_t lapic_id;
	uint32_t flags;
} ;

extern "C" void madt_init()
{
	auto table_addr = acpi_get_table_addr("APIC", 0);
	madt* table		= std::bit_cast<madt*>(table_addr);

	if(!table) return;

	std::vector<cpu_core> cores;

	for(auto madt_addr = std::bit_cast<uintptr_t>(&table->entries[0]);
		madt_addr < table_addr + table->header.length;)
	{
		auto header = read_addr<madt_header>(madt_addr);

		if(read_addr<madt_header>(madt_addr).type == 0)
		{
			madt_lapic lapic = read_addr<madt_lapic>(madt_addr);

			if((lapic.flags & 1) ^ ((lapic.flags >> 1) & 1))
			{
				cores.emplace_back(lapic.lapic_id);
			}
		}

		madt_addr += header.length;
	}

	init_smp(table->lapic_address, cores);
}