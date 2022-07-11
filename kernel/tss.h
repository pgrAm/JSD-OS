#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct __attribute__((packed)) tss
{
	uint16_t lin;
	uint16_t reserved0;
	uint32_t esp0;
	uint16_t stack_seg;
	uint16_t reserved1;
	uint32_t r[23];
};

struct __attribute__((packed)) gdt_entry
{
	uint16_t limit_lo;
	uint16_t base_lo;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_hi;
};

struct __attribute__((packed)) gdt
{
	gdt_entry null		= {0, 0, 0, 0, 0, 0};
	gdt_entry code		= {0xffff, 0x0000, 0x00, 0x9A, 0xCF, 0x00};
	gdt_entry data		= {0xffff, 0x0000, 0x00, 0x92, 0xCF, 0x00};
	gdt_entry user_code = {0xffff, 0x0000, 0x00, 0xFA, 0xCF, 0x00};
	gdt_entry user_data = {0xffff, 0x0000, 0x00, 0xF2, 0xCF, 0x00};
	gdt_entry tls_data	= {0xffff, 0x0000, 0x00, 0xF2, 0xCF, 0x00};
	gdt_entry cpu_data	= {0xffff, 0x0000, 0x00, 0x92, 0xCF, 0x00};
	gdt_entry tss_data	= {0, 0, 0, 0, 0, 0};
};

struct __attribute__((packed)) gdt_descriptor
{
	uint16_t size;
	gdt* offset;
};

struct __attribute__((packed)) saved_regs
{
	uintptr_t esp;
	uintptr_t esp0;
	uintptr_t cr3;

	uint32_t tls_gdt_hi;
	uint16_t tls_base_lo;
	uint16_t unused;
};

struct __attribute__((packed)) user_transition_stack_items
{
	uint32_t ebp;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t flags;
	uint32_t eip;
	uint32_t cs;
	uintptr_t code_addr;
	uintptr_t stack_addr;
};

constexpr saved_regs init_tcb_regs(uintptr_t stack_top, uintptr_t page_dir,
								   uintptr_t tls_ptr)
{
	return {
		.esp  = stack_top - sizeof(user_transition_stack_items),
		.esp0 = stack_top,
		.cr3  = page_dir,
		.tls_gdt_hi =
			(tls_ptr & 0xFF000000) | 0x00CFF200 | ((tls_ptr >> 16) & 0xFF),
		.tls_base_lo = static_cast<uint16_t>(tls_ptr & 0xFFFF),
	};
}

struct TCB;

struct __attribute__((packed)) arch_data
{
	tss m_tss;
	gdt m_gdt;

	TCB* tcb;
};

void setup_GDT(arch_data* cpu);
void create_TSS(arch_data* cpu, uintptr_t stack_addr);
void reload_TLS_seg();
uintptr_t get_TLS_seg_base(arch_data* n_cpu);
uintptr_t get_CPU_seg_base(arch_data* n_cpu);
void set_CPU_seg_base(arch_data* n_cpu, uintptr_t val);
void set_TLS_seg_base(arch_data* n_cpu, uintptr_t val);

#endif