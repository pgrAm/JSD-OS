#include <stdint.h>
#include <kernel/memorymanager.h>
#include <kernel/locks.h>
#include <kernel/tss.h>
#include <kernel/cpu.h>

#include <stdio.h>

extern cpu_state boot_cpu_state;

RECLAIMABLE_DATA constinit gdt_descriptor gdt_descriptor_location =
	{sizeof(gdt) - 1, &boot_cpu_state.arch.m_gdt};

void load_TSS(uint16_t tss_seg)
{
	asm volatile("ltr %0" : : "r"(tss_seg));
}

constexpr uintptr_t get_seg_base(gdt_entry& e)
{
	return (uintptr_t)(e.base_lo << 0) |
		   (uintptr_t)(e.base_mid << 16) |
		   (uintptr_t)(e.base_hi << 24);
}

uintptr_t get_TLS_seg_base(arch_data* n_cpu)
{
	return get_seg_base(n_cpu->m_gdt.tls_data);
}

uintptr_t get_CPU_seg_base(arch_data* n_cpu)
{
	return get_seg_base(n_cpu->m_gdt.cpu_data);
}

void reload_CPU_seg()
{
	auto seg = (uint16_t)(offsetof(gdt, cpu_data));
	asm volatile("mov %0, %%fs" : : "r"(seg));
}
void set_seg_base(gdt_entry& e, uintptr_t val)
{
	e.base_lo  = val & 0xFFFF;
	e.base_mid = (val >> 16) & 0xFF;
	e.base_hi  = (val >> 24) & 0xFF;
}
void set_CPU_seg_base(arch_data* n_cpu, uintptr_t val)
{
	set_seg_base(n_cpu->m_gdt.cpu_data, val);
	reload_CPU_seg();
}

void set_TLS_seg_base(arch_data* n_cpu, uintptr_t val)
{
	set_seg_base(n_cpu->m_gdt.tls_data, val);
	reload_TLS_seg();
}

void reload_TLS_seg()
{
	auto seg = (uint16_t)(offsetof(gdt, tls_data)) | 3;
	asm volatile("mov %0, %%gs" : : "r"(seg));
}

void setup_GDT(arch_data* cpu)
{
	gdt_descriptor gdt_descriptor_location = {sizeof(gdt) - 1, &cpu->m_gdt};

	asm volatile("lgdt %0" : : "m"(gdt_descriptor_location));
}

void create_TSS(arch_data* cpu, uintptr_t stack_addr)
{
	memset(&cpu->m_tss, 0, sizeof(tss));

	cpu->m_tss.stack_seg = (uint16_t)(offsetof(gdt, data));
	cpu->m_tss.esp0	  = stack_addr;

	auto tss_addr = std::bit_cast<uintptr_t>(&cpu->m_tss);

	cpu->m_gdt.tss_data = {
		.limit_lo	 = sizeof(tss) & 0x0000FFFF,
		.base_lo	 = (uint16_t)(tss_addr & 0x0000FFFF),
		.base_mid	 = (uint8_t)((tss_addr >> 16) & 0xFF),
		.access		 = 0x89,
		.granularity = ((sizeof(tss) >> 16) & 0x0F) | 0x40,
		.base_hi	 = (uint8_t)((tss_addr >> 24) & 0xFF),
	};

	uint16_t tss_seg = (uint16_t)(offsetof(gdt, tss_data)) | 3;
	
	load_TSS(tss_seg);
}