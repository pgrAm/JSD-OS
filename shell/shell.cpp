#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <graphics/graphics.h>
#include <keyboard.h>
#include <sys/syscalls.h>

#include <string>
#include <vector>
#include <charconv>

void splash_text(int w);

std::string current_path{};
char prompt_char = ']';

std::vector<std::string> command_history;

directory_stream* current_directory = nullptr;

size_t drive_index = 0;

int cursor_x = 0, cursor_y = 0;

void select_drive(size_t index)
{
	if(current_directory != nullptr)
	{
		close_dir(current_directory);
	}
	drive_index = index;
	current_path.clear();
	current_directory = open_dir_handle(get_root_directory(drive_index), 0);
}

void list_directory()
{
	if(current_directory == nullptr)
	{
		printf("Invalid Directory\n");
		return;
	}

	printf("\n Name     Type  Size   Created     Modified\n\n");

	size_t total_bytes = 0;
	size_t i = 0;
	file_handle* f_handle = nullptr;

	while((f_handle = get_file_in_dir(current_directory, i++)))
	{
		file_info f;
		if(get_file_info(&f, f_handle) != 0)
		{
			continue;
		}

		struct tm created = *localtime(&f.time_created);
		struct tm modified = *localtime(&f.time_modified);

		total_bytes += f.size;

		std::string name = {f.name, f.name_len};

		if(f.flags & IS_DIR)
		{
			printf(" %-9s (DIR)     -  %02d-%02d-%4d  %02d-%02d-%4d\n",
				   name.c_str(),
				   created.tm_mon + 1, created.tm_mday, created.tm_year + 1900,
				   modified.tm_mon + 1, modified.tm_mday, modified.tm_year + 1900);
		}
		else
		{
			auto dot = name.find_first_of('.');

			printf(" %-10s %-3s  %5d  %02d-%02d-%4d  %02d-%02d-%4d\n",
				   name.substr(0, dot).c_str(),
				   name.substr(dot + 1).c_str(),
				   f.size,
				   created.tm_mon + 1, created.tm_mday, created.tm_year + 1900,
				   modified.tm_mon + 1, modified.tm_mday, modified.tm_year + 1900);
		}
	}
	printf("\n %5u Files   %5u Bytes\n\n", i - 1, total_bytes);
}

file_handle* find_file_in_dir(directory_stream* dir, file_info* f, const std::string_view name)
{
	file_handle* f_handle = find_path(dir, name.data(), name.size());
	if(f_handle != nullptr)
	{
		if(get_file_info(f, f_handle) == 0 && !(f->flags & IS_DIR))
		{
			return f_handle;
		}
	}
	return nullptr;
}

void print_mem()
{
	auto mem = get_free_memory();

	printf("Free memory %u:\n", mem);

	if(auto GiBs = (mem / 0x40000000) % 0x400)
	{
		printf("\t%d GiB(s)\n", GiBs);
	}
	if(auto MiBs = (mem / 0x100000) % 0x400)
	{
		printf("\t%d MiB(s)\n", MiBs);
	}
	if(auto KiBs = (mem / 0x400) % 0x400)
	{
		printf("\t%d KiB(s)\n", KiBs);
	}
	if(auto Bs = mem % 0x400)
	{
		printf("\t%d B(s)\n", Bs);
	}
}

void clear_console()
{
	printf("\x1b[2J\x1b[1;1H");
	video_clear();
}

std::vector<std::string_view> tokenize(std::string_view input, char delim)
{
	std::vector<std::string_view> keywords;

	auto pos = input.find_first_of(delim);
	keywords.push_back(input.substr(0, pos));
	while(pos++ != std::string_view::npos)
	{
		auto n = input.find_first_of(delim, pos);
		keywords.push_back(input.substr(pos, n));
		pos = n;
	}
	return keywords;
}

