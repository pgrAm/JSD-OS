#include "../drivers/video.h"
#include "../drivers/sysclock.h"
#include "../drivers/kbrd.h"
#include "../drivers/rs232.h"
#include "../drivers/floppy.h"
#include "../drivers/fat12.h"

#include "interrupt.h"
#include "memorymanager.h"
#include "multiboot.h"
#include "console.h"
#include "task.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

extern struct multiboot_info* _multiboot;

void kernel_main() 
{
	initialize_video();
	
	puts("Kernel Booted\n");
	
	idt_init();
	isrs_init();
	irqs_init();
	
	memmanager_init();
	
	sysclock_init();
	
	floppy_init();
	
	filesystem_setup_drives();

	clear_screen();
	
	setup_syscalls();
	
	setup_first_task(); //we are now running as a kernel level task
	
	keyboard_init();
	
	printf("***");
	
	for(int i = 3; i < 37; i++)
	{
		putchar(' ');
	}
	
	printf("JSD/OS");
	
	for(int i = 3; i < 37; i++)
	{
		putchar(' ');
	}
	
	printf("***");
	
	time_t t_time = time(NULL);
	
	printf("UTC Time: %s\n", asctime(gmtime(&t_time)));
	
	printf("UNIX TIME: %d\n", t_time);
	
	size_t free_mem = memmanager_num_bytes_free();
	size_t total_mem = memmanager_mem_size();
	
	printf("%u KB free / %u KB Memory\n\n", free_mem / 1024, total_mem / 1024);
	
	struct tm sys_time = *localtime(&t_time);
	
	printf("EST Time: %s\n", asctime(&sys_time));

	int drive_index = 0;
	
	directory_handle* current_directory = filesystem_get_root_directory(drive_index);
	
	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d\n", drive_index);
	}
	
	spawn_process("shell.elf", WAIT_FOR_PROCESS);
	
	for(;;);
}
