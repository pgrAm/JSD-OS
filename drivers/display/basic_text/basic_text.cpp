#include <drivers/portio.h>
#include <kernel/display.h>

//This code covers VGA or MDA text mode

#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

#define VIDEOMEM 0xb8000

static void basic_text_set_cursor_visibility(bool on)
{
	outb(REG_SCREEN_CTRL, 0x0a);

	auto reg_val = inb(REG_SCREEN_DATA);

	outb(REG_SCREEN_DATA, on ? reg_val & ~0x20 : reg_val | 0x20);
}

static size_t basic_text_get_cursor_offset()
{
	outb(REG_SCREEN_CTRL, 0x0f);
	uint16_t offset = inb(REG_SCREEN_DATA);
	outb(REG_SCREEN_CTRL, 0x0e);
	offset += inb(REG_SCREEN_DATA) << 8;
	return offset;
}

static void basic_text_set_cursor_offset(size_t offset)
{
	// cursor LOW port to vga INDEX register
	outb(REG_SCREEN_CTRL, 0x0f);
	outb(REG_SCREEN_DATA, (uint8_t)(offset & 0xFF));

	// cursor HIGH port to vga INDEX register
	outb(REG_SCREEN_CTRL, 0x0E);
	outb(REG_SCREEN_DATA, (uint8_t)((offset >> 8) & 0xFF));
}

static uint8_t* basic_text_get_framebuffer()
{
	return (uint8_t*)VIDEOMEM;
}

static display_mode actual_mode{
	80,
	25,
	80,
	70, //refresh
	16, //bpp
	FORMAT_TEXT_W_ATTRIBUTE,
	DISPLAY_TEXT_MODE,
	80 * 25 * sizeof(uint16_t)
};

static bool basic_text_set_mode(size_t index)
{
	return true;
}

static void basic_text_set_display_offset(size_t offset, bool on_retrace) {}

static display_driver basic_text = 
{
	basic_text_set_mode,

	basic_text_get_framebuffer,

	basic_text_set_display_offset,

	basic_text_get_cursor_offset,
	basic_text_set_cursor_offset,
	basic_text_set_cursor_visibility,

	&actual_mode, 1
};

extern "C" void basic_text_init(void)
{
	display_add_driver(&basic_text, true);
}