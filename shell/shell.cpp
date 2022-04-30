#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <terminal/terminal.h>
#include <keyboard.h>
#include <sys/syscalls.h>

#include <string>
#include <vector>
#include <charconv>
#include <memory>
#include <array>

#include <shell/util.h>
#include <shell/shell.h>
#include <shell/commands.h>

terminal s_term{"terminal_1"};

std::string current_path{};

static std::vector<std::string> command_history;

file_h current_directory = nullptr;

size_t drive_index = 0;

int cursor_x = 0, cursor_y = 0;

static void select_drive(size_t index)
{
	drive_index = index;
	current_path.clear();
	current_directory = file_h{get_root_directory(drive_index)};
}

file_h find_file_in_dir(directory_stream* dir, file_info* f,
						const std::string_view name)
{
	file_h f_handle{find_path(dir, name.data(), name.size(), 0, 0)};
	if(f_handle != nullptr)
	{
		if(get_file_info(f, f_handle.get()) == 0 && !(f->flags & IS_DIR))
		{
			return f_handle;
		}
	}
	return nullptr;
}

file_h find_file_in_dir(file_info* f, const std::string_view name)
{
	auto dir = directory_ptr{open_dir_handle(get_current_dir(), 0)};

	return find_file_in_dir(dir.get(), f, name);
}

terminal& get_terminal()
{
	return s_term;
}

const file_handle* get_current_dir()
{
	return current_directory.get();
}

void clear_console()
{
	print_strings("\x1b[2J\x1b[1;1H");
	s_term.clear();
}

static std::vector<std::string_view> tokenize(std::string_view input, char delim)
{
	std::vector<std::string_view> keywords;
	size_t first = 0;
	while(first < input.size())
	{
		const auto second = input.find_first_of(delim, first);
		if(first != second)
		{
			keywords.emplace_back(input.substr(first, second - first));
		}
		if(second == std::string_view::npos)
		{
			break;
		}
		first = second + 1;
	}
	return keywords;
}

static int execute_line(std::string_view current_line)
{
	using namespace std::literals;

	std::vector<std::string_view> keywords = tokenize(current_line, ' ');

	if(keywords.empty())
	{
		return 0;
	}

	auto keyword = keywords[0];

	auto cmd = run_command(keyword, keywords);

	if(cmd.found) return cmd.result;

	if("rd0:"sv == keyword)
	{
		select_drive(0);
	}
	else if("fd0:"sv == keyword)
	{
		select_drive(1);
	}
	else if("fd1:"sv == keyword)
	{
		select_drive(2);
	}
	else if("drive:"sv == keyword)
	{
		if(keywords.size() > 1)
		{
			size_t drive = 0;
			std::from_chars(keywords[1].cbegin(), keywords[1].cend(), drive);
			select_drive(drive);
		}
	}
	else
	{
		auto dir = directory_ptr{open_dir_handle(current_directory.get(), 0)};

		file_info file;
		if(auto file_h = find_file_in_dir(dir.get(), &file, keyword))
		{
			std::string_view name = {file.name, file.name_len};

			if(auto dot = name.find('.'); dot != std::string_view::npos)
			{
				std::string_view extension = name.substr(dot + 1);

				if(filesystem_names_identical("elf", extension))
				{
					file_info fi;
					get_file_info(&fi, file_h.get());

					auto mode = (keywords.back() == "&") ? 0 : WAIT_FOR_PROCESS;

					spawn_process(file_h.get(), dir.get(), mode);
					return 0;
				}

				if(filesystem_names_identical("bat", extension))
				{
					auto f = file_ptr{open(dir.get(),
										  keyword.data(), keyword.size(), 0)};
					if(!f)
					{
						print_strings("Could not open file ", keyword, '\n');
						return -1;
					}

					auto dataBuf = std::make_unique<char[]>(file.size);

					read(&dataBuf[0], file.size, f.get());

					auto lines = tokenize(std::string_view(&dataBuf[0], file.size), '\n');
					for(auto ln : lines)
					{
						execute_line(ln);
					}

					return 0;
				}
			}
		}

		printf("File or Command not found\n");
		return -1;
	}

	return 1;
}

