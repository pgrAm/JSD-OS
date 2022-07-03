#include <kernel/bootstrap/boot_info.h>
#include <kernel/multiboot.h>
#include <kernel/physical_manager.h>
#include <kernel/memorymanager.h>

#include <stdio.h>

boot_info boot_information;

extern uint32_t _boot_eax;
extern uint32_t _boot_ebx;
extern uintptr_t _kernel_location;

extern void _BSS_END_;
extern void _IMAGE_END_;
extern void _KERNEL_START_;

extern uintptr_t boot_mapper_map_mem(uintptr_t addr, size_t bytes);

RECLAIMABLE void reserve_boot_mem()
{
	if(boot_information.memmap_location)
	{
		uintptr_t memmap = boot_mapper_map_mem(boot_information.memmap_location,
											   boot_information.memmap_size);

		for(uintptr_t addr = memmap;
			(uintptr_t)addr < (memmap + boot_information.memmap_size);)
		{
			multiboot_mmap_entry* entry = (multiboot_mmap_entry*)addr;
			if(entry->m_type == 1 && entry->m_length)
			{
				physical_memory_free((uintptr_t)entry->m_addr,
									 (size_t)entry->m_length);
			}
			addr += entry->m_size + sizeof(uint32_t);
		}
	}
	else
	{
		physical_memory_free(0u, boot_information.low_memory * 1024);
		physical_memory_free(0x00100000u, boot_information.high_memory * 1024);

		//reserve BIOS & VRAM & EBDA
		physical_memory_reserve(0x80000, 0x100000 - 0x80000);
	}
}

RECLAIMABLE void parse_boot_info()
{
	boot_information.kernel_location = _kernel_location;
	boot_information.kernel_size =
		(uintptr_t)&_IMAGE_END_ - (uintptr_t)&_KERNEL_START_;

	if(_boot_eax == 0x2badb002) //multiboot1
	{
		multiboot_info* info =
			(multiboot_info*)boot_mapper_map_mem(_boot_ebx,
												 sizeof(multiboot_info));
		multiboot_modules* modules = (multiboot_modules*)
			boot_mapper_map_mem(info->m_modsAddr,
								info->m_modsCount * sizeof(multiboot_modules));

		uintptr_t rd_begin = modules[0].begin;
		uintptr_t rd_end   = modules[0].end;

		boot_information.ramdisk_location = rd_begin;
		boot_information.ramdisk_size	  = rd_end - rd_begin;

		boot_information.high_memory = info->m_memoryHi;
		boot_information.low_memory	 = info->m_memoryLo;

		if(info->m_flags & (1 << 6))
		{
			boot_information.memmap_location = info->m_mmap_addr;
			boot_information.memmap_size	 = info->m_mmap_length;
		}
		else
		{
			boot_information.memmap_location = 0;
			boot_information.memmap_size	 = 0;
		}
	}
	else if(_boot_eax == 0x36d76289) //multiboot2
	{
		assert(false);
	}
	else
	{
		assert(false);
	}
}