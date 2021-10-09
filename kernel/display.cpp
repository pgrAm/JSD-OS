#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include <algorithm>

#include <kernel/memorymanager.h>
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

display_driver* default_driver = nullptr;
std::vector<display_driver*> display_drivers;

class kernel_terminal
{
public:
	static const int tab_size = 5;

	kernel_terminal(display_driver* driver, size_t rows, size_t cols) :
		m_display(driver),
		m_screen_ptr((text_char*)driver->get_framebuffer()),
		m_num_rows(rows),
		m_num_cols(cols),
		m_total_size(rows*cols),
		m_cursorpos(driver->get_cursor_offset())
	{
		m_screen_ptr[(rows * cols) - 2].ch = 'A';
	}

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

		auto row_start = (m_num_rows - 1) * m_num_cols;

		for(; currentchar < lastchar; currentchar++)
		{
			output_position += handle_char(*currentchar, ptr, output_position);

			if(output_position >= m_total_size)
			{
				scroll_up();
				output_position = row_start;
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

		memcpy(dst, src, m_total_size * sizeof(text_char));
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
private:

	display_driver* m_display;

	text_char* m_screen_ptr; //non owning ptr
	size_t m_num_rows;
	size_t m_num_cols;
	size_t m_total_size;
	volatile size_t m_cursorpos;
};

kernel_terminal* k_terminal = nullptr;

SYSCALL_HANDLER int set_cursor_offset(int offset)
{ 
	default_driver->set_cursor_offset(offset);
	return 0;
}

static void initialize_terminal(int col, int row)
{
	if(k_terminal) 
	{
		delete k_terminal;
	}
	k_terminal = new kernel_terminal(default_driver, row, col);
}

void set_cursor_visibility(bool on)
{
	default_driver->set_cursor_visible(on);
}

void print_string_len(const char* str, size_t length)
{
	if(k_terminal)
	{
		k_terminal->print_chars(str, length);
	}
}

void clear_screen()
{
	if(k_terminal)
	{
		k_terminal->clear();
	}
}

void display_add_driver(display_driver* d, bool use_as_default)
{
	display_drivers.push_back(d);
	if(use_as_default)
	{
		default_driver = d;
		set_display_mode(nullptr, nullptr);
	}
}

bool display_mode_satisfied(display_mode* requested, display_mode* actual)
{
	if(requested == nullptr)
	{
		return true;
	}
	if(actual == nullptr)
	{
		return false;
	}

	return	(requested->bpp == 0	|| requested->bpp	== actual->bpp) &&
			(requested->width == 0	|| requested->width == actual->width) &&
			(requested->height == 0 || requested->height == actual->height) &&
			(requested->pitch == 0	|| requested->pitch == actual->pitch) &&
			(requested->refresh == 0 || requested->refresh == actual->refresh) &&
			(requested->flags == 0	|| (requested->flags & actual->flags) == requested->flags) &&
			(requested->format == 0 || requested->format == actual->format);
}

SYSCALL_HANDLER int set_display_mode(display_mode* requested, display_mode* actual)
{
	if (this_task_is_active())
	{
		display_mode got;
		if(actual == nullptr)
		{
			actual = &got;
		}

		bool success = default_driver->set_mode(requested, actual);

		initialize_terminal(actual->width, actual->height);

		return success ? 0 : -1;
	}
	return -1;
}

SYSCALL_HANDLER uint8_t* map_display_memory(void)
{
	if (this_task_is_active())
	{
		return (uint8_t*)memmanager_map_to_new_pages((uint32_t)default_driver->get_framebuffer(), 0x10000 / PAGE_SIZE, PAGE_USER | PAGE_PRESENT | PAGE_RW);
	}
	return nullptr;
}