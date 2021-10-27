#include "libx86emu/include/x86emu.h"

#include <stdio.h>
#include <drivers/portio.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/display.h>
#include <kernel/util/hash.h>
#include <kernel/kassert.h>

//#include <drivers/rs232.h>
//uint16_t serial_port;

uintptr_t low_page_begin = 0x7C00;
uintptr_t low_page_end = low_page_begin + 1023;
uint8_t* virtual_bootsector = nullptr;

uintptr_t virtual_stack_end = 0x00080000;
uintptr_t virtual_stack_begin = virtual_stack_end - 1024;
uint8_t* virtual_stack = nullptr;

static std::vector<uintptr_t>* pages_mapped;
static hash_map<uintptr_t, uintptr_t>* mapping_cache;

struct __attribute__((packed)) far_ptr
{
	uint16_t offset;
	uint16_t segment;

	void* access() const
	{
		return (void*)((segment * 0x10) + offset);
	}
};

/*int ser_printf(const char* format, ...)
{
	char buffer[128];
	va_list args;
	va_start(args, format);

	int len = vsnprintf(buffer, 128, format, args);
	rs232_send_string_len(serial_port, buffer, len);

	va_end(args);

	return len;
}*/

static void* translate_virtual_far_ptr(far_ptr t)
{
	uint32_t addr = (uint32_t)t.access();

	if(addr >= low_page_begin && addr <= low_page_end)
	{
		return virtual_bootsector + (addr - low_page_begin);
	}
	if(addr >= virtual_stack_begin && addr <= virtual_stack_end)
	{
		return virtual_stack + (addr - virtual_stack_begin);
	}

	if(memmanager_get_physical(addr) == addr)
	{
		return (void*)addr;
	}

	return nullptr;
}

static void* map_address(uint32_t addr, bool write = false)
{
	if(addr >= low_page_begin && addr <= low_page_end)
	{
		return virtual_bootsector + (addr - low_page_begin);
	}
	if(addr >= virtual_stack_begin && addr <= virtual_stack_end)
	{
		return virtual_stack + (addr - virtual_stack_begin);
	}
	
	//allow touching the BDA
	//if(addr < 0x500)
	//	return (void*)addr;
	//allow touching the EBDA
	//if(addr >= 0x80000 && addr < 0x100000)
	//	return (void*)addr;

	uintptr_t page_addr = addr & ~(PAGE_SIZE - 1);
	uintptr_t page_offset = addr - page_addr;

	if(uintptr_t vaddr; mapping_cache->lookup(page_addr, &vaddr))
	{
		void* ret = (void*)(vaddr + page_offset);
		return ret;
	}

	//if(!write && memmanager_get_physical(page_addr) == page_addr)
	//{
	//	return (void*)addr;
	//}

	//if(write)
	//	ser_printf("writing %X\r\n", addr);
	//else
	//	ser_printf("reading %X\r\n", addr);

	if((page_addr < 0x1000 || (page_addr >= 0x80000 && page_addr < 0x100000)) &&
	   memmanager_get_physical(page_addr) == page_addr)
	{
		//ser_printf("accessing real page %X\r\n", page_addr);
		mapping_cache->insert(page_addr, page_addr);
		return (void*)addr;
	}

	uintptr_t vaddr = (uintptr_t)nullptr;
	if(mapping_cache && !mapping_cache->lookup(page_addr, &vaddr))
	{
		if(physical_memory_allocate_in_range(page_addr, page_addr + PAGE_SIZE,
											 PAGE_SIZE, PAGE_SIZE))
		{
			//ser_printf("mapping %X to %X page\r\n", page_addr, vaddr);
			vaddr = (uintptr_t)memmanager_map_to_new_pages(page_addr, 1, PAGE_PRESENT | PAGE_RW);
			k_assert(vaddr);
		}
		else
		{
			//ser_printf("mapping %X to random page\r\n", addr);
			vaddr = (uintptr_t)memmanager_virtual_alloc(nullptr, 1, PAGE_RW);
		}
		pages_mapped->push_back(vaddr);
		mapping_cache->insert(page_addr, vaddr);
	}
	return (void*)(vaddr + page_offset);
}

template<typename T> static uint32_t mem_read(uint32_t addr)
{
	T val = *(T*)map_address(addr);
	return val;
}

template<typename T> static void mem_write(uint32_t addr, uint32_t val)
{
	T tval = (T)val;
	*(T*)map_address(addr, true) = tval;
}

