#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <keyboard.h>

char console_user[] = "root";
char prompt_char = ']';
#define MAX_HISTORY_SIZE 4
char command_buffers[MAX_HISTORY_SIZE][MAX_PATH];
size_t history_size = 0;
size_t command_buffer_index = 0;
char* command_buffer = command_buffers[0];

directory_handle* current_directory = NULL;

size_t drive_index = 0;

void list_directory()
{
	if(current_directory == NULL)
	{
		printf("Invalid Directory\n");
		return;
	}
	
	printf("\n Name     Type  Size   Created     Modified\n\n");
	
	size_t total_bytes = 0;
	size_t i = 0;
	file_handle* f_handle = NULL;

	while((f_handle = get_file_in_dir(current_directory, i++)))
	{
		file_info f;
		get_file_info(&f, f_handle);

		struct tm created	= *localtime(&f.time_created);
		struct tm modified	= *localtime(&f.time_modified);
		
		total_bytes += f.size;

		if (f.flags & IS_DIR)
		{
			printf(" %-8s (DIR)     -  %02d-%02d-%4d  %02d-%02d-%4d\n",
				f.name,
				created.tm_mon + 1, created.tm_mday, created.tm_year + 1900,
				modified.tm_mon + 1, modified.tm_mday, modified.tm_year + 1900);
		}
		else
		{
			putchar(' ');

			int i = 1;
			char c = f.name[0];
			while (c != '\0' && c != '.')
			{
				putchar(c);
				c = f.name[i++];
			}

			int num_spaces = i;
			while (num_spaces++ < 10)
			{
				putchar(' ');
			}

			printf(" %-3s  %5d  %02d-%02d-%4d  %02d-%02d-%4d\n",
				f.name + i,
				f.size,
				created.tm_mon + 1, created.tm_mday, created.tm_year + 1900,
				modified.tm_mon + 1, modified.tm_mday, modified.tm_year + 1900);
		}
	}
	printf("\n %5d Files   %5d Bytes\n\n", i-1, total_bytes);
}

file_handle* find_file_in_dir(const directory_handle* dir, file_info* f, const char* name)
{
	file_handle* f_handle = find_path(dir, name);
	if (f_handle != NULL)
	{
		if (get_file_info(f, f_handle) == 0 && !(f->flags & IS_DIR))
		{
			return f_handle;
		}
	}
	return NULL;
}

void clear_console()
{
	printf("\x1b[2J\x1b[1;1H");
	video_clear();
}

