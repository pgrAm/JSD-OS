#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <kernel/filesystem.h>
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/multiboot.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <drivers/video.h>
#include <drivers/sysclock.h>
#include <drivers/kbrd.h>
#include <drivers/rs232.h>
#include <drivers/floppy.h>
#include <drivers/ramdisk.h>
#include <drivers/isa_dma.h>

extern multiboot_info* _multiboot;

extern void _IMAGE_END_;
extern void _BSS_END_;
extern void _DATA_END_;

typedef struct {
	const char* name;
	void* address;
} func_info;

void load_driver(const char* filename, const char* func_name, 
				 const func_info* functions, size_t num_functions)
{
	dynamic_object ob;
	memset(&ob, 0, sizeof(dynamic_object));
	ob.lib_set = hashmap_create(16);
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	for(size_t i = 0; i < num_functions; i++)
	{
		//printf("key = %s, value = %X\n", functions[i].name, (uint32_t)functions[i].address);
		hashmap_insert(ob.symbol_map, 
					   functions[i].name, (uint32_t)functions[i].address);
	}

	//hashmap_print(ob.symbol_map);

	load_elf(filename, &ob, false);

	/*for(size_t i = 0; i < num_functions; i++)
	{
		printf("key = %s, value = %X\n", functions[i].name, (uint32_t)functions[i].address);
	}*/

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, func_name, &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}
}

void load_floppy_driver()
{
	func_info list[] = {
		{"irq_install_handler", &irq_install_handler},
		{"printf", &printf},
		{"filesystem_add_drive", &filesystem_add_drive},
		{"sysclock_sleep", &sysclock_sleep},
		{"isa_dma_begin_transfer", &isa_dma_begin_transfer}
	};

	load_driver("floppy.drv", "floppy_init", list, sizeof(list) / sizeof(func_info));
}

void load_kbrd_driver()
{
	func_info list[] = {
		{"irq_install_handler", &irq_install_handler},
		{"printf", &printf},
		{"handle_keyevent", &handle_keyevent}
	};

	load_driver("kbrd.drv", "AT_keyboard_init", list, sizeof(list) / sizeof(func_info));
}

extern void memmanager_reserve_physical_memory(uintptr_t address, size_t size);
extern void memmanager_map_page(uintptr_t virtual_address, uintptr_t physical_address, uint32_t flags);
extern void memmanager_print_all_mappings_to_physical_DEBUG(uintptr_t physical);

void list_directory(directory_handle* current_directory)
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

	while((f_handle = filesystem_get_file_in_dir(current_directory, i++)))
	{
		file_info f;
		filesystem_get_file_info(&f, f_handle);

		struct tm created = *localtime(&f.time_created);
		struct tm modified = *localtime(&f.time_modified);

		total_bytes += f.size;

		if(f.flags & IS_DIR)
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
			while(c != '\0' && c != '.')
			{
				putchar(c);
				c = f.name[i++];
			}

			int num_spaces = i;
			while(num_spaces++ < 10)
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
	printf("\n %5d Files   %5d Bytes\n\n", i - 1, total_bytes);
}

void kernel_main()
{
	initialize_video(80, 25);

	clear_screen();

	printf("Found Kernel %X - %X\n", 0x8000, &_BSS_END_);

	for(size_t i = 0; i < _multiboot->m_modsCount; i++)
	{
		uintptr_t rd_begin = ((multiboot_modules*)_multiboot->m_modsAddr)[i].begin;
		uintptr_t rd_end = ((multiboot_modules*)_multiboot->m_modsAddr)[i].end;

		printf("Found Module %X - %X\n", rd_begin, rd_end);
	}

	idt_init();
	isrs_init();
	irqs_init();

	memmanager_init();

	//while(true);

	sysclock_init();

	setup_syscalls();

	setup_first_task(); //we are now running as a kernel level task

	ramdisk_init();

	filesystem_setup_drives();

	//memmanager_map_page(0x30000, 0, PAGE_PRESENT);
	//memmanager_reserve_physical_memory(0x30000, 0x60000);
	//memmanager_reserve_physical_memory(0x90000, 0x1000);
	keyboard_init();

	load_kbrd_driver();

	//memmanager_reserve_physical_memory(0x30000, 0x1000);

	//memmanager_print_all_mappings_to_physical_DEBUG(0x30000);
	//memmanager_virtual_alloc(NULL, 1, PAGE_RW | PAGE_PRESENT);

	load_floppy_driver();


	filesystem_set_default_drive(1);

	//load_kbrd_driver();

	//while(true);

	filesystem_get_root_directory(1);

	clear_screen();

	size_t free_mem = memmanager_num_bytes_free();
	size_t total_mem = memmanager_mem_size();

	printf("%u KB free / %u KB Memory\n\n", free_mem / 1024, total_mem / 1024);

	int drive_index = 1;

	directory_handle* current_directory = filesystem_get_root_directory(drive_index);

	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d\n", drive_index);
	}

	spawn_process("shell.elf", WAIT_FOR_PROCESS);
	
	for(;;);
}