extern "C" unsigned memio_handler(x86emu_t * emu, u32 addr, u32 * val, unsigned type)
{
	emu->mem->invalid = 0;

	uint32_t bits = type & 0xFF;
	type &= ~0xFF;

	switch(type)
	{
	case X86EMU_MEMIO_X:
	case X86EMU_MEMIO_R:
		switch(bits)
		{
		case X86EMU_MEMIO_8:
			*val = mem_read<u8>(addr);
			break;
		case X86EMU_MEMIO_16:
			*val = mem_read<u16>(addr);
			break;
		case X86EMU_MEMIO_32:
			*val = mem_read<u32>(addr);
			break;
		case X86EMU_MEMIO_8_NOPERM:
			*val = mem_read<u8>(addr);
			break;
		}
		break;
	case X86EMU_MEMIO_W:
		switch(bits)
		{
		case X86EMU_MEMIO_8:
			mem_write<u8>(addr, *val);
			break;
		case X86EMU_MEMIO_16:
			mem_write<u16>(addr, *val);
			break;
		case X86EMU_MEMIO_32:
			mem_write<u32>(addr, *val);
			break;
		case X86EMU_MEMIO_8_NOPERM:
			mem_write<u8>(addr, *val);
			break;
		}
		break;
	case X86EMU_MEMIO_I:
		switch(bits)
		{
		case X86EMU_MEMIO_8:
			*val = inb(addr);
			break;
		case X86EMU_MEMIO_16:
			*val = inw(addr);
			break;
		case X86EMU_MEMIO_32:
			*val = ind(addr);
			break;
		}
		break;
	case X86EMU_MEMIO_O:
		switch(bits)
		{
		case X86EMU_MEMIO_8:
			outb(addr, *val);
			break;
		case X86EMU_MEMIO_16:
			outw(addr, *val);
			break;
		case X86EMU_MEMIO_32:
			outd(addr, *val);
			break;
		}
		break;
	}

	return 0;
}

static uint16_t int10h(uint16_t ax, uint16_t bx, uint16_t cx, uint16_t dx, uint16_t es, uint16_t di)
{ 
	//ser_printf("int 10h; ax = %X\r\n", ax);

	pages_mapped = new std::vector<uintptr_t>();
	mapping_cache = new hash_map<uintptr_t, uintptr_t>();
	x86emu_t* emu = x86emu_new(X86EMU_PERM_RWX, X86EMU_PERM_RWX);
	x86emu_set_memio_handler(emu, &memio_handler);

	x86emu_set_seg_register(emu, emu->x86.R_CS_SEL, 0);
	x86emu_set_seg_register(emu, emu->x86.R_SS_SEL, virtual_stack_end / 0x10);
	x86emu_set_seg_register(emu, emu->x86.R_ES_SEL, es);

	virtual_bootsector[0] = 0x90; //nop
	virtual_bootsector[1] = 0xF4; //hlt
	emu->x86.R_IP = 0x7C00;
	emu->x86.R_AX = ax;
	emu->x86.R_BX = bx;
	emu->x86.R_CX = cx;
	emu->x86.R_DI = di;
	x86emu_intr_raise(emu, 0x10, INTR_TYPE_SOFT, 0);
	x86emu_run(emu, X86EMU_RUN_LOOP);
	x86emu_done(emu);

	for(auto page : *pages_mapped)
	{
		memmanager_free_pages((void*)page, 1);
	}

	delete pages_mapped;
	delete mapping_cache;

	//ser_printf("int 10h; ax = %X completed\r\n", ax);

	return emu->x86.R_AX;
}

enum vesa_memory_model {
	TEXT = 0x00,
	CGA = 0x01,
	HERCULES = 0x02,
	PLANAR = 0x03,
	PACKED = 0x04,
	NONCHAIN = 0x05,
	DIRECT_COLOR = 0x06,
	YUV = 0x07
};

struct __attribute__((packed)) vesa_mode_info
{
	uint16_t attributes;	// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;		// deprecated
	uint8_t window_b;		// deprecated
	uint16_t granularity;	// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;	// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;		// height in pixels
	uint8_t w_char;			//
	uint8_t y_char;			//
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;

	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
};

struct __attribute__((packed)) vesa_info_block
{
	char		vesa_sig[4];		// == "VESA"
	uint16_t	version;			// == 0x0300 for VBE 3.0
	far_ptr		oem_string_ptr;
	uint8_t		capabilities[4];
	far_ptr		video_mode_ptr;
	uint16_t	total_memory;		// as # of 64KB blocks
};

struct vesa_mode
{
	display_mode mode;
	uintptr_t fb_addr;
	size_t index;
	bool is_vesa;
};

int current_mode_index = -1;
std::vector<vesa_mode>* available_modes;

