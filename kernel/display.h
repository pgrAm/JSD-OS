#ifndef DISPLAY_H
#define DISPLAY_H
#ifdef __cplusplus

#include <string_view>

extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <kernel/syscall.h>
#include <common/display_mode.h>

void print_string_len(const char* str, size_t length);

typedef struct mode_list mode_list;

struct display_driver 
{
	bool (*set_mode)(size_t index);

	uint8_t* (*get_framebuffer)();

	void (*set_display_offset)(size_t offset, bool on_retrace);

	//text mode funcs
	size_t(*get_cursor_offset)();
	void (*set_cursor_offset)(size_t offset);
	void (*set_cursor_visible)(bool visible);

	const display_mode* available_modes;
	size_t num_modes;
};

typedef struct display_driver display_driver;

void display_add_driver(const display_driver* d, bool use_as_default);

SYSCALL_HANDLER int set_cursor_offset(size_t offset);
SYSCALL_HANDLER int set_display_mode(const display_mode* requested, display_mode* actual);
SYSCALL_HANDLER int get_display_mode(int index, display_mode* result);
SYSCALL_HANDLER uint8_t* map_display_memory(void);
SYSCALL_HANDLER int set_display_offset(size_t offset, int on_retrace);

#ifdef __cplusplus
}

void print_string(char c);
void print_string(std::string_view str);

template<typename... Args>
inline void print_strings(Args&&... args)
{
	(print_string(std::forward<Args>(args)), ...);
}


#endif
#endif