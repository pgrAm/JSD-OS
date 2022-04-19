#include <kernel/boot_info.h>
#include <kernel/multiboot.h>
#include <kernel/physical_manager.h>
#include <kernel/memorymanager.h>

#include <stdio.h>

boot_info boot_information;

extern uint32_t _boot_eax;
extern uint32_t _boot_edx;
extern uintptr_t _kernel_location;

extern void _BSS_END_;
extern void _IMAGE_END_;
extern void _KERNEL_START_;

void reserve_boot_mem()
{
	if(!boot_information.memmap_location)
		return;

	size_t mmap_entries = boot_information.memmap_size / sizeof(multiboot_mmap_entry);

	multiboot_mmap_entry* memmap = (multiboot_mmap_entry*)boot_information.memmap_location;

	for(size_t i = 0; i < mmap_entries; i++)
	{
		if(memmap[i].m_type != 1)
		{
			physical_memory_reserve(memmap[i].m_addr, memmap[i].m_length);
		}
	}
}

void parse_boot_info()
{
	boot_information.kernel_location = _kernel_location;
	boot_information.kernel_size	= (uintptr_t)&_IMAGE_END_
									- (uintptr_t)&_KERNEL_START_;

	if(_boot_eax == 0x2badb002) //multiboot1
	{
		multiboot_info* info = (multiboot_info*)_boot_edx;

		uintptr_t rd_begin = ((multiboot_modules*)info->m_modsAddr)[0].begin;
		uintptr_t rd_end = ((multiboot_modules*)info->m_modsAddr)[0].end;

		boot_information.ramdisk_location = rd_begin;
		boot_information.ramdisk_size = rd_end - rd_begin;

		boot_information.high_memory = info->m_memoryHi;
		boot_information.low_memory = info->m_memoryLo;

		if(info->m_flags & (1 << 6))
		{
			boot_information.memmap_location = info->m_mmap_addr;
			boot_information.memmap_size = info->m_mmap_length;
		}
		else
		{
			boot_information.memmap_location = 0;
			boot_information.memmap_size = 0;
		}
	}
	else if(_boot_eax == 0x36d76289)  //multiboot2
	{
	}
}