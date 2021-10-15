extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <keyboard.h>
}

#include <string>
#include <vector>

void splash_text(int w);

std::string console_user = "root";
char prompt_char = ']';
#define MAX_HISTORY_SIZE 4
std::string command_buffers[MAX_HISTORY_SIZE];
size_t history_size = 0;
size_t command_buffer_index = 0;

directory_handle* current_directory = nullptr;

size_t drive_index = 0;
size_t test_index = 0;

void select_drive(size_t index)
{
	if(current_directory != nullptr)
	{
		close_dir(current_directory);
	}
	drive_index = index;
	current_directory = get_root_directory(drive_index);
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

file_handle* find_file_in_dir(const directory_handle* dir, file_info* f, const std::string& name)
{
	file_handle* f_handle = find_path(dir, name.c_str(), name.size());
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

	printf("Free memory %d:\n", mem);

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

int get_command(char* input)
{
	std::string current_line;

	if(input == nullptr)
	{
		if(history_size >= MAX_HISTORY_SIZE)
		{
			history_size = command_buffer_index + 1;
		}
		std::string command_buffer = command_buffers[command_buffer_index++ % history_size];

		while(true)
		{
			key_type k = wait_and_getkey();
			if((k == VK_UP || k == VK_DOWN) && get_keystate(k))
			{
				command_buffers[command_buffer_index] = command_buffer;

				command_buffer_index = (k == VK_UP) ? command_buffer_index - 1 : command_buffer_index + 1;
				command_buffer_index %= history_size;

				video_erase_chars(command_buffer.size());
				command_buffer = command_buffers[command_buffer_index];

				printf("%s", command_buffer.c_str());
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

		current_line = command_buffer;
	}
	else
	{
		current_line = strtok(input, "\n");
	}

	std::vector<std::string> keywords;

	auto pos = current_line.find_first_of(' ');
	keywords.push_back(current_line.substr(0, pos));
	while(pos++ != std::string::npos)
	{
		auto n = current_line.find_first_of(' ', pos);
		keywords.push_back(current_line.substr(pos, n));
		pos = n;
	}

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
		select_drive(atoi(keywords[1].c_str()));
	}
	else if("cls" == keyword || "clear" == keyword)
	{
		clear_console();
	}
	else if("echo" == keyword)
	{
		printf("%s\n", keywords[1].c_str());
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
			directory_handle* d = open_dir(current_directory,
										   path.c_str(),
										   path.size(), 0);
			if(d != nullptr)
			{
				current_directory = d;
				return 1;
			}
			close_dir(d);
			printf("Could not find path %s\n", path.c_str());
		}
	}
	else if("mode" == keyword)
	{
		int width = atoi(keywords[1].c_str());
		int height = atoi(keywords[2].c_str());

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
		printf("Could not open file %s\n", arg.c_str());
		return -1;
	}
	else
	{
		file_info file;
		if(find_file_in_dir(current_directory, &file, keyword))
		{
			const char* extension = strchr(file.name, '.') + 1;

			if(strcasecmp("elf", extension) == 0)
			{
				spawn_process(file.name, file.name_len, WAIT_FOR_PROCESS);
				return 0;
			}

			if(strcasecmp("bat", extension) == 0)
			{
				file_stream* f = open(current_directory, 
									  keyword.c_str(), keyword.size(), 0);
				if(f)
				{
					uint8_t* dataBuf = new uint8_t[file.size];

					read(dataBuf, file.size, f);
					close(f);

					get_command((char*)dataBuf);
					delete[] dataBuf;

					return 0;
				}
			}
		}

		printf("File or Command not found\n");
		return -1;
	}

	return 1;
}

const char* drive_names[] = {"rd0", "fd0", "fd1"};

void prompt()
{
	const char* drive = "?";
	if(drive_index < (sizeof(drive_names) / sizeof(char*)))
	{
		drive = drive_names[drive_index];
	}

	printf("\x1b[32;22m%s\x1b[37m@%s%c", console_user.c_str(), drive, prompt_char);
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
	splash_text(get_terminal_width());

	time_t t_time = time(nullptr);

	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));

	printf("UNIX TIME: %d\n", t_time);

	printf("EST Time: %s\n", ctime(&t_time));

	current_directory = get_root_directory(drive_index);

	if(current_directory == nullptr)
	{
		printf("Could not mount root directory for drive %u %s\n", drive_index, drive_names[drive_index]);
	}

	for(size_t i = 0; i < MAX_HISTORY_SIZE; i++)
	{
		command_buffers[i] = "";
	}

	for(;;)
	{
		prompt();
		get_command(nullptr);
	}

	return 0;
}
