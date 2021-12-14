/****************************************************************************
Sets VGA-compatible video modes without using the BIOS
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

To do:
- more registers dumps, for various text modes and ModeX
- flesh out code to support SVGA chips?
- do something with 16- and 256-color palettes?
*****************************************************************************/
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <kernel/filesystem.h>
#include <kernel/memorymanager.h>
#include <kernel/display.h>
#include <kernel/kassert.h>

#include <drivers/portio.h>

#define	VGA_AC_INDEX		0x3C0
#define	VGA_AC_WRITE		0x3C0
#define	VGA_AC_READ			0x3C1
#define	VGA_MISC_WRITE		0x3C2
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5
#define	VGA_DAC_READ_INDEX	0x3C7
#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9
#define	VGA_MISC_READ		0x3CC
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF
//COLOR emulation	MONO emulation 
#define VGA_CRTC_INDEX		0x3D4		/* 0x3B4 */
#define VGA_CRTC_DATA		0x3D5		/* 0x3B5 */
#define	VGA_INSTAT_READ		0x3DA

#define	VGA_NUM_SEQ_REGS	5
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		9
#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_REGS		(1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
				VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

#define HIGH_ADDRESS 0x0C
#define LOW_ADDRESS  0x0D

#define INPUT_STATUS_1		0x03da
#define VRETRACE			0x08

#include "vga_modes.h"

static const vga_mode* current_mode = nullptr;

static uint8_t* vga_memory = nullptr;

static void write_regs(const uint8_t* regs)
{
	// write MISCELLANEOUS reg
	outb(VGA_MISC_WRITE, *regs);
	regs++;
	//write SEQUENCER regs
	for (size_t i = 0; i < VGA_NUM_SEQ_REGS; i++)
	{
		outb(VGA_SEQ_INDEX, i);
		outb(VGA_SEQ_DATA, *regs);
		regs++;
	}
	//unlock CRTC registers
	outb(VGA_CRTC_INDEX, 0x03);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
	outb(VGA_CRTC_INDEX, 0x11);
	outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
	//make sure they remain unlocked

	//assert(regs[0x03] & 0x80);
	//assert(!(regs[0x11] & 0x80));

	//write CRTC regs
	for (size_t i = 0; i < VGA_NUM_CRTC_REGS; i++)
	{
		outb(VGA_CRTC_INDEX, i);
		outb(VGA_CRTC_DATA, *regs);
		regs++;
	}
	// write GRAPHICS CONTROLLER regs
	for (size_t i = 0; i < VGA_NUM_GC_REGS; i++)
	{
		outb(VGA_GC_INDEX, i);
		outb(VGA_GC_DATA, *regs);
		regs++;
	}
	// write ATTRIBUTE CONTROLLER regs
	for (size_t i = 0; i < VGA_NUM_AC_REGS; i++)
	{
		(void)inb(VGA_INSTAT_READ);
		outb(VGA_AC_INDEX, i);
		outb(VGA_AC_WRITE, *regs);
		regs++;
	}
	// lock 16-color palette and unblank display
	(void)inb(VGA_INSTAT_READ);
	outb(VGA_AC_INDEX, 0x20);
}

static void set_plane(uint8_t p)
{
	p &= 3;
	uint8_t pmask = 1 << p;
	// set read plane 
	outb(VGA_GC_INDEX, 4);
	outb(VGA_GC_DATA, p);
	// set write plane 
	outb(VGA_SEQ_INDEX, 2);
	outb(VGA_SEQ_DATA, pmask);
}
/****************************************************************************
VGA framebuffer is at 0xA0000, 0xB0000, or 0xB8000
depending on bits in GC 6
*****************************************************************************/
static uint8_t* vga_get_framebuffer()
{
	unsigned seg;

	outb(VGA_GC_INDEX, 6);
	seg = inb(VGA_GC_DATA);
	seg >>= 2;
	seg &= 3;
	switch (seg)
	{
	default:
	case 0:
	case 1:
		return (uint8_t*)0xA0000;
	case 2:
		return (uint8_t*)0xB0000;
	case 3:
		return (uint8_t*)0xB8000;
	}
}

//write font to plane P4 (assuming planes are named P1, P2, P4, P8)
static void write_font(const uint8_t* buf, unsigned font_height)
{
	//save registers
	//set_plane() modifies GC 4 and SEQ 2, so save them as well
	outb(VGA_SEQ_INDEX, 2);
	uint8_t seq2 = inb(VGA_SEQ_DATA);

	outb(VGA_SEQ_INDEX, 4);
	unsigned char seq4 = inb(VGA_SEQ_DATA);
	//turn off even-odd addressing (set flat addressing)
	//assume: chain-4 addressing already off 
	outb(VGA_SEQ_DATA, seq4 | 0x04);

	outb(VGA_GC_INDEX, 4);
	uint8_t gc4 = inb(VGA_GC_DATA);

	outb(VGA_GC_INDEX, 5);
	uint8_t gc5 = inb(VGA_GC_DATA);
	// turn off even-odd addressing
	outb(VGA_GC_DATA, gc5 & ~0x10);

	outb(VGA_GC_INDEX, 6);
	uint8_t gc6 = inb(VGA_GC_DATA);
	// turn off even-odd addressing
	outb(VGA_GC_DATA, gc6 & ~0x02);
	// write font to plane P4
	set_plane(2);
	// write font 0

	//size_t font_offset = font_height == 16 ? 0x0400 : 0;

	auto start = vga_memory + ((uintptr_t)vga_get_framebuffer() - 0xA0000);

	for (size_t i = 0; i < 256; i++)
	{
		memcpy(start + i * 32, buf, font_height);
		buf += font_height;
	}

	// restore registers
	outb(VGA_SEQ_INDEX, 2);
	outb(VGA_SEQ_DATA, seq2);
	outb(VGA_SEQ_INDEX, 4);
	outb(VGA_SEQ_DATA, seq4);
	outb(VGA_GC_INDEX, 4);
	outb(VGA_GC_DATA, gc4);
	outb(VGA_GC_INDEX, 5);
	outb(VGA_GC_DATA, gc5);
	outb(VGA_GC_INDEX, 6);
	outb(VGA_GC_DATA, gc6);
}

