#include "../drivers/video.h"
#include "../drivers/sysclock.h"
#include "../drivers/kbrd.h"
#include "../drivers/rs232.h"
#include "../drivers/floppy.h"
#include "../drivers/fat12.h"

#include "interrupt.h"
#include "memorymanager.h"
#include "multiboot.h"
#include "task.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

//extern struct multiboot_info* _multiboot;
extern void _IMAGE_END_;

#include <elf.h>

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

	dynamic_object ob;
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	hashmap_insert(ob.symbol_map, "handle_keyevent",		(uintptr_t)&handle_keyevent);
	hashmap_insert(ob.symbol_map, "irq_install_handler",	(uintptr_t)&irq_install_handler);
	hashmap_insert(ob.symbol_map, "printf",					(uintptr_t)&printf);
	load_elf("kbrd.drv", &ob, false);

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, "AT_keyboard_init", &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}

	//while (true);

	spawn_process("shell.elf", WAIT_FOR_PROCESS);
	
	for(;;);
}
