#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include <algorithm>
#include <new>

#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>

#include <kernel/locks.h>
#include <kernel/task.h>
#include <drivers/portio.h>

#include <vector>	

#include "display.h"

struct text_char {
	char ch;
	uint8_t attr;
};

constexpr text_char clearval = {'\0', 0x0f};

const display_driver* default_driver = nullptr;
std::vector<display_driver*> display_drivers;

display_mode current_mode;

class kernel_terminal
{
	static text_char* get_mapped_frame_buffer(const display_driver* driver, size_t size)
	{
		uintptr_t phys_addr = (uintptr_t)driver->get_framebuffer();

		if(memmanager_get_physical(phys_addr) != phys_addr)
		{
			auto num_pages = memmanager_minimum_pages(size * sizeof(text_char));
			return (text_char*)memmanager_map_to_new_pages(phys_addr, num_pages,
														   PAGE_PRESENT | PAGE_RW);
		}

		return (text_char*)phys_addr;
	}

public:
	static const int tab_size = 5;

	kernel_terminal(const display_driver* driver, size_t rows, size_t cols) :
		m_display(driver),
		m_screen_ptr((text_char*)get_mapped_frame_buffer(driver, rows* cols)),
		m_num_rows(rows),
		m_num_cols(cols),
		m_total_size(rows*cols),
		m_last_row_start((rows - 1)* cols),
		m_cursorpos(driver->get_cursor_offset())
	{}

	kernel_terminal(const kernel_terminal&) = delete;
	~kernel_terminal() {}

	void set_cursor_position(size_t row, size_t col)
	{
		set_cursor_offset((row * m_num_rows) + col);
	}

	void set_cursor_offset(size_t offset)
	{
		m_cursorpos = offset;
		m_display->set_cursor_offset(offset);
	}

	size_t cursor_pos()
	{
		return m_cursorpos;
	}

	uint8_t* get_framebuffer()
	{
		return m_display->get_framebuffer();
	}

	text_char* get_text_buffer()
	{
		return m_screen_ptr;
	}

	void delete_chars(size_t num)
	{
		auto cursor = cursor_pos();

		auto begin = get_text_buffer() + cursor;
		auto end = begin - num;

		std::fill(begin, end, clearval);

		set_cursor_offset(cursor - num);
	}

	void print_chars(const char* str, size_t length)
	{
		size_t output_position = cursor_pos();

		auto buffer = get_text_buffer();
		auto ptr = buffer + output_position;

		const char* currentchar = str;
		const char* lastchar = str + length;

		for(; currentchar < lastchar; currentchar++)
		{
			output_position += handle_char(*currentchar, ptr, output_position);

			if(output_position >= m_total_size)
			{
				scroll_up();
				output_position = m_last_row_start;
			}
			ptr = buffer + output_position;
		}

		set_cursor_offset(output_position);
	}

	void clear()
	{
		auto begin = get_text_buffer();
		auto end = begin + m_total_size;

		std::fill(begin, end, clearval);

		set_cursor_offset(0);
	}

	void clear_row(size_t row)
	{
		auto begin = get_text_buffer() + row * m_num_cols;
		auto end = begin + m_num_cols;
		std::fill(begin, end, clearval);
	}

	void scroll_up()
	{
		auto dst = get_text_buffer();
		auto src = dst + m_num_cols;

		memcpy(dst, src, (m_total_size - m_num_cols) * sizeof(text_char));
		set_cursor_position(m_num_rows - 1, 0);
		clear_row(m_num_rows - 1);
	}

	int handle_char(char source, text_char* dest, size_t pos)
	{
		switch(source)
		{
		case '\n':
			return m_num_cols - (pos % m_num_cols);
			break;
		case '\t':
			return tab_size - (pos % tab_size);
		case '\b':
			dest->ch = '\0';
			return -1;
		default:
			dest->ch = source;
			return 1;
		}
	}

	bool still_valid(const display_driver* d, size_t width, size_t height)
	{
		return (d == m_display) && (width == m_num_cols) && (height == m_num_rows);
	}
private:

	const display_driver* m_display;