#define PSF_MAGIC 0x0436

static uint8_t* loadpsf(std::string_view file)
{
	typedef struct {
		uint16_t magic;		// Magic number
		uint8_t mode;		// PSF font mode
		uint8_t charsize;	// Character size
	} __attribute__((packed)) PSF_font;

	file_stream* f = filesystem_open_file(nullptr, file.data(), file.size(), 0);

	if (!f) return nullptr;

	PSF_font font;
	filesystem_read_file(&font, sizeof(PSF_font), f);

	size_t size = 256 * font.charsize;
	
	uint8_t* buffer = nullptr;

	if (font.magic == PSF_MAGIC)
	{
		buffer = (uint8_t*)malloc(size);
		filesystem_read_file(buffer, size, f);
	}

	filesystem_close_file(f);

	return buffer;
}

static uint8_t* font16 = nullptr;
static uint8_t* font08 = nullptr;

static bool vga_do_mode_switch(const vga_mode* m)
{
	write_regs(m->regs);

	if (m->mode.flags & DISPLAY_TEXT_MODE)
	{
		if (m->char_height == 16 && font16 == nullptr)
		{
			font16 = loadpsf("font16.psf");
		}
		if (m->char_height == 8 && font08 == nullptr)
		{
			font08 = loadpsf("font08.psf");
		}
		uint8_t* f = m->char_height == 16 ? font16 : font08;
		if (f)
		{
			write_font(f, m->char_height);
		}
	}

	current_mode = m;

	return true;
}

static void vga_set_cursor_visibility(bool on)
{
	outb(VGA_CRTC_INDEX, 0x0a);

	auto reg_val = inb(VGA_CRTC_DATA);

	outb(VGA_CRTC_INDEX, on ? reg_val & ~0x20 : reg_val | 0x20);
}

static size_t vga_get_cursor_offset()
{
	outb(VGA_CRTC_INDEX, 0x0f);
	uint16_t offset = inb(VGA_CRTC_DATA);
	outb(VGA_CRTC_INDEX, 0x0e);
	offset += inb(VGA_CRTC_DATA) << 8;
	return offset;
}

static void vga_set_cursor_offset(size_t offset)
{
	// cursor LOW port to vga INDEX register
	outb(VGA_CRTC_INDEX, 0x0f);
	outb(VGA_CRTC_DATA, (uint8_t)(offset & 0xFF));

	// cursor HIGH port to vga INDEX register
	outb(VGA_CRTC_INDEX, 0x0E);
	outb(VGA_CRTC_DATA, (uint8_t)((offset >> 8) & 0xFF));
}

static bool vga_get_mode(size_t index, display_mode* result)
{
	if(index < NUM_GRAPHICS_MODES)
	{
		*result = available_modes[index].mode;
		return true;
	}
	return false;
}

static bool vga_set_mode(display_mode* requested, display_mode* actual)
{
	bool success =	current_mode != nullptr && 
					display_mode_satisfied(requested, &current_mode->mode);
	if(!success)
	{
		for(int i = 0; i < NUM_GRAPHICS_MODES; i++)
		{
			const vga_mode* m = &available_modes[i];

			if(display_mode_satisfied(requested, &m->mode))
			{
				success = vga_do_mode_switch(m);
			}
		}
	}

	if(actual != nullptr && current_mode != nullptr)
	{
		*actual = current_mode->mode;
	}
	return success;
}

static void vga_set_display_offset(size_t offset, bool on_retrace)
{
	if(on_retrace)
	{
		while(!(inb(INPUT_STATUS_1) & VRETRACE));
		while((inb(INPUT_STATUS_1) & VRETRACE));
	}

	outw(VGA_CRTC_INDEX, HIGH_ADDRESS | (offset & 0xff00));
	outw(VGA_CRTC_INDEX, LOW_ADDRESS | (offset << 8));
}

static display_driver vga_driver =
{
	vga_set_mode,
	vga_get_mode,
	vga_get_framebuffer,

	vga_set_display_offset,

	vga_get_cursor_offset,
	vga_set_cursor_offset,
	vga_set_cursor_visibility
};

extern "C" void vga_init(void)
{
	uintptr_t begin = 0xA0000;
	uintptr_t memory_size = 256 * 1024; //256k

	auto num_pages = memmanager_minimum_pages(memory_size);

	vga_memory = (uint8_t*)memmanager_map_to_new_pages(begin, num_pages, PAGE_PRESENT | PAGE_RW);

	k_assert(vga_memory);

	display_mode requested = {
		80, 25,
		0,
		0,
		0,
		FORMAT_DONT_CARE,
		DISPLAY_TEXT_MODE
	};
	vga_set_mode(&requested, nullptr);
	display_add_driver(&vga_driver, true);
}