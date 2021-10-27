#ifndef VGA_MODES_H
#define VGA_MODES_H
/*****************************************************************************
VGA REGISTER DUMPS FOR VARIOUS TEXT MODES

()=to do
	40x25	(40x30)	40x50	(40x60)
	(45x25)	(45x30)	(45x50)	(45x60)
	80x25	(80x30)	80x50	(80x60)
	(90x25)	90x30	(90x50)	90x60
*****************************************************************************/
uint8_t g_40x25_text[] =
{
	/* MISC */
	0x67,
	/* SEQ */
	0x03, 0x08, 0x03, 0x00, 0x02,
	/* CRTC */
	0x2D, 0x27, 0x28, 0x90, 0x2B, 0xA0, 0xBF, 0x1F,
	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0xA0,
	0x9C, 0x8E, 0x8F, 0x14, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_40x50_text[] =
{
	/* MISC */
	0x67,
	/* SEQ */
	0x03, 0x08, 0x03, 0x00, 0x02,
	/* CRTC */
	0x2D, 0x27, 0x28, 0x90, 0x2B, 0xA0, 0xBF, 0x1F,
	0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x04, 0x60,
	0x9C, 0x8E, 0x8F, 0x14, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_80x25_text[] =
{
	/* MISC */
	0x67,
	/* SEQ */
	0x03, 0x00, 0x03, 0x00, 0x02,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
	0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00
};

uint8_t g_80x50_text[] =
{
	/* MISC */
	0x67,
	/* SEQ */
	0x03, 0x00, 0x03, 0x00, 0x02,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
	0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x01, 0x40,
	0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_90x30_text[] =
{
	/* MISC */
	0xE7,
	/* SEQ */
	0x03, 0x01, 0x03, 0x00, 0x02,
	/* CRTC */
	0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x2D, 0x10, 0xE8, 0x05, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};

uint8_t g_90x60_text[] =
{
	/* MISC */
	0xE7,
	/* SEQ */
	0x03, 0x01, 0x03, 0x00, 0x02,
	/* CRTC */
	0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
	0x00, 0x47, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x2D, 0x08, 0xE8, 0x05, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};
/*****************************************************************************
VGA REGISTER DUMPS FOR VARIOUS GRAPHICS MODES
*****************************************************************************/
uint8_t g_640x480x2[] =
{
	/* MISC */
	0xE3,
	/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x06,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};
/*****************************************************************************
*** NOTE: the mode described by g_320x200x4[]
is different from BIOS mode 05h in two ways:
- Framebuffer is at A000:0000 instead of B800:0000
- Framebuffer is linear (no screwy line-by-line CGA addressing)
*****************************************************************************/
uint8_t g_320x200x4[] =
{
	/* MISC */
	0x63,
	/* SEQ */
	0x03, 0x09, 0x03, 0x00, 0x02,
	/* CRTC */
	0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x02, 0x00,
	0xFF,
	/* AC */
	0x00, 0x13, 0x15, 0x17, 0x02, 0x04, 0x06, 0x07,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x01, 0x00, 0x03, 0x00, 0x00
};

uint8_t g_640x480x16[] =
{
	/* MISC */
	0xE3,
	/* SEQ */
	0x03, 0x01, 0x08, 0x00, 0x06,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};

uint8_t g_720x480x16[] =
{
	/* MISC */
	0xE7,
	/* SEQ */
	0x03, 0x01, 0x08, 0x00, 0x06,
	/* CRTC */
	0x6B, 0x59, 0x5A, 0x82, 0x60, 0x8D, 0x0B, 0x3E,
	0x00, 0x40, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x2D, 0x08, 0xE8, 0x05, 0xE3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x01, 0x00, 0x0F, 0x00, 0x00,
};

uint8_t g_320x200x256[] =
{
	/* MISC */
	0x63,
	/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x0E,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00,	0x00
};

uint8_t g_320x240x256_modex[] =
{
	/* MISC */
	0x63,
	/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x06,
	/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0D, 0x3E,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0xAC, 0xDF, 0x28, 0x00, 0xE7, 0x06, 0xE3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00, 0x00
};

uint8_t g_400x240x256_modex[] =
{
	/* MISC */
	0xe7,
	/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x06,
	/* CRTC */
	0x70, 0x63, 0x64, 0x8f, 0x65, 0x80, 0x0D, 0x3E,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0xAC, 0xDF, 0x32, 0x00, 0xE7, 0x06, 0xE3,
	0xFF,
	/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
	/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00, 0x00
};

typedef struct
{
	size_t char_height;
	uint8_t* regs;
	display_mode mode;
} vga_mode;

#define NUM_GRAPHICS_MODES 14

#include <common/display_mode.h>

vga_mode available_modes[NUM_GRAPHICS_MODES] = {
	{16, g_40x25_text,	{40, 25, 80, 60, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{8,  g_40x50_text,	{40, 50, 80, 60, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{16, g_80x25_text,	{80, 25, 160, 70, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{8,  g_80x50_text,	{80, 50, 160, 70, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{16, g_90x30_text,	{90, 30, 180, 70, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{8,  g_90x60_text,	{90, 60, 180, 70, 16, FORMAT_TEXT_W_ATTRIBUTE, DISPLAY_TEXT_MODE}},
	{0,  g_640x480x2,	{640, 480, 80, 60, 1, FORMAT_GRAYSCALE, DISPLAY_MONO}},
	{0,  g_320x200x4,	{320, 200, 80, 60, 4, FORMAT_INDEXED_BIT_PLANE, DISPLAY_INDEXED}},
	{0,  g_640x480x16,	{640, 480, 80, 60, 4, FORMAT_INDEXED_BIT_PLANE, DISPLAY_INDEXED}},
	{0,  g_720x480x16,	{720, 480, 90, 70, 4, FORMAT_INDEXED_BIT_PLANE, DISPLAY_INDEXED}},
	{0,  g_320x200x256,	{320, 200, 320, 70, 8, FORMAT_INDEXED_LINEAR, DISPLAY_INDEXED}},
	{0,  g_320x240x256_modex, {320, 240, 80, 60, 8, FORMAT_INDEXED_BYTE_PLANE, DISPLAY_INDEXED}},
	{0,  g_400x240x256_modex, {400, 240, 100, 60, 8, FORMAT_INDEXED_BYTE_PLANE, DISPLAY_INDEXED}}
};
#endif