	text_char* m_screen_ptr; //non owning ptr
	size_t m_num_rows;
	size_t m_num_cols;
	size_t m_total_size;
	size_t m_last_row_start;
	volatile size_t m_cursorpos;
};

uint8_t k_terminal_mem[sizeof(kernel_terminal)];
kernel_terminal* k_terminal = nullptr;

SYSCALL_HANDLER int set_cursor_offset(int offset)
{
	default_driver->set_cursor_offset(offset);
	return 0;
}

static void initialize_terminal(int col, int row)
{
	//size_t p = 0;

	if(k_terminal)
	{
		if(k_terminal->still_valid(default_driver, col, row))
		{
			return;
		}
		k_terminal->~kernel_terminal();
	}
	k_terminal = new (k_terminal_mem) kernel_terminal(default_driver, row, col);
	k_terminal->clear();
}

void print_string(std::string_view str)
{
	print_string_len(str.data(), str.size());
}

void print_string_len(const char* str, size_t length)
{
	if(k_terminal)
	{
		k_terminal->print_chars(str, length);
	}
}

void display_add_driver(const display_driver* d, bool use_as_default)
{
	//display_drivers.push_back(d);
	if(use_as_default)
	{
		display_mode requested = {
			80, 25,
			0,
			0,
			0,
			FORMAT_DONT_CARE,
			DISPLAY_TEXT_MODE,
			0
		};

		current_mode.format = DISPLAY_MODE_INVALID;
		default_driver = d;
		set_display_mode(&requested, nullptr);
	}
}

static bool display_mode_satisfied(const display_mode& requested, const display_mode& actual)
{
	if(actual.format == DISPLAY_MODE_INVALID)
	{
		return false;
	}

	return	(requested.bpp		== 0	|| requested.bpp		== actual.bpp) &&
			(requested.width	== 0	|| requested.width		== actual.width) &&
			(requested.height	== 0	|| requested.height		== actual.height) &&
			(requested.pitch	== 0	|| requested.pitch		== actual.pitch) &&
			(requested.refresh	== 0	|| requested.refresh	== actual.refresh) &&
			(requested.flags	== 0	|| (requested.flags & actual.flags) == requested.flags) &&
			(requested.format	== 0	|| requested.format == actual.format) &&
			(requested.buffer_size == 0 || requested.buffer_size <= actual.buffer_size);
}

SYSCALL_HANDLER int get_display_mode(int index, display_mode* result)
{
	if(result == nullptr)
	{
		return -1;
	}

	if(index == -1)
	{
		*result = current_mode;
		return 0;
	}

	if(index >= 0 && index < default_driver->num_modes)
	{
		*result = default_driver->available_modes[index];

		return 0;
	}
	return -1;
}

SYSCALL_HANDLER int set_display_mode(const display_mode* requested, display_mode* actual)
{
	if(this_task_is_active())
	{
		bool success = !requested || display_mode_satisfied(*requested, current_mode);

		if(!success)
		{
			for(size_t i = 0; i < default_driver->num_modes; i++)
			{
				auto& mode = default_driver->available_modes[i];

				if(display_mode_satisfied(*requested, mode))
				{
					success = default_driver->set_mode(i);
					if(success)
					{
						current_mode = mode;
						break;
					}
				}
			}
		}

		if(success)
		{
			if(current_mode.flags & DISPLAY_TEXT_MODE)
			{
				initialize_terminal(current_mode.width, current_mode.height);
			}
		}

		if(actual != nullptr)
		{
			*actual = current_mode;
		}

		return success ? 0 : -1;
	}

	return -1;
}

SYSCALL_HANDLER int set_display_offset(size_t offset, int on_retrace)
{
	default_driver->set_display_offset(offset, !!on_retrace);
	return 0;
}

SYSCALL_HANDLER uint8_t* map_display_memory(void)
{
	if(this_task_is_active())
	{
		auto num_pages = memmanager_minimum_pages(current_mode.buffer_size);
		auto buf = default_driver->get_framebuffer();

		return (uint8_t*)memmanager_map_to_new_pages((uintptr_t)buf, num_pages,
													 PAGE_USER | PAGE_PRESENT | PAGE_RW);
	}
	return nullptr;
}