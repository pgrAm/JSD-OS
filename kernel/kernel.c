#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <kernel/filesystem.h>
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/multiboot.h>
#include <kernel/driver_loader.h>
#include <kernel/task.h>
#include <kernel/elf.h>

#include <drivers/video.h>
#include <drivers/sysclock.h>
#include <drivers/ramdisk.h>
#include <drivers/kbrd.h>
#include <drivers/formats/rdfs.h>

extern multiboot_info* _multiboot;

extern void _IMAGE_END_;
extern void _BSS_END_;
extern void _DATA_END_;

typedef void (*func_ptr)(void);
extern func_ptr __init_array_start[], __init_array_end[];
static void handle_init_array(void)
{
	for(func_ptr* func = __init_array_start; func != __init_array_end; func++)
		(*func)();
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

	//call global constructors
	handle_init_array();

	sysclock_init();

	setup_syscalls();

	setup_first_task(); //we are now running as a kernel level task

	ramdisk_init();

	rdfs_init();

	//memmanager_map_page(0x30000, 0, PAGE_PRESENT);
	//memmanager_reserve_physical_memory(0x30000, 0x60000);
	//memmanager_reserve_physical_memory(0x90000, 0x1000);
	keyboard_init();

	load_drivers();

	printf("driver loading complete\n");

	clear_screen();

	size_t free_mem = memmanager_num_bytes_free();
	size_t total_mem = memmanager_mem_size();

	printf("%u KB free / %u KB Memory\n\n", free_mem / 1024, total_mem / 1024);

	spawn_process("shell.elf", WAIT_FOR_PROCESS);

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