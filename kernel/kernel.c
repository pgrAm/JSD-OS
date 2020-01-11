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
extern void _IMAGE_END_;

int width = 80;
int height = 25;

void kernel_main() 
{
	//puts("Kernel Booted\n");
	initialize_video(80, 25);
	printf("BSS END %X\n", &_IMAGE_END_);

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
	
	size_t free_mem = memmanager_num_bytes_free();
	size_t total_mem = memmanager_mem_size();
	
	printf("%u KB free / %u KB Memory\n\n", free_mem / 1024, total_mem / 1024);

	int drive_index = 0;
	
	directory_handle* current_directory = filesystem_get_root_directory(drive_index);
	
	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d\n", drive_index);
	}
	
	spawn_process("shell.elf", WAIT_FOR_PROCESS);
	
	for(;;);
}
