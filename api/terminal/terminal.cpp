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

struct buf_handle_data
{
	uintptr_t handle;
	bool needs_init;
};

struct shared_terminal_state
{
	display_mode current_mode;

	size_t width = 0;
	size_t height = 0;
	size_t total_size = 0;
	size_t last_row_start = 0;
	size_t cursor_pos = 0;
	uint8_t is_bright = 0x1;
	uint8_t fgr_color = 0x07;
	uint8_t bgr_color = 0x0;

	uint8_t color = 0x0f;
	text_char clear_val = {'\0', color};
};

static int set_display_mode(size_t width, size_t height, shared_terminal_state& state)
{
	display_mode requested = {
		width, height,
		0,0,0,
		FORMAT_DONT_CARE,
		DISPLAY_TEXT_MODE
	};

	int err = set_display_mode(&requested, &state.current_mode);

	if(state.current_mode.flags & DISPLAY_TEXT_MODE)
	{
		state.width = state.current_mode.width;
		state.height = state.current_mode.height;
		state.total_size = state.width * state.height;
		state.last_row_start = state.width * (state.height - 1);
	}

	return err;
}

static buf_handle_data get_buf_handle(const char* name, size_t name_len, terminal::open_mode mode)
{
	if(mode != terminal::OPEN_EXISTING)
	{
		auto buf = create_shared_buffer(name, name_len, sizeof(shared_terminal_state));
		if(buf || mode == terminal::CREATE_NEW)
		{
			return{buf, true};
		}
	}

	return{open_shared_buffer(name, name_len), false};
}

static shared_terminal_state& get_shared_state(uintptr_t buf_handle, size_t width, size_t height, bool init)
{
	auto& buf = *(shared_terminal_state*)map_shared_buffer(buf_handle, 
														   sizeof(shared_terminal_state), PAGE_RW);
	if(init)
	{
		new (&buf) shared_terminal_state{};

		set_display_mode(width, height, buf);
	}

	return buf;
}

class terminal::impl
{
public:
	impl(buf_handle_data h, size_t width, size_t height)
		: buf_handle{h.handle}
		, m_state{get_shared_state(h.handle, width, height, h.needs_init)}
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

	int handle_escape_sequence(const char* sequence);
	int handle_char(char source, text_char* dest, size_t pos);

	void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright)
	{
		m_state.fgr_color = fgr;
		m_state.bgr_color = bgr;
		m_state.is_bright = bright;

		m_state.color = ((bgr & 0x0f) << 4) | ((fgr + (bright & 0x01) * 8) & 0x0f);
		m_state.clear_val = {'\0', m_state.color};
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

		std::fill(begin, end, m_state.clear_val);

		set_cursor_pos(0);
	}
};

terminal::terminal(const char* name, size_t name_len, size_t width, size_t height, open_mode mode)
	: m_impl{new impl(get_buf_handle(name, name_len, mode), width, height)}
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

uint8_t* terminal::get_underlying_buffer()
{
	return (uint8_t*)m_impl->m_screen_ptr;
}

void terminal::delete_chars(size_t num)
{
	auto cursor = cursor_pos();

	auto end = m_impl->m_screen_ptr + cursor;
	auto begin = end - num;

	std::fill(begin, end, m_impl->m_state.clear_val);

	set_cursor_pos(cursor - num);
}

void terminal::print(char c)
{
	print(&c, 1);
}

void terminal::print(const char* str, size_t length)
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
	std::fill(begin, end, m_impl->m_state.clear_val);
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
		*dest = {source, m_state.color};
		return 1;
	}
}