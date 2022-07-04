#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <string>
#include <string_view>
#include <memory>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef WIN32
#	include <shell/util.h>
#	include <sys/syscalls.h>
#	include <terminal/terminal.h>
#	include <keyboard.h>
terminal s_term{"terminal_1"};

#else
#	define NOMINMAX

#	include <Windows.h>

char get_ascii_from_vk(int key)
{
	BYTE keys[256];
	GetKeyboardState(&keys[0]);
	WORD Char;
	keys[VK_SHIFT]	 = GetAsyncKeyState(VK_SHIFT) ? 0x80 : 0;

	const auto scancode = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);

	if(ToAscii(key, scancode, &keys[0], &Char, 0) == 1) return Char;
	return 0;
}

#define VK_RCTRL VK_RCONTROL
#define VK_LCTRL VK_LCONTROL
#define VK_RALT VK_RMENU
#define VK_LALT VK_LMENU
#define VK_S 0x53

#include <iostream>
#include <locale>

enum event_type
{
	BUTTON_DOWN = 0,
	BUTTON_UP,
	KEY_DOWN,
	KEY_UP,
	AXIS_MOTION
};

struct input_event
{
	size_t device_index;
	size_t control_index;
	int data;
	event_type type;
	clock_t time_stamp;
};

int get_input_event(input_event* ev, bool wait)
{
	HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

	DWORD cNumRead;

	INPUT_RECORD e;
	ReadConsoleInput(hConsole, &e, 1, &cNumRead);

	if(e.EventType == KEY_EVENT)
	{
		ev->type = e.Event.KeyEvent.bKeyDown ? KEY_DOWN : KEY_UP;
	}
	else
		return -1;

	ev->data = e.Event.KeyEvent.wVirtualKeyCode;
	ev->time_stamp = clock();

	switch(ev->data)
	{
	case VK_SHIFT:	 // converts to VK_LSHIFT or VK_RSHIFT
	case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
	case VK_MENU:	 // converts to VK_LMENU or VK_RMENU
		ev->data = LOWORD(MapVirtualKeyW(e.Event.KeyEvent.wVirtualScanCode,
										 MAPVK_VSC_TO_VK_EX));
		break;
	}

	return 0;
}

#endif

bool control_held = false;
bool alt_held	  = false;

int get_keypress()
{
	while(true)
	{
		input_event e;
		if(get_input_event(&e, true) == 0)
		{
			if(e.type != KEY_DOWN && e.type != KEY_UP) continue;

			if(e.data == VK_RCTRL || e.data == VK_LCTRL)
			{
				control_held = e.type == KEY_DOWN;
			}
			else if(e.data == VK_RALT || e.data == VK_LALT)
			{
				alt_held = e.type == KEY_DOWN;
			}

			if(e.type == KEY_DOWN)
			{
				return e.data;
			}
		}
	}
}

static constexpr std::string_view newline_chars = "\r\n";

struct line
{
	line(std::string_view s, std::string_view nl) : text{s}, ending{nl}
	{
	}

	line(line&&)				 = default;
	line& operator=(line&&)		 = default;
	line(const line&)			 = default;
	line& operator=(const line&) = default;

	std::string_view get_view()
	{
		return std::string_view{text};
	}

	std::string text;
	std::string ending;
};

std::vector<line> line_buffer;

size_t cursor_x = 0, cursor_y = 0, preffered_x = 0;
size_t window_y_offset = 0;
size_t window_x_offset = 0;
size_t term_width = 0, term_height = 0;

std::string current_file_name;

static void set_color(uint8_t bgr, uint8_t fgr)
{
#ifndef WIN32
	s_term.set_ansi_color(bgr, fgr);
#else
	printf("\x1b[44m\x1b[37m");
#endif
}

static size_t get_term_width()
{
#ifndef WIN32
	return s_term.width();
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO data;
	GetConsoleScreenBufferInfo(hConsole, &data);
	return data.srWindow.Right - data.srWindow.Left + 1;
#endif
}

static size_t get_term_height()
{
#ifndef WIN32
	return s_term.height() - 1;
#else
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO data;
	GetConsoleScreenBufferInfo(hConsole, &data);
	return data.srWindow.Bottom - data.srWindow.Top;
#endif
}

static void clear_screen()
{
#ifdef WIN32
	system("cls");
#else
	s_term.clear();
#endif
}


