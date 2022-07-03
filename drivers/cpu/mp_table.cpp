#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <kernel/memorymanager.h>
#include <kernel/locks.h>
#include <bit>
#include <string>
#include <vector>
#include <drivers/portio.h>

#include <kernel/sysclock.h>

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

template<typename T>
T read_addr(uintptr_t addr) requires(std::is_trivially_copyable_v<T>)
{
	T value;
	memcpy(&value, (void*)addr, sizeof(T));
	return value;
}

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

struct cpu_core
{
	size_t lapic_id;
};

#define ENABLE 0x00000100  // Unit Enable
#define INIT 0x00000500	   // INIT/RESET
#define STARTUP 0x00000600 // Startup IPI
#define DELIVS 0x00001000  // Delivery status
#define ASSERT 0x00004000  // Assert interrupt (vs deassert)
#define DEASSERT 0x00000000
#define LEVEL 0x00008000 // Level triggered
#define BCAST 0x00080000 // Send to all APICs, including self.
#define BUSY 0x00001000
#define FIXED 0x00000000
#define X1 0x0000000B		// divide counts by 1
#define PERIODIC 0x00020000 // Periodic
#define MASKED 0x00010000	// Interrupt masked

enum lapic_reg : uint32_t
{
	ID			   = (0x0020 / 4), // ID
	VERSION		   = (0x0030 / 4), // Version
	TASK_PRI	   = (0x0080 / 4), // Task Priority
	EOI			   = (0x00B0 / 4), // EOI
	SVR			   = (0x00F0 / 4), // Spurious Interrupt Vector
	ESR			   = (0x0280 / 4), // Error Status
	INT_COMMAND_LO = (0x0300 / 4), // Interrupt Command
	INT_COMMAND_HI = (0x0310 / 4), // Interrupt Command [63:32]
	TIMER		   = (0x0320 / 4), // Local Vector Table 0 (TIMER)
	PCINT		   = (0x0340 / 4), // Performance Counter LVT
	LINT0		   = (0x0350 / 4), // Local Vector Table 1 (LINT0)
	LINT1		   = (0x0360 / 4), // Local Vector Table 2 (LINT1)
	ERROR		   = (0x0370 / 4), // Local Vector Table 3 (ERROR)
	TICR		   = (0x0380 / 4), // Timer Initial Count
	TCCR		   = (0x0390 / 4), // Timer Current Count
	TDCR		   = (0x03E0 / 4)  // Timer Divide Configuration
};

static uint32_t lapic_read(uint32_t* base, uint32_t reg)
{
	return __atomic_load_n(base + reg, __ATOMIC_SEQ_CST);
}

static void lapic_write(uint32_t* base, uint32_t reg, uint32_t value)
{
	__atomic_store_n(base + reg, value, __ATOMIC_SEQ_CST);
	__atomic_load_n(base + (0x20 / 4), __ATOMIC_SEQ_CST);
}

struct __attribute__((packed)) ap_bootstrap_params
{
	uint32_t page_dir;
	uint32_t entry_point;
	uint32_t processor_id;
};

extern uint8_t _binary_ap_bootstrap_bin_start;
extern uint8_t _binary_ap_bootstrap_bin_end;

sync::atomic_flag cpu_spinlock;

void test_entry_point(size_t cpu_id)
{
	printf("CPU %d initialized\n", cpu_id);

	cpu_spinlock.clear();

	assert(false);
}

uintptr_t create_mapping(uintptr_t physical, size_t num_pages, page_flags_t flags)
{
	return (uintptr_t)memmanager_map_to_new_pages(physical & ~(PAGE_SIZE - 1),
												  num_pages, flags) +
		   (physical & (PAGE_SIZE - 1));
}

void init_smp(uintptr_t lapic_phys_addr, std::vector<cpu_core>& cores)
{
	uintptr_t ap_bootstrap_addr = 0x7000;
	size_t ap_bootstrap_size =
		&_binary_ap_bootstrap_bin_end - &_binary_ap_bootstrap_bin_start;

	size_t params_offset = ap_bootstrap_size - sizeof(ap_bootstrap_params);

	memcpy((void*)ap_bootstrap_addr, &_binary_ap_bootstrap_bin_start,
		   ap_bootstrap_size);

	ap_bootstrap_params* params =
		std::bit_cast<ap_bootstrap_params*>(ap_bootstrap_addr + params_offset);

	params->page_dir = std::bit_cast<uint32_t>(get_page_directory());
	params->entry_point = std::bit_cast<uint32_t>(&test_entry_point);

	printf("%X\n", params->page_dir);

	auto lapic_addr =
		(uint32_t*)memmanager_map_to_new_pages(lapic_phys_addr &
												   ~(PAGE_SIZE - 1),
											   1, PAGE_PRESENT) +
		(lapic_phys_addr & (PAGE_SIZE - 1));

	auto my_id = lapic_read(lapic_addr, 0x20 / 4);

	printf("BSP id: %X\n", my_id);

	outb(0x70, 0x0F); 
	outb(0x71, 0x0A);

	auto zero =
		(uintptr_t)memmanager_map_to_new_pages(0,
											   1, PAGE_PRESENT | PAGE_RW);

	auto wrv = (uint16_t*)(zero + ((0x40 << 4 | 0x67))); // Warm reset vector
	wrv[0]	 = 0;
	wrv[1]	 = ap_bootstrap_addr >> 4;

	for(auto cpu : cores)
	{
		if(cpu.lapic_id == my_id) continue;

		cpu_spinlock.test_and_set();

		params->processor_id = cpu.lapic_id;

		printf("Starting: %X\n", cpu.lapic_id);

		lapic_write(lapic_addr, lapic_reg::INT_COMMAND_HI, cpu.lapic_id << 24);
		lapic_write(lapic_addr, lapic_reg::INT_COMMAND_LO,
					INIT | LEVEL | ASSERT);
		sysclock_sleep(200, MICROSECONDS);
		lapic_write(lapic_addr, lapic_reg::INT_COMMAND_LO, INIT | LEVEL);
		sysclock_sleep(100, MICROSECONDS);

		for(size_t i = 0; i < 2; i++)
		{
			lapic_write(lapic_addr, lapic_reg::INT_COMMAND_HI,
						cpu.lapic_id << 24);
			lapic_write(lapic_addr, lapic_reg::INT_COMMAND_LO,
						STARTUP | (ap_bootstrap_addr >> 12));
			sysclock_sleep(200, MICROSECONDS);
		}

		__asm__ volatile("cli");
		while(cpu_spinlock.test())
			;
	}
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