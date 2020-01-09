#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/syscalls.h>

char console_user[] = "root";
char prompt_char = ']';
char command_buffer[256];

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
	printf("\n %5d Files   %5d Bytes\n\n", i, total_bytes);
}

int find_file_in_dir(const directory_handle* dir, file_info* f, const char* name)
{
	size_t i = 0;
	file_handle* f_handle = NULL;
	while ((f_handle = get_file_in_dir(dir, i++)))
	{
		get_file_info(f, f_handle);
		if (strcasecmp(name, f->name) == 0)
		{
			return 0;
		}
	}
	return -1;
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
		char *c = command_buffer;
		
		char* end = c + 256;
		
		char temp = '\0';
		
		while(c < end)
		{
			temp = getchar();
			
			if(temp == '\b') 
			{
				if(c > command_buffer)
				{
					*c-- = '\0';
					video_erase_chars(1);
				}
			}
			else
			{
				putchar(temp);
				*c = temp;
				c++;
			}
			
			if(temp == '\n') 
			{
				break;
			}
		}
		
		*c = '\0';
		
		keyword = strtok(command_buffer, " \n");
	}
	else
	{
		keyword = strtok(input, " \n");
		
		if(keyword == NULL)
		{
			return 0;
		}
	}
	
	if(strcmp("time", keyword) == 0)
	{
		printf("%d\n", time(NULL));
	}
	else if(strcmp("fd0:", keyword) == 0)
	{
		drive_index = 0;
		current_directory = get_root_directory(drive_index);
	}
	else if(strcmp("fd1:", keyword) == 0)
	{
		drive_index = 1;
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
			directory_handle* d = open_dir(path, 0);
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

		if (set_video_mode(width, height, VIDEO_TEXT_MODE) == 0)
		{
			clear_console();
		}
	}
	else if(strcmp("cat", keyword) == 0)
	{
		const char* arg = strtok(NULL, "\"\'\n");
		file_info file;
		if (find_file_in_dir(current_directory, &file, arg) == 0)
		{
			file_stream* fs = open(arg, 0);
			if (fs)
			{
				char *dataBuf = (char*)malloc(file.size);
				read(dataBuf, file.size, fs);
				
				for(size_t i = 0; i < file.size; i++)
				{
					if(dataBuf[i] != '\r')
					{
						putchar(dataBuf[i]);
					}
				}
				
				free(dataBuf);
				close(fs);
				putchar('\n');

				return 0;
			}
			printf("Could open find file %s\n", arg);
			return -1;
		}
		
		printf("Could not find file %s\n", arg);
		return -1;
	}
	else
	{
		file_info file;
		if(find_file_in_dir(current_directory, &file, keyword) == 0)
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

				file_stream* f = open(keyword, 0);
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

char* drive_names[] = {"fd0", "fd1"};

void prompt()
{
	printf("\x1b[32;22m%s\x1b[37m@%s%c", console_user, drive_names[drive_index], prompt_char);
}

int _start(void)
{
	current_directory = get_root_directory(drive_index);
	
	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d %s\n", drive_index, drive_names[drive_index]);
	}
	
	for(;;)
	{
		prompt();
		get_command(NULL);
	}
	
	return 0;
}
