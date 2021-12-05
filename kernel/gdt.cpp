#include <stdint.h>
#include <kernel/memorymanager.h>
#include <kernel/locks.h>
#include <kernel/tss.h>
#include <stdio.h>

struct __attribute__((packed)) gdt_entry
{
	uint16_t limit_low; 
	uint16_t base_low;  
	uint8_t  base_middle;
	uint8_t  access;
	uint8_t  granularity;
	uint8_t  base_high; 
};

struct __attribute__((packed)) gdt_descriptor
{
	uint16_t size;
	uint32_t offset;
};

struct __attribute__((packed)) tss
{
	uint16_t lin;
	uint16_t reserved0;
	uint32_t esp0;
	uint16_t stack_seg;
	uint16_t reserved1;
	uint32_t r[23];
};

extern "C" 
{
	extern gdt_descriptor gdt_descriptor_location;
	extern gdt_entry gdt_location;
	extern gdt_entry gdt_data_location;
	extern gdt_entry gdt_tss_location;
}

static kernel_mutex tss_mtx = {-1, 0};

extern "C" void load_TSS(uint16_t tss_seg);

tss* create_TSS(uintptr_t stack_addr)
{
	//lock modifications to the TSS
	scoped_lock l{&tss_mtx};

	auto n_tss = new tss{};

	memset(n_tss, 0, sizeof(tss));

	n_tss->stack_seg = ((uintptr_t)&gdt_data_location - (uintptr_t)&gdt_location);
	n_tss->esp0 = stack_addr;

	auto tss_addr = (uintptr_t)n_tss;

	gdt_tss_location.limit_low = sizeof(tss) & 0x0000FFFF;
	gdt_tss_location.base_low = tss_addr & 0x0000FFFF;
	gdt_tss_location.base_middle = (tss_addr >> 16) & 0xFF;
	gdt_tss_location.access = 0x89;
	gdt_tss_location.base_high = (tss_addr >> 24) & 0xFF;
	gdt_tss_location.granularity = ((sizeof(tss) >> 16) & 0x0F) | 0x40;

	uint16_t tss_seg = ((uintptr_t)&gdt_tss_location - (uintptr_t)&gdt_location) | 3;
	
	load_TSS(tss_seg);

	printf("%X\n", tss_addr);

	return n_tss;
}