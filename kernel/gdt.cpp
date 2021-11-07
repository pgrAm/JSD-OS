#include <stdint.h>
#include <kernel/memorymanager.h>

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

extern "C" 
{
	extern gdt_descriptor gdt_descriptor_location;
	extern gdt_entry gdt_location;
	extern gdt_entry gdt_tss_location;
	extern uint32_t tss_location;
}

extern "C" void adjust_gdt()
{
	//gdt_descriptor_location.offset = (uintptr_t)&gdt_location;

	auto tss = ((uintptr_t)&tss_location);

	gdt_tss_location.base_low = tss & 0x0000FFFF;
	gdt_tss_location.base_middle = (tss >> 16) & 0xFF;
	gdt_tss_location.base_high = (tss >> 24) & 0xFF;
}