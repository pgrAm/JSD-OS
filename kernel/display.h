#ifndef DISPLAY_H
#define DISPLAY_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/syscall.h>
#include <common/display_mode.h>

void set_cursor_visibility(bool on);

void print_string_len(const char* str, size_t length);

void clear_screen();

void clear_row(uint16_t row);

struct display_driver 
{
	bool (*set_mode)(display_mode* requested, display_mode* actual);
	bool (*get_mode)(size_t index, display_mode* actual);

	uint8_t* (*get_framebuffer)();

	void (*set_display_offset)(size_t offset, bool on_retrace);

	//text mode funcs
	size_t(*get_cursor_offset)();
	void (*set_cursor_offset)(size_t offset);
	void (*set_cursor_visible)(bool visible);
};

typedef struct display_driver display_driver;

void display_add_driver(display_driver* d, bool use_as_default);

bool display_mode_satisfied(display_mode* requested, display_mode* actual);

SYSCALL_HANDLER int set_cursor_offset(int offset);
SYSCALL_HANDLER int set_display_mode(display_mode* requested, display_mode* actual);
SYSCALL_HANDLER int get_display_mode(int index, display_mode* result);
SYSCALL_HANDLER uint8_t* map_display_memory(void);
SYSCALL_HANDLER int set_display_offset(size_t offset, int on_retrace);

#ifdef __cplusplus
}
#endif
#endif