int get_command(char* input)
{
	char* keyword;
	
	if(input == NULL)
	{
		if (history_size <= MAX_HISTORY_SIZE) 
		{
			history_size = command_buffer_index + 1;
		}
		command_buffer = command_buffers[command_buffer_index++ % history_size];

		char *c = command_buffer;
		char* end = command_buffer + MAX_PATH;
		
		while(c < end)
		{
			key_type k = wait_and_getkey();
			if ((k == VK_UP || k == VK_DOWN) && get_keystate(k))
			{
				command_buffer_index = (k == VK_UP) ? command_buffer_index - 1 : command_buffer_index + 1;
				command_buffer_index %= history_size;

				video_erase_chars(c - command_buffer);

				command_buffer = command_buffers[command_buffer_index];

				printf("%s", command_buffer);
				c = command_buffer + strlen(command_buffer);
				end = command_buffer + MAX_PATH;
			}
			else
			{
				char temp = get_ascii_from_vk(k);

				if (temp == '\b')
				{
					if (c > command_buffer)
					{
						*c-- = '\0';
						video_erase_chars(1);
					}
				}
				else if (temp != '\0')
				{
					putchar(temp);
					*c = temp;
					c++;
				}

				if (temp == '\n')
				{
					break;
				}
			}
		}
		
		*c = '\0';

		keyword = strtok(command_buffer, " \n");
	}
	else
	{
		keyword = strtok(input, " \n");
	}

	if (keyword == NULL)
	{
		return 0;
	}
	
	if(strcmp("time", keyword) == 0)
	{
		printf("%d\n", time(NULL));
	}
	else if(strcmp("rd0:", keyword) == 0)
	{
		drive_index = 0;
		current_directory = get_root_directory(drive_index);
	}
	else if(strcmp("fd0:", keyword) == 0)
	{
		drive_index = 1;
		current_directory = get_root_directory(drive_index);
	}
	else if(strcmp("fd1:", keyword) == 0)
	{
		drive_index = 2;
		current_directory = get_root_directory(drive_index);
	}
	else if(strcmp("cls", keyword) == 0 || strcmp("clear", keyword) == 0)
	{
		clear_console();
	}
	else if(strcmp("echo", keyword) == 0)
	{
		printf("%s\n", strtok(NULL, "\"\'\n"));
	}
	else if(strcmp("dir", keyword) == 0 || strcmp("ls", keyword) == 0 )
	{
		list_directory();
	}
	else if (strcmp("cd", keyword) == 0)
	{
		char* path = strtok(NULL, "\"\'\n");
		if (path != NULL)
		{
			directory_handle* d = open_dir(current_directory, path, 0);
			if (d != NULL)
			{
				current_directory = d;
				return 1;
			}
			printf("Could not find path %s\n", path);
		}
	}
	else if (strcmp("mode", keyword) == 0)
	{
		int width = atoi(strtok(NULL, "\"\'\n "));
		int height = atoi(strtok(NULL, "\"\'\n "));

		if (initialize_text_mode(width, height) == 0)
		{
			clear_console();
		}
	}
	else if(strcmp("cat", keyword) == 0 || strcmp("type", keyword) == 0)
	{
		const char* arg = strtok(NULL, "\"\'\n");
		file_info file;
		file_handle* f_handle = find_file_in_dir(current_directory, &file, arg);
		if (f_handle)
		{
			file_stream* fs = open_file_handle(f_handle, 0);
			if (fs)
			{
				//printf("malloc'ing %d bytes\n", file.size);
				char* dataBuf = (char*)malloc(file.size);
				read(dataBuf, file.size, fs);

				print_string(dataBuf, file.size);

				free(dataBuf);
				close(fs);
				putchar('\n');

				return 0;
			}
		}
		printf("Could not open file %s\n", arg);
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
				spawn_process(file.name, WAIT_FOR_PROCESS);
				return 0;
			}

			if (strcasecmp("bat", extension) == 0)
			{
				uint8_t* dataBuf = (uint8_t*)malloc(file.size);

				file_stream* f = open(current_directory, keyword, 0);
				if (f)
				{
					read(dataBuf, file.size, f);
					close(f);

					get_command((char*)dataBuf);
					free(dataBuf);

					return 0;
				}
			}
		}

		printf("File or Command not found\n");
		return -1;
	}

	return 1;
}

char* drive_names[] = {"rd0", "fd0", "fd1"};

void prompt()
{
	printf("\x1b[32;22m%s\x1b[37m@%s%c", console_user, drive_names[drive_index], prompt_char);
}

int width = 90;
int height = 30;

void splash_text(int w)
{
	printf("***");
	for (int i = 3; i < ((w / 2) - 3); i++)
	{
		putchar(' ');
	}
	printf("JSD/OS");
	for (int i = 3; i < ((w / 2) - 3); i++)
	{
		putchar(' ');
	}
	printf("***");
}


int _start(void)
{
	initialize_text_mode(width, height);
	splash_text(width);
		
	time_t t_time = time(NULL);

	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));

	printf("UNIX TIME: %d\n", t_time);

	struct tm sys_time = *localtime(&t_time);

	printf("EST Time: %s\n", asctime(&sys_time));

	current_directory = get_root_directory(drive_index);
	
	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d %s\n", drive_index, drive_names[drive_index]);
	}
	
	for (size_t i = 0; i < MAX_HISTORY_SIZE; i++)
	{
		memset(command_buffers[i], '\0', MAX_PATH);
	}

	for(;;)
	{
		prompt();
		get_command(NULL);
	}
	
	return 0;
}
