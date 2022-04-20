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

terminal s_term{0, 0, "terminal_1"};

void splash_text(int w);

std::string current_path{};
char prompt_char = ']';

std::vector<std::string> command_history;

struct dir_deleter {
	void operator()(directory_stream* b) { close_dir(b); }
};

using directory_ptr = std::unique_ptr <directory_stream, dir_deleter>;

struct file_deleter {
	void operator()(file_stream* b) { close(b); }
};

using file_ptr = std::unique_ptr <file_stream, file_deleter>;

struct file_handle_deleter {
	void operator()(const file_handle* f) { dispose_file_handle(f); }
};

using file_h = std::unique_ptr <const file_handle, file_handle_deleter>;

file_h current_directory = nullptr;

size_t drive_index = 0;

int cursor_x = 0, cursor_y = 0;

void select_drive(size_t index)
{
	drive_index = index;
	current_path.clear();
	current_directory = file_h{get_root_directory(drive_index)};
}

void list_directory(const file_handle* dir_handle)
{
	if(dir_handle == nullptr)
	{
		printf("Invalid Directory\n");
		return;
	}

	auto dir = directory_ptr{open_dir_handle(dir_handle, 0)};
	if(!dir)
	{
		printf("Something didn't work\n");
		return;
	}

	printf("\n Name     Type  Size   Created     Modified\n\n");

	size_t total_bytes = 0;
	size_t i = 0;
	file_h f_handle = nullptr;

	while((f_handle = file_h{get_file_in_dir(dir.get(), i++)}))
	{
		file_info f;
		if(get_file_info(&f, f_handle.get()) != 0)
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

file_h find_file_in_dir(directory_stream* dir, file_info* f, const std::string_view name)
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
	s_term.clear();
}

std::vector<std::string_view> tokenize(std::string_view input, char delim)
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

void file_error(std::string_view error, std::string_view file)
{
	s_term.print_string(error);
	s_term.print_string(file);
	putchar('\n');
}

int execute_line(std::string_view current_line)
{
	using namespace std::literals;

	std::vector<std::string_view> keywords = tokenize(current_line, ' ');

	if(keywords.empty())
	{
		return 0;
	}

	auto dir = directory_ptr{open_dir_handle(current_directory.get(), 0)};

	const auto& keyword = keywords[0];

	if("time"sv == keyword)
	{
		printf("%u\n", time(nullptr));
	}
	else if("rd0:"sv == keyword)
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
		size_t drive = 0;
		std::from_chars(keywords[1].cbegin(), keywords[1].cend(), drive);
		select_drive(drive);
	}
	else if("cls"sv == keyword || "clear"sv == keyword)
	{
		clear_console();
	}
	else if("echo" == keyword)
	{
		s_term.print_string(keywords[1]);
		putchar('\n');
	}
	else if("dir"sv == keyword || "ls"sv == keyword)
	{
		if(keywords.size() > 1)
		{
			const auto& path = keywords[1];

			file_h list_dir{find_path(dir.get(), path.data(), path.size(), 0, 0)};

			list_directory(list_dir.get());
		}
		else
		{
			list_directory(current_directory.get());
		}
	}
	else if("cd"sv == keyword)
	{
		if(keywords.size() < 2)
		{
			printf("Not enough arguments\n");
		}

		const auto& path = keywords[1];

		file_info file;
		file_h f_handle{find_path(dir.get(), path.data(), path.size(), 0, 0)};

		if(f_handle == nullptr)
		{
			file_error("Could not find path ", path);
			return -1;
		}

		if(get_file_info(&file, f_handle.get()) != 0 || !(file.flags & IS_DIR))
		{
			s_term.print_string(path);
			printf(" is not a valid directory\n");
			return -1;
		}

		current_path.assign(file.name, file.name_len);
		current_directory = std::move(f_handle);
		return 1;
	}
	else if("mode"sv == keyword)
	{
		unsigned int width = 0;
		std::from_chars(keywords[1].cbegin(), keywords[1].cend(), width);
		unsigned int height = 0;
		std::from_chars(keywords[2].cbegin(), keywords[2].cend(), height);

		if(s_term.set_mode(width, height) == 0)
		{
			clear_console();
		}
		else
		{
			printf("Unable to set mode\n");
		}
	}
	else if("mem"sv == keyword)
	{
		print_mem();
	}
	else if("cat"sv == keyword || "type"sv == keyword)
	{
		const auto& arg = keywords[1];
		file_info file;

		if(auto f_handle = find_file_in_dir(dir.get(), &file, arg))
		{
			auto fs = file_ptr{open_file_handle(f_handle.get(), 0)};
			if(fs)
			{
				auto dataBuf = std::make_unique<char[]>(file.size);

				read(&dataBuf[0], file.size, fs.get());

				s_term.print_string(&dataBuf[0], file.size);

				putchar('\n');

				return 0;
			}
		}
		file_error("Could not open file ", arg);
		return -1;
	}
	else if("mkdir"sv == keyword)
	{
		if(keywords.size() < 2)
		{
			printf("Incorrect number of arguments\n");
			return -1;
		}

		find_path(dir.get(), keywords[1].data(), keywords[1].size(), FILE_CREATE, IS_DIR);
	}
	else if("copy"sv == keyword)
	{
		if(keywords.size() < 3)
		{
			printf("Incorrect number of arguments\n");
			return -1;
		}

		const auto& src = keywords[1];
		const auto& dst = keywords[2];
		file_info file;
		auto src_handle = find_file_in_dir(dir.get(), &file, src);
		if(!src_handle)
		{
			file_error("Could not find source file ", src);
			return -1;
		}

		auto src_stream = file_ptr{open_file_handle(src_handle.get(), 0)};
		if(!src_stream)
		{
			file_error("Could not find open source file ", src);
			return -1;
		}

		auto dst_stream = file_ptr{open(dir.get(),
							   dst.data(), dst.size(),
							   FILE_WRITE | FILE_CREATE)};
		if(!dst_stream)
		{
			file_error("Could not create destination file ", dst);
			return -1;
		}

		auto dataBuf = std::make_unique<char[]>(file.size);

		read(&dataBuf[0], file.size, src_stream.get());
		write(&dataBuf[0], file.size, dst_stream.get());

		return 0;
	}
	else
	{
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
						file_error("Could not open file ", keyword);
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

void set_mouse_color(uint8_t color)
{
	//auto coord = (cursor_x + cursor_y * s_term.width()) * sizeof(uint16_t) + 1;

	//get_screen_buf()[coord] = (get_screen_buf()[coord] & 0x0F) | (color << 4);
}

int get_command()
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

							s_term.print_string(command_buffer);
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
	printf("\x1b[32;22m%s\x1b[37m %u:/%s%c", drive, drive_index, current_path.c_str(), prompt_char);
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
	set_stdout([](const char* buf, size_t size, void* impl) {
					s_term.print_string(buf, size);
			   });

	s_term.clear();
	splash_text(s_term.width());

	time_t t_time = time(nullptr);

	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));

	printf("EPOCH TIME: %u\n", t_time);

	printf("EST Time: %s\n", ctime(&t_time));

	printf("t=%u\n", clock_ticks(nullptr));

	current_directory = file_h{get_root_directory(drive_index)};

	if(current_directory == nullptr)
	{
		printf("Could not mount root directory for drive %u %s\n", drive_index, drive_names[drive_index]);
	}

	for(;;)
	{
		prompt();
		get_command();
	}

	return 0;
}
