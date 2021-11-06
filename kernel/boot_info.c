#include <kernel/boot_info.h>
#include <kernel/multiboot.h>

boot_info boot_information;

extern uint32_t _boot_eax;
extern uint32_t _boot_edx;

void parse_boot_info()
{
	if(_boot_eax == 0x2badb002) //multiboot1
	{
		multiboot_info* info = (multiboot_info*)_boot_edx;

		uintptr_t rd_begin = ((multiboot_modules*)info->m_modsAddr)[0].begin;
		uintptr_t rd_end = ((multiboot_modules*)info->m_modsAddr)[0].end;

		boot_information.ramdisk_location = rd_begin;
		boot_information.ramdisk_size = rd_end - rd_begin;

		boot_information.high_memory = info->m_memoryHi;
		boot_information.low_memory = info->m_memoryLo;
	}
}