static void set_mouse_color(uint8_t color)
{
	auto coord = (cursor_x + cursor_y * s_term.width()) * sizeof(uint16_t) + 1;

	auto buf = s_term.get_underlying_buffer();

	buf[coord] = (buf[coord] & 0x0F) | (color << 4);
}

static int get_command()
{
	std::string command_buffer;

	size_t history_index = command_history.size();

	while(true)
	{
		input_event e;
		if(get_input_event(&e, true) == 0)
		{
			if(e.device_index == 0)
			{
				if(e.type == AXIS_MOTION)
				{
					if(e.control_index == 0) // x axis
					{
						set_mouse_color(0);
						cursor_x = (cursor_x + e.data);

						if(cursor_x < 0)
							cursor_x = 0;
						else if(cursor_x >= s_term.width())
							cursor_x = s_term.width() - 1;

						set_mouse_color(7);
					}
					else if(e.control_index == 1) // y axis
					{
						set_mouse_color(0);
						cursor_y = (cursor_y + e.data);

						if(cursor_y < 0)
							cursor_y = 0;
						else if(cursor_y >= s_term.height())
							cursor_y = s_term.height() - 1;

						set_mouse_color(7);
					}
				}
				else if(e.type == KEY_DOWN)
				{
					if(e.data == VK_UP || e.data == VK_DOWN)
					{
						if(!command_history.empty())
						{
							s_term.delete_chars(command_buffer.size());

							if(e.data == VK_UP)
							{
								if(history_index > 0)
									command_buffer = command_history[--history_index];
							}
							else
							{
								if(history_index < (command_history.size() - 1))
									command_buffer = command_history[++history_index];
							}

							s_term.print(command_buffer);
						}
					}
					else
					{
						char in_char = get_ascii_from_vk(e.data);

						if(in_char == '\b')
						{
							if(!command_buffer.empty())
							{
								command_buffer.pop_back();
								s_term.delete_chars(1);
							}
						}
						else if(in_char == '\n')
						{
							putchar(in_char);
							break;
						}
						else if(in_char != '\0')
						{
							putchar(in_char);
							command_buffer += in_char;
						}
					}
				}
			}
		}
	}

	command_history.push_back(command_buffer);

	return execute_line(command_buffer);
}

using namespace std::literals;

constexpr std::array drive_names = {"rd0"sv, "fd0"sv, "fd1"sv};

static void prompt()
{
	char prompt_char = ']';

	std::string_view drive = "?";
	if(drive_index < drive_names.size())
	{
		drive = drive_names[drive_index];
	}

	print_strings("\x1b[32;22m", drive, "\x1b[37m ");
	printf("%u", drive_index);
	print_strings(":/", current_path, prompt_char);
}

static void splash_text(int w)
{
	print_strings("***");
	for(int i = 3; i < ((w / 2) - 3); i++)
	{
		print_strings(' ');
	}
	print_strings("JSD/OS");
	for(int i = 3; i < ((w / 2) - 3); i++)
	{
		print_strings(' ');
	}
	print_strings("***");
}

int main(int argc, char** argv)
{
	set_stdout([](const char* buf, size_t size, void* impl) {
					s_term.print(buf, size);
			   });

	s_term.clear();
	splash_text(s_term.width());

	time_t t_time = time(nullptr);

	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));

	printf("EPOCH TIME: %u\n", t_time);

	printf("EST Time: %s\n", ctime(&t_time));

	//printf("t=%u\n", clock_ticks(nullptr));

	current_directory = file_h{get_root_directory(drive_index)};

	if(current_directory == nullptr)
	{
		print_strings("Could not mount root directory for drive ");
		printf("%d", drive_index);
		print_strings(drive_names[drive_index], '\n');
	}

	for(;;)
	{
		prompt();
		get_command();
	}

	return 0;
}
