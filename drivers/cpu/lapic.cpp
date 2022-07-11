#include <stdint.h>
#include <vector>

#include <kernel/locks.h>
#include <kernel/memorymanager.h>
#include <kernel/sysclock.h>
#include <drivers/portio.h>

#include "lapic.h"

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

sync::atomic_flag cpu_spinlock;

void test_entry_point(size_t cpu_id)
{
	cpu_entry_point(cpu_id, std::bit_cast<uint8_t*>(&cpu_spinlock));

	assert(false);
}

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

	params->page_dir	= std::bit_cast<uint32_t>(get_page_directory());
	params->entry_point = std::bit_cast<uint32_t>(&test_entry_point);

	printf("pdir: %X\n", params->page_dir);

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
		(uintptr_t)memmanager_map_to_new_pages(0, 1, PAGE_PRESENT | PAGE_RW);

	auto wrv = (uint16_t*)(zero + ((0x40 << 4 | 0x67))); // Warm reset vector
	wrv[0]	 = 0;
	wrv[1]	 = ap_bootstrap_addr >> 4;

	for(auto cpu : cores)
	{
		if(cpu.lapic_id == my_id) continue;

		add_cpu(cpu.lapic_id);

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

		while(cpu_spinlock.test());
	}
}