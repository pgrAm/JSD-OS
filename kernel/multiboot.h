#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

typedef struct
{
	uint32_t begin;
	uint32_t end;
}  __attribute__((packed)) multiboot_modules;

typedef struct 
{
	uint32_t	m_flags;
	uint32_t	m_memoryLo;
	uint32_t	m_memoryHi;
	uint32_t	m_bootDevice;
	uint32_t	m_cmdLine;
	uint32_t	m_modsCount;
	uint32_t	m_modsAddr;
	uint32_t	m_syms0;
	uint32_t	m_syms1;
	uint32_t	m_syms2;
	uint32_t	m_mmap_length;
	uint32_t	m_mmap_addr;
	uint32_t	m_drives_length;
	uint32_t	m_drives_addr;
	uint32_t	m_config_table;
	uint32_t	m_bootloader_name;
	uint32_t	m_apm_table;
	uint32_t	m_vbe_control_info;
	uint16_t	m_vbe_mode_info;
	uint16_t	m_vbe_mode;
	uint16_t	m_vbe_interface_off;
	uint16_t	m_vbe_interface_len;
} __attribute__((packed)) multiboot_info;

#endif