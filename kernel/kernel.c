#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <kernel/filesystem.h>
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/boot_info.h>
#include <kernel/driver_loader.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/display.h>
#include <kernel/sysclock.h>
#include <kernel/kassert.h>

#include <drivers/ramdisk.h>
#include <drivers/kbrd.h>
#include <drivers/formats/rdfs.h>
#include <drivers/display/basic_text/basic_text.h>

extern void _IMAGE_END_;

typedef void (*func_ptr)(void);
extern func_ptr __init_array_start[], __init_array_end[];
static void handle_init_array(void)
{
	for(func_ptr* func = __init_array_start; func != __init_array_end; func++)
		(*func)();
}

extern void memmanager_print_free_map();

void kernel_main()
{
	parse_boot_info();

	//printf("got here\n");

	interrupts_init();

	physical_memory_init();

	reserve_boot_mem();

	memmanager_init();

	basic_text_init();

	//call global constructors
	handle_init_array();

	printf(	"Found Kernel: %X - %X\n", 
			boot_information.kernel_location, 
			boot_information.kernel_location + boot_information.kernel_size);

	printf(	"Found Ram Disk: %X - %X\n", 
			boot_information.ramdisk_location, 
			boot_information.ramdisk_location + boot_information.ramdisk_size);

	sysclock_init();

	clock_t init_time = sysclock_get_ticks(NULL);

	setup_syscalls();

	setup_first_task(); //we are now running as a kernel level task

	ramdisk_init();

	rdfs_init();

	//memmanager_map_page(0x30000, 0, PAGE_PRESENT);
	//memmanager_reserve_physical_memory(0x30000, 0x60000);
	//memmanager_reserve_physical_memory(0x90000, 0x1000);
	keyboard_init();

	//memmanager_print_free_map();

	/*while(true) {
		printf("%d\n", sysclock_get_ticks(NULL));
	}*/

	load_drivers();

	for(;;);
}

void __cxa_pure_virtual() {
	// Do Nothing
}

#define ATEXIT_FUNC_MAX 128

typedef unsigned uarch_t;

struct atexitFuncEntry_t {
	void (*destructorFunc) (void*);
	void* objPtr;
	void* dsoHandle;

};

struct atexitFuncEntry_t __atexitFuncs[ATEXIT_FUNC_MAX];
uarch_t __atexitFuncCount = 0;

void* __dso_handle = 0;

int __cxa_atexit(void (*f)(void*), void* objptr, void* dso) {
	if(__atexitFuncCount >= ATEXIT_FUNC_MAX) {
		return -1;
	}
	__atexitFuncs[__atexitFuncCount].destructorFunc = f;
	__atexitFuncs[__atexitFuncCount].objPtr = objptr;
	__atexitFuncs[__atexitFuncCount].dsoHandle = dso;
	__atexitFuncCount++;
	return 0;
}

void __cxa_finalize(void* f) {
	signed i = __atexitFuncCount;
	if(!f) {
		while(i--) {
			if(__atexitFuncs[i].destructorFunc) {
				(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
			}
		}
		return;
	}

	for(; i >= 0; i--) {
		if(__atexitFuncs[i].destructorFunc == f) {
			(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
			__atexitFuncs[i].destructorFunc = 0;
		}
	}
}