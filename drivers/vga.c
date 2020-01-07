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
#include "portio.h"

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

#include "vga_modes.h"

void write_regs(uint8_t* regs)
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
	regs[0x03] |= 0x80;
	regs[0x11] &= ~0x80;
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
uint8_t* get_fb_addr()
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
static void write_font(unsigned char* buf, unsigned font_height)
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

	for (size_t i = 0; i < 256; i++)
	{
		memcpy(get_fb_addr() + i * 32, buf, font_height);
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

#include<stdlib.h>
#include<stdio.h>
#include "../kernel/filesystem.h"
#include "kbrd.h"
#define PSF_MAGIC 0x0436

uint8_t* loadpsf(const char* file)
{
	typedef struct {
		uint16_t magic;		// Magic number
		uint8_t mode;		// PSF font mode
		uint8_t charsize;	// Character size
	} __attribute__((packed)) PSF_font;

	file_stream* f = filesystem_open_file(file, 0);

	if (!f) return NULL;

	PSF_font font;
	filesystem_read_file(&font, sizeof(PSF_font), f);

	size_t size = 256 * font.charsize;
	
	uint8_t* buffer = NULL;

	if (font.magic == PSF_MAGIC)
	{
		buffer = (uint8_t*)malloc(size);
		filesystem_read_file(buffer, size, f);
	}

	filesystem_close_file(f);

	return buffer;
}

uint8_t* font16 = NULL;
uint8_t* font08 = NULL;

bool set_vga_mode(vga_mode* m)
{
	write_regs(m->regs);

	if (m->is_text)
	{
		if (m->char_height == 16 && font16 == NULL)
		{
			font16 = loadpsf("font16.psf");
		}
		if (m->char_height == 8 && font08 == NULL)
		{
			font08 = loadpsf("font08.psf");
		}
		uint8_t* f = m->char_height == 16 ? font16 : font08;
		if (f)
		{
			write_font(f, m->char_height);
		}
	}

	return true;
}

bool set_vga_mode_by_attributes(int cols, int rows, int bpp, bool text)
{
	for (int i = 0; i < NUM_GRAPHICS_MODES; i++)
	{
		vga_mode* m = &available_modes[i];

		if (m->width == cols && m->height == rows && (!!m->is_text == !!text) && 
			(bpp == 0 || m->bits_per_pixel == bpp))
		{
			return set_vga_mode(m);
		}
	}

	return false;
}