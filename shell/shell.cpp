#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <terminal/terminal.h>
#include <keyboard.h>
#include <sys/syscalls.h>

#include <string_view>
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
	current_path = std::to_string(index) + ':';
	current_directory.reset(get_root_directory(drive_index));
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

constexpr static std::vector<std::string_view> tokenize(std::string_view input,
														char delim)
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
		if(second == input.npos)
		{
			break;
		}
		first = second + 1;
	}
	return keywords;
}

using namespace std::literals;

constexpr std::array drive_names = {"rd0"sv, "fd0"sv, "fd1"sv};

static int execute_line(std::string_view current_line)
{
	std::vector<std::string_view> keywords = tokenize(current_line, ' ');

	if(keywords.empty())
	{
		return 0;
	}

	const auto keyword = keywords[0];

	if(auto cmd = run_command(keyword, keywords); cmd.found)
	{
		return cmd.result;
	}
	else if(keyword.ends_with(':'))
	{
		const auto drive_cmd = keyword.substr(0, keyword.size() - 1);

		if(auto it =
			   std::find(drive_names.begin(), drive_names.end(), drive_cmd);
		   it != drive_names.end())
		{
			select_drive((size_t)(it - drive_names.begin()));
		}
		else if("drive"sv == drive_cmd)
		{
			if(keywords.size() > 1)
			{
				size_t drive = 0;
				std::from_chars(keywords[1].cbegin(), keywords[1].cend(),
								drive);
				select_drive(drive);
			}
		}
	}
	else
	{
		auto dir = directory_ptr{open_dir_handle(current_directory.get(), 0)};

		file_info file;
		if(auto file_h = find_file_in_dir(dir.get(), &file, keyword))
		{
			const std::string_view name{file.full_path, file.full_path_len};

			if(auto dot = name.find('.'); dot != name.npos)
			{
				const std::string_view extension = name.substr(dot + 1);

				if(filesystem_names_identical("elf", extension))
				{
					file_info fi;
					get_file_info(&fi, file_h.get());

					auto mode = keywords.back().ends_with('&') ? 0 : WAIT_FOR_PROCESS;

					spawn_process(file_h.get(), current_line.data(),
								  current_line.size(), mode);
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

					read(0, &dataBuf[0], file.size, f.get());

					auto lines = tokenize(std::string_view(&dataBuf[0], file.size), '\n');
					for(auto ln : lines)
					{
						execute_line(ln);
					}

					return 0;
				}
			}
		}

		print_strings("File or Command not found\n");
		return -1;
	}

	return 1;
}

static void set_mouse_color(uint8_t color)
{
	auto coord = (cursor_x + cursor_y * (int)s_term.width()) * (int)sizeof(uint16_t) + 1;

	auto buf = s_term.get_underlying_buffer();

	buf[coord] = (uint8_t)(buf[coord] & 0x0Fu) | (uint8_t)(color << 4);
}


static int get_command()
{
	std::string input;

	size_t history_index = command_history.size();
	size_t char_index	 = 0;

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
						cursor_x = std::clamp(cursor_x + e.data, 0,
											  (int)s_term.width() - 1);
						set_mouse_color(7);
					}
					else if(e.control_index == 1) // y axis
					{
						set_mouse_color(0);
						cursor_y = std::clamp(cursor_y + e.data, 0,
											  (int)s_term.height() - 1);
						set_mouse_color(7);
					}
				}
				else if(e.type == KEY_DOWN)
				{
					if(e.data == VK_RIGHT) 
					{
						if(char_index < input.size())
						{
							char_index++;
							s_term.set_cursor_pos(s_term.cursor_pos() + 1);
						}
					}
					else if(e.data == VK_LEFT)
					{
						if(char_index > 0)
						{
							char_index--;
							s_term.set_cursor_pos(s_term.cursor_pos() - 1);
						}
					}
					else if(e.data == VK_UP || e.data == VK_DOWN)
					{
						if(!command_history.empty())
						{
							s_term.set_cursor_pos(s_term.cursor_pos() +
												  input.size() - char_index);
							s_term.delete_chars(input.size());

							if(e.data == VK_UP)
							{
								if(history_index > 0)
									--history_index;
							}
							else
							{
								if(history_index < (command_history.size() - 1))
									++history_index;
							}

							input = command_history[history_index];

							s_term.print(input);
							char_index = input.size();
						}
					}
					else
					{
						char in_char = get_ascii_from_vk(e.data);

						if(in_char == '\b')
						{
							if(char_index > 0)
							{
								--char_index;
								input.erase(input.begin() + char_index);
								auto cursor = s_term.cursor_pos();
								s_term.set_cursor_pos(cursor + input.size() -
													  char_index);
								s_term.delete_chars(1);
								s_term.set_cursor_pos(--cursor);
								s_term.print(
									std::string_view{input}.substr(char_index));
								s_term.set_cursor_pos(cursor);
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
							input.insert(input.begin() + char_index, in_char);
							char_index++;
							auto cursor = s_term.cursor_pos();
							s_term.print(
								std::string_view{input}.substr(char_index));
							s_term.set_cursor_pos(cursor);
						}
					}
				}
			}
		}
	}

	command_history.emplace_back(std::move(input));

	return execute_line(command_history.back());
}

static void prompt()
{
	constexpr char prompt_char = ']';

	std::string_view drive = "?";
	if(drive_index < drive_names.size())
	{
		drive = drive_names[drive_index];
	}

	print_strings("\x1b[32;22m", drive, "\x1b[37m ", current_path, prompt_char);
}

static void splash_text(size_t w)
{
	print_chars('*', 3);
	print_chars(' ', ((w / 2) - 6u));
	print_strings("JSD/OS");
	print_chars(' ', ((w / 2) - 6u));
	print_chars('*', 3);
}

int main(int argc, char** argv)
{
	set_stdout(
		[](const char* buf, size_t size, void* impl)
		{
			s_term.print(buf, size);
			return size;
		});

	s_term.clear();
	splash_text(s_term.width());

	time_t t_time = time(nullptr);

	print_strings("UTC Time: ", asctime(gmtime(&t_time)), '\n');
	print_strings("EPOCH TIME: ", (size_t)t_time, '\n');
	print_strings("EST Time: ", ctime(&t_time), '\n');

	//printf("t=%u\n", clock_ticks(nullptr));

	select_drive(drive_index);

	if(current_directory == nullptr)
	{
		print_strings("Could not mount root directory for drive ", drive_index, ' ',
					  drive_names[drive_index], '\n');
	}

	for(;;)
	{
		prompt();
		get_command();
	}

	return 0;
}
