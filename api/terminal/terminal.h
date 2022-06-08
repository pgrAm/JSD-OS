#ifndef TERMINAL_H
#define TERMINAL_H

enum terminal_open_mode
{
	TERMINAL_CREATE_NEW = 0,
	TERMINAL_CREATE_IF_NOT_FOUND,
	TERMINAL_OPEN_EXISTING
};

#ifdef __cplusplus

#include <stdint.h>
#include <string_view>

class terminal
{
public:
	static const int tab_size = 4;

	using open_mode = terminal_open_mode;

	terminal(const char* name, size_t name_len, size_t width, size_t height, open_mode mode);

	terminal(std::string_view name, size_t width, size_t height, open_mode mode)
		: terminal{name.data(), name.size(), width, height, mode}
	{}

	terminal(std::string_view name,
			 open_mode mode = TERMINAL_CREATE_IF_NOT_FOUND)
		: terminal{name.data(), name.size(), 0, 0, mode}
	{}

	terminal(const terminal&) = delete;
	~terminal();

	void set_cursor_pos(size_t x, size_t y);
	void set_cursor_pos(size_t offset);

	void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright);

	size_t cursor_pos() const;

	void delete_chars(size_t num);

	void print(const char* str, size_t length);

	inline void print(std::string_view text)
	{
		print(text.data(), text.size());
	}

	void print(char c);

	void print(int number);

	void print(unsigned int number);

	void print(char c, size_t num);

	uint8_t* get_underlying_buffer() const;

	void clear();
	void clear_row(size_t row);
	void scroll_up();

	int set_mode(size_t width, size_t height);

	size_t width() const;
	size_t height() const;

private:
	class impl;
	impl* m_impl;
};

extern "C"
{
#endif
	typedef enum terminal_open_mode terminal_open_mode;

	struct terminal_instance;
	typedef struct terminal_instance terminal_instance;

	terminal_instance* open_terminal(const char* name, size_t name_len,
									 size_t width, size_t height,
									 terminal_open_mode mode);

	void print_terminal(const char* buf, size_t size,
						terminal_instance* terminal);
	void close_terminal(terminal_instance* terminal);

	void reset_terminal_mode(terminal_instance* terminal);

#ifdef __cplusplus
}
#endif

#endif