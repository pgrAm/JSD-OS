#include <sys/syscalls.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <new>

#include <terminal/terminal.h>

struct text_char {
	char ch;
	uint8_t attr;
};

static constexpr uint8_t vt100_colors[8] =
{
	0x00, 0x04, 0x02, 0x06, 0x01, 0x05, 0x03, 0x07
};

struct shared_terminal_state
{
	size_t width = 0;
	size_t height = 0;
	size_t total_size = 0;
	size_t last_row_start = 0;
	size_t cursor_pos = 0;
	uint8_t is_bright = 0x1;
	uint8_t fgr_color = 0x07;
	uint8_t bgr_color = 0x0;
};

static int set_display_mode(size_t width, size_t height, shared_terminal_state& state)
{
	display_mode requested = {
		width, height,
		0,0,0,
		FORMAT_DONT_CARE,
		DISPLAY_TEXT_MODE
	};
	display_mode actual;

	int err = set_display_mode(&requested, &actual);

	if(actual.flags & DISPLAY_TEXT_MODE)
	{
		state.width = actual.width;
		state.height = actual.height;
		state.total_size = actual.width * actual.height;
		state.last_row_start = actual.width * (actual.height - 1);
	}

	return err;
}

static shared_terminal_state& get_shared_state(uintptr_t buf_handle)
{
	return *(shared_terminal_state*)map_shared_buffer(buf_handle, sizeof(shared_terminal_state), PAGE_RW);
}

static shared_terminal_state& make_shared_state(uintptr_t buf_handle, size_t width, size_t height)
{
	auto& buf = get_shared_state(buf_handle);

	new (&buf) shared_terminal_state{};

	set_display_mode(width, height, buf);

	return buf;
}

class terminal::impl
{
public:
	impl(uintptr_t handle)
		: buf_handle{handle}
		, m_state{get_shared_state(handle)}
		, m_screen_ptr{(text_char*)map_display_memory()}
	{}

	impl(size_t width, size_t height, uintptr_t handle)
		: buf_handle{handle}
		, m_state{make_shared_state(handle, width, height)}
		, m_screen_ptr{(text_char*)map_display_memory()}
	{}

	~impl()
	{
		unmap_pages(&m_state, (sizeof(shared_terminal_state) + (PAGE_SIZE - 1)) / PAGE_SIZE);
		close_shared_buffer(buf_handle);
	}

	uintptr_t buf_handle;
	shared_terminal_state& m_state;

	text_char* m_screen_ptr;
	uint8_t m_color = 0x0f;
	text_char m_clear_val = {'\0', m_color};

	int handle_escape_sequence(const char* sequence);
	int handle_char(char source, text_char* dest, size_t pos);

	void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright)
	{
		m_state.fgr_color = fgr;
		m_state.bgr_color = bgr;
		m_state.is_bright = bright;

		m_color = ((bgr & 0x0f) << 4) | ((fgr + (bright & 0x01) * 8) & 0x0f);
		m_clear_val = {'\0', m_color};
	}

	void set_cursor_pos(size_t offset)
	{
		m_state.cursor_pos = offset;
		set_display_cursor(offset);
	}

	void set_cursor_pos(size_t x, size_t y)
	{
		set_cursor_pos((y * m_state.height) + x);
	}

	void clear()
	{
		auto begin = m_screen_ptr;
		auto end = begin + m_state.total_size;

		std::fill(begin, end, m_clear_val);

		set_cursor_pos(0);
	}
};

terminal::terminal(size_t width, size_t height, const char* name, size_t name_len) 
	: m_impl{ new impl(width, height, create_shared_buffer(name, name_len, sizeof(shared_terminal_state)))}
{}

terminal::terminal(const char* name, size_t name_len)
	: m_impl { new impl(open_shared_buffer(name, name_len))}
{}

terminal::~terminal()
{
	delete m_impl;
}

void terminal::set_cursor_pos(size_t x, size_t y)
{
	m_impl->set_cursor_pos(x, y);
}

void terminal::set_cursor_pos(size_t offset)
{
	m_impl->set_cursor_pos(offset);
}

void terminal::set_color(uint8_t bgr, uint8_t fgr, uint8_t bright)
{
	m_impl->set_color(bgr, fgr, bright);
}

size_t terminal::cursor_pos()
{
	return m_impl->m_state.cursor_pos;
}