struct rect
{
	size_t x, y, w, h;
};

static void write_clearline(size_t x, size_t y, size_t length)
{
#ifdef WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD coord;
	coord.X = x;
	coord.Y = y;

	DWORD chars;

	FillConsoleOutputCharacter(hConsole, L' ', length, coord, &chars);
#else
	s_term.write_chars_at_pos(x, y, '\0', length);
#endif
}

static void write_at_position(size_t x, size_t y, std::string_view str)
{
#ifdef WIN32
	if(str.empty()) return;

	std::locale::global(std::locale("en_US.utf8"));
	auto& f = std::use_facet<std::codecvt<char, wchar_t, std::mbstate_t>>(
		std::locale());

	std::mbstate_t mb{};
	std::wstring out(str.size() * f.max_length(), '\0');
	const char* from_next;
	wchar_t* to_next;
	f.out(mb, &str[0], &str[0] + str.size(), from_next, &out[0],
		  &out[out.size()], to_next);
	out.resize(to_next - &out[0]);

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD coord;
	coord.X = x;
	coord.Y = y;

	auto size = std::min(coord.X + out.size(), term_width) - coord.X;

	DWORD chars;

	WriteConsoleOutputCharacter(hConsole, out.data(), size, coord, &chars);
#else
	auto width = std::min(x + str.size(), term_width) - x;

	s_term.write_chars_at_pos(x, y, str.substr(0, width));
#endif
}

static void draw_rect(rect dst, rect src)
{
	auto height =
		std::min(std::min(dst.h,
						  (size_t)std::max((int)line_buffer.size() - (int)src.y,
										   0)),
				 term_height);

	for(size_t y = 0; y < height; y++)
	{
		auto line = line_buffer[src.y + y].get_view();

		const auto width =
			std::min(std::min(dst.w,
							  (size_t)std::max((int)line.size() - (int)src.x,
											   0)),
					 term_width);

		if(src.x < line.size())
		{
			write_at_position(dst.x, dst.y + y, line.substr(src.x, width));
		}
		write_clearline(dst.x + width, dst.y + y, dst.w - width);
	}
}

static void do_full_redraw()
{
	draw_rect({0, 0, term_width, term_height},
			  {window_x_offset, window_y_offset});
}

static void toggle_cell_highlighted(size_t x, size_t y)
{
	if(x < term_width && y < term_width)
	{
#ifndef WIN32
		auto buf = s_term.get_underlying_buffer();

		auto coord = (x + y * term_width) * sizeof(uint16_t) + 1;
		buf[coord] = ~buf[coord];
#else
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		DWORD numAttrs;
		WORD attribute;
		COORD coord{(SHORT)(x), (SHORT)(y)};

		ReadConsoleOutputAttribute(hConsole, &attribute, 1, coord, &numAttrs);
		attribute = (~attribute & 0x00FF) | (attribute & 0xFF00);
		WriteConsoleOutputAttribute(hConsole, &attribute, 1, coord, &numAttrs);
#endif
	}
}

static void adjust_window_for_cursor_x()
{
	if(cursor_x < window_x_offset)
	{
		window_x_offset = cursor_x;
	}
	if(cursor_x > window_x_offset + (term_width - 1))
	{
		window_x_offset = cursor_x - (term_width - 1);
	}
}

static void adjust_cursor_for_line()
{
	auto line_size = line_buffer[cursor_y].text.size();

	if(preffered_x > line_size)
		cursor_x = line_size;
	else
		cursor_x = preffered_x;

	adjust_window_for_cursor_x();
}


static void adjust_window_for_cursor_up()
{
	if(cursor_y < window_y_offset)
	{
		window_y_offset = cursor_y;
	}
}

static void adjust_window_for_cursor_down()
{
	if(cursor_y > window_y_offset + (term_height - 1))
	{
		window_y_offset = cursor_y - (term_height - 1);
	}
}

static void move_cursor_up()
{
	if(cursor_y > 0)
	{
		cursor_y--;
		adjust_cursor_for_line();
		adjust_window_for_cursor_up();
	}
}

static void move_cursor_down()
{
	if(cursor_y + 1 < line_buffer.size())
	{
		cursor_y++;
		adjust_cursor_for_line();
		adjust_window_for_cursor_down();
	}
}

