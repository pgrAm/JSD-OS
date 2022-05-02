#ifndef SHELL_H
#define SHELL_H

#include <string_view>
#include <shell/util.h>

terminal& get_terminal();
const file_handle* get_current_dir();
void clear_console();

file_h find_file_in_dir(directory_stream* dir, file_info* f,
						const std::string_view name);
file_h find_file_in_dir(file_info* f, const std::string_view name);

template<typename... Args>
inline void print_strings(Args&&... args)
{
	(get_terminal().print(std::forward<Args>(args)), ...);
}

inline void print_chars(char c, size_t num)
{
	get_terminal().print(c, num);
}

#endif