void terminal::delete_chars(size_t num)
{
	auto cursor = cursor_pos();

	auto end = m_impl->m_screen_ptr + cursor;
	auto begin = end - num;

	std::fill(begin, end, m_impl->m_clear_val);

	set_cursor_pos(cursor - num);
}

void terminal::print_string(const char* str, size_t length)
{
	size_t output_position = cursor_pos();

	auto buffer = m_impl->m_screen_ptr;
	auto ptr = buffer + output_position;

	const char* currentchar = str;
	const char* lastchar = str + length;

	for(; currentchar < lastchar; currentchar++)
	{
		if(*currentchar == '\x1b') //ANSI escape sequence
		{
			currentchar += m_impl->handle_escape_sequence(currentchar);
		}
		else
		{
			output_position += m_impl->handle_char(*currentchar, ptr, output_position);
		}

		if(output_position >= m_impl->m_state.total_size)
		{
			scroll_up();
			output_position = m_impl->m_state.last_row_start;
		}
		ptr = buffer + output_position;
	}

	set_cursor_pos(output_position);
}

void terminal::clear()
{
	m_impl->clear();
}

void terminal::clear_row(size_t row)
{
	auto begin = m_impl->m_screen_ptr + row * m_impl->m_state.width;
	auto end = begin + m_impl->m_state.width;
	std::fill(begin, end, m_impl->m_clear_val);
}

void terminal::scroll_up()
{
	auto dst = m_impl->m_screen_ptr;
	auto src = dst + m_impl->m_state.width;

	memcpy(dst, src, (m_impl->m_state.total_size - m_impl->m_state.width) * sizeof(text_char));
	set_cursor_pos(m_impl->m_state.height - 1, 0);
	clear_row(m_impl->m_state.height - 1);
}

int terminal::set_mode(size_t width, size_t height)
{
	auto err = set_display_mode(width, height, m_impl->m_state);

	m_impl->m_screen_ptr = (text_char*)map_display_memory();

	return err;
}

size_t terminal::width()
{
	return m_impl->m_state.width;
}

size_t terminal::height()
{
	return m_impl->m_state.height;
}

int terminal::impl::handle_escape_sequence(const char* sequence)
{
	static uint16_t cursorstore = 0;

	char* seq = (char*)sequence;

	seq++;

	if(*seq == '[') //CSI control code
	{
		size_t argnum = 0;

		unsigned int args[4] = {0, 0, 0, 0};
		do
		{
			seq++;
			while(isdigit(*seq))
			{
				args[argnum] = args[argnum] * 10 + *seq - '0';
				seq++;
			}
			argnum++;
		} while(*seq == ';');

		unsigned int n = args[0];

		switch(*seq)
		{
		case 'J': //clear screen
			if(n == 2)
			{
				clear();
			}
			break;
		case 'H': //move cursor
			set_cursor_pos(args[0] - 1, args[1] - 1);
			break;
		case 'm': //SGR
			for(int i = 0; i < argnum; i++)
			{
				int comm = args[i];

				if(comm == 1)
				{
					set_color(m_state.bgr_color, m_state.fgr_color, 1);
				}
				else if(comm == 22)
				{
					set_color(m_state.bgr_color, m_state.fgr_color, 0);
				}
				else if(comm >= 30 && comm <= 37) //change text color
				{
					set_color(m_state.bgr_color,
							  vt100_colors[comm - 30], m_state.is_bright);
				}
				else if(comm >= 40 && comm <= 47) //change background color
				{
					set_color(vt100_colors[comm - 40],
							  m_state.fgr_color, m_state.is_bright);
				}
			}
			break;
		case 's':
		{
			cursorstore = m_state.cursor_pos; //save the current cursor position
		}
		break;
		case 'u':
		{
			m_state.cursor_pos = cursorstore; //restore the cursor
		}
		break;
		default:
			break;
		}
	}

	return seq - sequence;
}

int terminal::impl::handle_char(char source, text_char* dest, size_t pos)
{
	switch(source)
	{
	case '\r':
		return 0;
	case '\n':
		return m_state.width - (pos % m_state.width);
		break;
	case '\t':
		return tab_size - (pos % tab_size);
	case '\b':
		dest->ch = '\0';
		return -1;
	default:
		*dest = {source, m_color};
		return 1;
	}
}