static void move_cursor_left()
{
	if(cursor_x < 1)
	{
		if(cursor_y > 0)
		{
			cursor_y--;
			cursor_x = line_buffer[cursor_y].text.size();
			adjust_window_for_cursor_up();
		}
	}
	else
	{
		cursor_x--;
	}
	preffered_x = cursor_x;

	adjust_window_for_cursor_x();
}

static void move_cursor_right()
{
	if(cursor_x + 1 > line_buffer[cursor_y].text.size())
	{
		if(cursor_y + 1 < line_buffer.size())
		{
			cursor_y++;
			cursor_x = 0;
			adjust_window_for_cursor_down();
		}
	}
	else
	{
		cursor_x++;
	}
	preffered_x = cursor_x;

	adjust_window_for_cursor_x();
}

static void redraw_from_line(size_t line)
{
	const size_t y_coord = (line - window_y_offset);
	draw_rect({0, y_coord, term_width, term_height - y_coord}, {0, line});
}

static void insert_blank_line()
{
	write_clearline(0, cursor_y - window_y_offset, term_width);
	redraw_from_line(cursor_y + 1);
}

static void print_rest_of_line()
{
	const auto line = line_buffer[cursor_y].get_view();

	write_at_position(cursor_x - window_x_offset, cursor_y - window_y_offset,
					  line.substr(cursor_x));
}

static void handle_char_insert(char in_char)
{
	if(newline_chars.find(in_char) != newline_chars.npos)
	{
		line_buffer.emplace(line_buffer.begin() + cursor_y + 1,
							line_buffer[cursor_y].get_view().substr(cursor_x),
							line_buffer[cursor_y].ending);
		line_buffer[cursor_y].text.resize(cursor_x);

		write_clearline(cursor_x - window_x_offset, cursor_y - window_y_offset,
						term_width - (cursor_x - window_x_offset));

		cursor_y++;
		cursor_x = 0;

		adjust_window_for_cursor_down();
		adjust_window_for_cursor_x();

		insert_blank_line();
	}
	else
	{
		write_at_position(cursor_x - window_x_offset,
						  cursor_y - window_y_offset, {&in_char, 1});

		auto& line = line_buffer[cursor_y];
		line.text.insert(line.text.cbegin() + cursor_x, in_char);

		move_cursor_right();
	}
	print_rest_of_line();
}

static void merge_lines(size_t dst, size_t src)
{
	line_buffer[dst].text.append(std::move(line_buffer[src].text));
	line_buffer.erase(line_buffer.begin() + src);
}

static void handle_char_delete()
{
	auto& buffer = line_buffer[cursor_y].text;

	if(cursor_x < buffer.size())
	{
		buffer.erase(buffer.begin() + cursor_x);
		write_clearline(cursor_x - window_x_offset, cursor_y - window_y_offset,
						term_width - (cursor_x - window_x_offset));
	}
	else if(cursor_y + 1 < line_buffer.size())
	{
		merge_lines(cursor_y, cursor_y + 1);
		redraw_from_line(cursor_y + 1);
		if(window_x_offset + term_height > line_buffer.size())
		{
			write_clearline(window_x_offset,
							line_buffer.size() - window_y_offset, term_width);
		}
	}
	print_rest_of_line();
}

static void update_coordinate_display()
{
	set_color(7, 0);
	write_clearline(0, term_height, term_width);
	auto coord = std::string{"ln: "} + std::to_string(cursor_y) +
				 " ch: " + std::to_string(cursor_x);

	write_at_position(term_width - coord.size(), term_height, coord);
	set_color(4, 7);
}

static void write_info_message(std::string_view message,
							   std::string_view filename)
{
	set_color(7, 0);
	write_at_position(0, term_height, message);
	write_at_position(message.size(), term_height, filename);
	set_color(4, 7);
}