static bool vesa_populate_modes()
{
	uintptr_t block_location = low_page_begin + 0x200;

	uint8_t* const data_ptr = &virtual_bootsector[0x200];

	vesa_info_block* vesa_info = (vesa_info_block*)data_ptr;

	const far_ptr block = {	(uint16_t)(block_location % 0x10), 
							(uint16_t)(block_location / 0x10) };

	//memset(fake_ivt, 0, sizeof(fake_ivt));

	memset(vesa_info, 0, sizeof(vesa_info_block));
	memcpy(vesa_info->vesa_sig, "VBE2", 4);

	if(int10h(0x4f00, 0, 0, 0, block.segment, block.offset) != 0x4f)
	{
		printf("Error setting up VESA BIOS");
		return false;
	}

	if(memcmp(vesa_info->vesa_sig, "VESA", 4))
	{
		return false;
	}
	
	printf("\t%s\n", translate_virtual_far_ptr(vesa_info->oem_string_ptr));

	std::vector<uint16_t> modes;
	uint16_t* mode_array = (uint16_t*)translate_virtual_far_ptr(vesa_info->video_mode_ptr);
	for(size_t i = 0; mode_array[i] != 0xFFFF; ++i)
	{
		modes.push_back(mode_array[i]);
	}

	for(auto & mode : modes) 
	{
		vesa_mode_info* mode_info = (vesa_mode_info*)data_ptr;
		memset(mode_info, 0, sizeof(vesa_mode_info));

		if(int10h(0x4F01, 0, mode, 0, block.segment, block.offset) != 0x4f)
			continue;

		bool is_text = mode_info->memory_model == TEXT;

		if(!is_text)
		{
			// Check if this is a graphics mode with linear frame buffer support
			if((mode_info->attributes & 0x80) != 0x80) continue;

			// Check if this is a packed pixel or direct color mode
			if(mode_info->memory_model != PACKED &&
			   mode_info->memory_model != DIRECT_COLOR) continue;
		}

		vesa_mode d_mode = {
			{
				mode_info->width,
				mode_info->height,
				mode_info->pitch,
				60,
				mode_info->bpp,
				is_text ? FORMAT_TEXT_W_ATTRIBUTE : FORMAT_RGBA,
				is_text ? DISPLAY_TEXT_MODE : DISPLAY_RGB
			},
			mode_info->framebuffer,
			mode,
			true
		};
		available_modes->push_back(d_mode);
	}

	return true;
}
bool vesa_do_mode_switch(uint16_t mode_num, bool vesa) 
{
	if(vesa)
	{
		int10h(0x4F02, mode_num, 0, 0, 0, 0);
	}
	else
	{
		int10h(mode_num, 0, 0, 0, 0, 0);
	}
	//ser_printf("set\r\n");
	return true;
}

static bool vesa_set_mode(display_mode* requested, display_mode* actual)
{
	bool success = current_mode_index != -1 &&
		display_mode_satisfied(requested, &(*available_modes)[current_mode_index].mode);
	if(!success)
	{
		for(int i = 0; i < available_modes->size(); i++)
		{
			auto& m = (*available_modes)[i];

			if(display_mode_satisfied(requested, &m.mode))
			{
				if(success = vesa_do_mode_switch(m.index, m.is_vesa); success)
				{
					//ser_printf("vesa mode set\r\n");
					current_mode_index = i;
					break;
				}
			}
		}
	}

	if(actual != nullptr && current_mode_index != -1)
	{
		*actual = (*available_modes)[current_mode_index].mode;
	}

	return success;
}

static uint8_t* vesa_get_framebuffer()
{
	if(current_mode_index != -1)
	{
		return (uint8_t*)(*available_modes)[current_mode_index].fb_addr;
	}
	return nullptr;
}

static void vesa_set_cursor_visibility(bool on)
{
}

static size_t vesa_get_cursor_offset()
{
	return 0;
}

static void vesa_set_cursor_offset(size_t offset)
{
}

static display_driver vesa_driver =
{
	vesa_set_mode,
	vesa_get_framebuffer,

	vesa_get_cursor_offset,
	vesa_set_cursor_offset,
	vesa_set_cursor_visibility
};


extern "C" void vesa_init()
{
	//serial_port = ((uint16_t*)0x400)[0];
	//rs232_init(serial_port, 9600);

	virtual_bootsector = new uint8_t[low_page_end - low_page_begin];
	virtual_stack = new uint8_t[virtual_stack_end - virtual_stack_begin];

	available_modes = new std::vector<vesa_mode>();

	if(!vesa_populate_modes())
	{
		delete[] virtual_stack;
		delete[] virtual_bootsector;
		delete available_modes;
		return;
	}

	display_mode requested = {
		80, 25,
		0,
		0,
		0,
		FORMAT_DONT_CARE,
		DISPLAY_TEXT_MODE
	};

	if(!vesa_set_mode(&requested, nullptr))
	{
		//fall back and add 1 vga text mode
		vesa_mode d_mode = {
		{ 80, 25, 80, 70, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE },
		0xB8000, 0x03, false
		};
		available_modes->push_back(d_mode);
		vesa_set_mode(&requested, nullptr);
	}

	display_add_driver(&vesa_driver, true);
}