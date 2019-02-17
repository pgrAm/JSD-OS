#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../drivers/video.h"
#include "../kernel/filesystem.h"

#include "../kernel/memorymanager.h"

char console_user[] = "root";
char prompt_char = ']';
char command_buffer[256];

directory* current_directory = NULL;

size_t drive_index = 0;

void list_directory()
{
	if(current_directory == NULL)
	{
		printf("Invalid Directory\n");
		return;
	}
	
	printf("\n Name    Type  Size   Created     Modified\n\n");
	
	const char* format = " %-8s %s  %5d  %02d-%02d-%4d  %02d-%02d-%4d\n";
	
	size_t total_bytes = 0;
	size_t i;
	
	for(i = 0; i < current_directory->num_files; i++)
	{
		struct tm created	= *localtime(&current_directory->file_list[i].time_created);
		struct tm modified	= *localtime(&current_directory->file_list[i].time_modified);
		
		total_bytes += current_directory->file_list[i].size;
		
		printf(format,	current_directory->file_list[i].name, 
						current_directory->file_list[i].type, 
						current_directory->file_list[i].size, 
						created.tm_mon + 1, created.tm_mday, created.tm_year + 1900,
						modified.tm_mon + 1, modified.tm_mday, modified.tm_year + 1900);
	}
	printf("\n %5d Files   %5d Bytes\n\n", i, total_bytes);
}

typedef int (*cMain)(int, char**);

typedef int (*exeHeader)(void);

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
					delete_chars(1);
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
		current_directory = filesystem_mount_root_directory(drive_index);
	}
	else if(strcmp("fd1:", keyword) == 0)
	{
		drive_index = 1;
		current_directory = filesystem_mount_root_directory(drive_index);
	}
	else if(strcmp("cls", keyword) == 0 || strcmp("clear", keyword) == 0)
	{
		printf("\x1b[2J\x1b[1;1H");
		clear_screen();
	}
	else if(strcmp("echo", keyword) == 0)
	{
		printf("%s\n", strtok(NULL, "\"\'\n"));
	}
	else if(strcmp("dir", keyword) == 0 || strcmp("ls", keyword) == 0 )
	{
		list_directory();
	}
	else if(strcmp("cat", keyword) == 0)
	{
		file_stream* file;
		
		if((file = filesystem_open_file(strtok(NULL, "\"\'\n"))))
		{
			size_t file_size = filesystem_get_size(file);
			
			char *dataBuf = (char*)malloc(file_size);
			
			filesystem_read_file(dataBuf, file_size, file);
			
			for(int i = 0; i < file_size; i++)
			{
				if(dataBuf[i] != '\r')
				{
					putchar(dataBuf[i]);
					//printf("%X,", dataBuf[i]);
				}
			}
			
			filesystem_close_file(file);
			free(dataBuf);
			
			putchar('\n');
		}
	}
	else
	{
		file_handle* file;
		
		if((file = filesystem_find_file_in_dir(current_directory, keyword)))
		{	
			if(strcmp("EXE", file->type) == 0)
			{
				int p = load_exe(file);
				printf("%d\n", p);
				return p;
			}
			else if(strcmp("ELF", file->type) == 0)
			{
				int p = run_elf(file);
				printf("%d\n", p);
				return p;
			}
	
			uint8_t *dataBuf = (uint8_t*)malloc(file->size);
			
			file_stream* f = filesystem_open_handle(file);
			
			filesystem_read_file(dataBuf, file->size, f);
			
			if(strcmp("BAT", file->type) == 0)
			{
				get_command(dataBuf);
			}
			
			filesystem_close_file(f);
			free(dataBuf);
		}
		else
		{
			printf("File or Command not found\n");
			return -1;
		}
	}

	return 1;
}

char* drive_names[] = {"fd0", "fd1"};

void prompt()
{
	printf("\x1b[32;22m%s\x1b[37m@%s%c", console_user, drive_names[drive_index], prompt_char);
}

int enter_console()
{
	//printf("dir=%X\n", current_directory);
	
	current_directory = filesystem_mount_root_directory(drive_index);
	
	//printf("EIP=%X\n", getEIP());
	
	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d %s\n", drive_index, drive_names[drive_index]);
	}
	
	//printf("EIP=%X\n", getEIP());
	
	for(;;)
	{
		prompt();
		get_command(NULL);
	}
	
	return 0;
}