static void save_file()
{
	if(current_file_name.empty())
	{
		set_color(7, 0);
		write_clearline(0, term_height, term_width);
		std::string_view msg{"Save as: "};
		write_at_position(0, term_height, msg);
		size_t pos = 0;

		while(true)
		{
			const auto key = get_keypress();

			if(key == VK_ESCAPE)
			{
				return;
			}

			const char in_char = get_ascii_from_vk(key);

			if(in_char == '\b')
			{
				if(!current_file_name.empty())
				{
					current_file_name.pop_back();
					write_clearline(msg.size() + --pos, term_height, 1);
				}
			}
			else if(newline_chars.find(in_char) != newline_chars.npos)
			{
				break;
			}
			else if(in_char != '\0')
			{
				current_file_name.push_back(in_char);
				write_at_position(msg.size() + pos, term_height,
								  std::string_view{current_file_name}.substr(
									  pos));
				pos++;
			}
		}
		set_color(4, 7);
	}

	auto f = fopen(current_file_name.c_str(), "wb");
	if(!f)
	{
		write_info_message("Could not save file ", current_file_name);
		return;
	}
	for(auto& line : line_buffer)
	{
		fwrite(line.text.c_str(), 1, line.text.size(), f);
		fwrite(line.ending.c_str(), 1, line.ending.size(), f);
	}
	fclose(f);

	write_info_message("File saved as ", current_file_name);
}

void build_lines_from_raw_str(std::string_view buffer)
{
	size_t first = 0;
	while(first < buffer.size())
	{
		auto second = buffer.find_first_of(newline_chars, first);

		auto next = second;
		line_buffer.emplace_back(buffer.substr(first, second - first),
								 buffer.substr(second,
											   buffer[next++] == '\r' ? 2 : 1));

		if(second == buffer.npos)
		{
			line_buffer.emplace_back(buffer.substr(first, second - first), "");
			break;
		}

		first = next + 1;
	}
}

static bool load_file(std::string_view name)
{
	auto f = fopen(name.data(), "rb");

	if(!f) return false;

	fseek(f, 0, SEEK_END);

	const auto size = ftell(f);

	fseek(f, 0, SEEK_SET);

	std::string buffer;
	buffer.resize(size);
	fread(&buffer[0], 1, size, f);

	fclose(f);

	build_lines_from_raw_str(buffer);

	current_file_name = name;

	return true;
}

static void handle_keypress(int key)
{
	if(control_held)
	{
		if(key == VK_S)
		{
			if(alt_held)
			{
				current_file_name.clear();
			}
			save_file();
		}
		return;
	}

	auto cached_win_x = window_x_offset;
	auto cached_win_y = window_y_offset;

	toggle_cell_highlighted(cursor_x - window_x_offset,
							cursor_y - window_y_offset);

	switch(key)
	{
	case VK_ESCAPE:
		set_color(0, 7);
		clear_screen();
		exit(0);
		break;
	case VK_UP:
		move_cursor_up();
		break;
	case VK_DOWN:
		move_cursor_down();
		break;
	case VK_LEFT:
		move_cursor_left();
		break;
	case VK_RIGHT:
		move_cursor_right();
		break;
	case VK_DELETE:
		handle_char_delete();
		break;
	default:
	{
		const char in_char = get_ascii_from_vk(key);

		if(in_char == '\b')
		{
			if(cursor_x != 0 || cursor_y != 0)
			{
				move_cursor_left();
				handle_char_delete();
			}
		}
		else if(in_char != '\0')
		{
			handle_char_insert(in_char);
		}
	}
	}

	if(window_x_offset != cached_win_x || window_y_offset != cached_win_y)
	{
		do_full_redraw();
	}

	toggle_cell_highlighted(cursor_x - window_x_offset,
							cursor_y - window_y_offset);

	update_coordinate_display();
}

static void handle_input()
{
	handle_keypress(get_keypress());
}

int main(int argc, char** argv)
{
#ifndef WIN32
	set_stdout(
		[](const char* buf, size_t size, void* impl)
		{
			s_term.print(buf, size);
			return size;
		});
#endif

	term_width = get_term_width();
	term_height = get_term_height();

	set_color(4, 7);
	clear_screen();

	update_coordinate_display();

	if(argc >= 2)
	{
		if(!load_file(argv[1]))
		{
			write_info_message("Could not open ", argv[1]);
		}
	}

	for(size_t offset = 0; offset < std::min(line_buffer.size(), term_height);
		offset++)
	{
		write_at_position(0, offset - window_y_offset,
						  line_buffer[offset].get_view().substr(0, term_width));
	}

	toggle_cell_highlighted(cursor_x - window_x_offset,
							cursor_y - window_y_offset);

	if(!line_buffer.size()) line_buffer.emplace_back("", "");

	while(true)
	{
		handle_input();
	}

	return 0;
}