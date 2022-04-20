#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <string_view>

class terminal
{
public:
	static const int tab_size = 5;

	terminal(size_t width, size_t height, const char* name, size_t name_len);
	terminal(const char* name, size_t name_len);

	terminal(size_t width, size_t height, std::string_view name) 
		: terminal{width, height, name.data(), name.size()}
	{}

	terminal(std::string_view name)
		: terminal{name.data(), name.size()}
	{}

	terminal(const terminal&) = delete;
	~terminal();

	void set_cursor_pos(size_t x, size_t y);
	void set_cursor_pos(size_t offset);

	void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright);

	size_t cursor_pos();

	void delete_chars(size_t num);

	void print_string(const char* str, size_t length);

	inline void print_string(std::string_view name)
	{
		print_string(name.data(), name.size());
	}

	void clear();
	void clear_row(size_t row);
	void scroll_up();

	int set_mode(size_t width, size_t height);

	size_t width();
	size_t height();

private:
	class impl;
	impl* m_impl;
};

#endif