int execute_line(std::string_view current_line)
{
	std::vector<std::string_view> keywords = tokenize(current_line, ' ');

	if(keywords.empty())
	{
		return 0;
	}

	const auto& keyword = keywords[0];

	if("time" == keyword)
	{
		printf("%d\n", time(nullptr));
	}
	else if("rd0:" == keyword)
	{
		select_drive(0);
	}
	else if("fd0:" == keyword)
	{
		select_drive(1);
	}
	else if("fd1:" == keyword)
	{
		select_drive(2);
	}
	else if("drive:" == keyword)
	{
		size_t drive = 0;
		std::from_chars(keywords[1].cbegin(), keywords[1].cend(), drive);
		select_drive(drive);
	}
	else if("cls" == keyword || "clear" == keyword)
	{
		clear_console();
	}
	else if("echo" == keyword)
	{
		print_string(keywords[1].data(), keywords[1].size());
		putchar('\n');
	}
	else if("dir" == keyword || "ls" == keyword)
	{
		list_directory();
	}
	else if("cd" == keyword)
	{
		if(keywords.size() > 1)
		{
			const auto& path = keywords[1];

			file_info file;
			file_handle* f_handle = find_path(current_directory, path.data(), path.size());

			if(f_handle == nullptr)
			{
				printf("Could not find path ");
				print_string(path.data(), path.size());
				putchar('\n');
				return -1;
			}

			if(get_file_info(&file, f_handle) != 0 || !(file.flags & IS_DIR))
			{
				print_string(path.data(), path.size());
				printf(" is not a valid directory\n");
				return -1;
			}

			auto d = open_dir_handle(f_handle, 0);

			if(d == nullptr)
			{
				printf("Could not open directory ");
				print_string(path.data(), path.size());
				putchar('\n');
				return -1;
			}

			current_path.assign(file.name, file.size);
			current_directory = d;
			return 1;
		}
	}
	else if("mode" == keyword)
	{
		unsigned int width = 0;
		std::from_chars(keywords[1].cbegin(), keywords[1].cend(), width);
		unsigned int height = 0;
		std::from_chars(keywords[2].cbegin(), keywords[2].cend(), height);

		if(initialize_text_mode(width, height) == 0)
		{
			clear_console();
		}
		else
		{
			printf("Unable to set mode\n");
		}
	}
	else if("mem" == keyword)
	{
		print_mem();
	}
	else if("cat" == keyword || "type" == keyword)
	{
		const auto& arg = keywords[1];
		file_info file;
		file_handle* f_handle = find_file_in_dir(current_directory, &file, arg);
		if(f_handle)
		{
			file_stream* fs = open_file_handle(f_handle, 0);
			if(fs)
			{
				//printf("malloc'ing %d bytes\n", file.size);
				char* dataBuf = new char[file.size];
				read(dataBuf, file.size, fs);

				print_string(dataBuf, file.size);

				delete[] dataBuf;
				close(fs);
				putchar('\n');

				return 0;
			}
		}
		printf("Could not open file ");
		print_string(arg.data(), arg.size());
		putchar('\n');
		return -1;
	}
	else
	{
		file_info file;
		if(auto file_h = find_file_in_dir(current_directory, &file, keyword))
		{
			std::string_view name = {file.name, file.name_len};

			if(auto dot = name.find('.'); dot != std::string_view::npos)
			{
				std::string_view extension = name.substr(dot + 1);

				if(filesystem_names_identical("elf", extension))
				{
					file_info fi;
					get_file_info(&fi, file_h);

					spawn_process(file_h, current_directory, WAIT_FOR_PROCESS);
					return 0;
				}

				if(filesystem_names_identical("bat", extension))
				{
					file_stream* f = open(current_directory,
										  keyword.data(), keyword.size(), 0);
					if(f)
					{
						uint8_t* dataBuf = new uint8_t[file.size];

						read(dataBuf, file.size, f);
						close(f);

						auto lines = tokenize(std::string_view((char*)dataBuf, file.size), '\n');
						for(auto ln : lines)
						{
							execute_line(ln);
						}
						delete[] dataBuf;

						return 0;
					}
				}
			}
		}

		printf("File or Command not found\n");
		return -1;
	}

	return 1;
}

void set_mouse_color(uint8_t color)
{
	auto coord = (cursor_x + cursor_y * get_terminal_width()) * sizeof(uint16_t) + 1;

	get_screen_buf()[coord] = (get_screen_buf()[coord] & 0x0F) | (color << 4);
}

int get_command(char* input)
{
	std::string command_buffer;

	size_t history_index = 0;

	while(true)
	{
		input_event e;
		if(get_input_event(&e) == 0)
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
						else if(cursor_x >= get_terminal_width())
							cursor_x = get_terminal_width() - 1;

						set_mouse_color(7);
					}
					else if(e.control_index == 1) // y axis
					{
						set_mouse_color(0);
						cursor_y = (cursor_y + e.data);

						if(cursor_y < 0)
							cursor_y = 0;
						else if(cursor_y >= get_terminal_height())
							cursor_y = get_terminal_height() - 1;

						set_mouse_color(7);
					}
				}
			}
		}

		key_type k = getkey();
		if((k == VK_UP || k == VK_DOWN) && get_keystate(k))
		{
			if(!command_history.empty())
			{
				video_erase_chars(command_buffer.size());

				history_index %= command_history.size();

				command_buffer = command_history[history_index];

				history_index = (k == VK_UP) ? history_index + 1 : history_index - 1;

				printf("%s", command_buffer.c_str());
			}
		}
		else
		{
			char in_char = get_ascii_from_vk(k);

			if(in_char == '\b')
			{
				if(!command_buffer.empty())
				{
					command_buffer.pop_back();
					video_erase_chars(1);
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

	command_history.push_back(command_buffer);

	return execute_line(command_buffer);
}

const char* drive_names[] = {"rd0", "fd0", "fd1"};

void prompt()
{
	const char* drive = "?";
	if(drive_index < (sizeof(drive_names) / sizeof(char*)))
	{
		drive = drive_names[drive_index];
	}

	//printf("\x1b[32;22m");
	//printf("\x1b[37m@");
	printf("\x1b[32;22m%s\x1b[37m:/%s%c", drive, current_path.c_str(), prompt_char);
}

void splash_text(int w)
{
	printf("***");
	for(int i = 3; i < ((w / 2) - 3); i++)
	{
		putchar(' ');
	}
	printf("JSD/OS");
	for(int i = 3; i < ((w / 2) - 3); i++)
	{
		putchar(' ');
	}
	printf("***");
}


int main(int argc, char** argv)
{
	//asm volatile ("1: jmp 1b");

	initialize_text_mode(0, 0);
	video_clear();
	splash_text(get_terminal_width());

	time_t t_time = time(nullptr);

	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));

	printf("EPOCH TIME: %d\n", t_time);

	printf("EST Time: %s\n", ctime(&t_time));

	printf("t=%d\n", clock_ticks(nullptr));

	current_directory = open_dir_handle(get_root_directory(drive_index), 0);

	if(current_directory == nullptr)
	{
		printf("Could not mount root directory for drive %u %s\n", drive_index, drive_names[drive_index]);
	}

	for(;;)
	{
		prompt();
		get_command(nullptr);
	}

	return 0;
}
