#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/bootstrap/boot_info.h>
#include <kernel/driver_loader.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/display.h>
#include <kernel/sysclock.h>
#include <kernel/kassert.h>

#include <drivers/ramdisk.h>
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

extern void _RECLAIMABLE_CODE_BEGIN_, _RECLAIMABLE_CODE_END_;
extern void _RECLAIMABLE_DATA_BEGIN_, _RECLAIMABLE_DATA_END_;
extern void _RECLAIMABLE_BSS_BEGIN_, _RECLAIMABLE_BSS_END_;

RECLAIMABLE void init_kernel_state() __attribute__((noinline))
{
	parse_boot_info();

	setup_boot_cpu();

	interrupts_init();

	reserve_boot_mem();

	physical_memory_init();

	memmanager_init();

	//call global constructors
	handle_init_array();

	setup_first_task(); //we are now running as a kernel level task
}
extern void print_free_map();

void kernel_main()
{
	init_kernel_state();

	//jettison the boostrap code
	physical_memory_free(memmanager_get_physical(&_RECLAIMABLE_CODE_BEGIN_),
						 &_RECLAIMABLE_CODE_END_ - &_RECLAIMABLE_CODE_BEGIN_);
	physical_memory_free(memmanager_get_physical(&_RECLAIMABLE_DATA_BEGIN_),
						 &_RECLAIMABLE_DATA_END_ - &_RECLAIMABLE_DATA_BEGIN_);
	physical_memory_free(memmanager_get_physical(&_RECLAIMABLE_BSS_BEGIN_),
						 &_RECLAIMABLE_BSS_END_ - &_RECLAIMABLE_BSS_BEGIN_);
	
	basic_text_init();

	printf(	"Found Kernel: %X - %X\n", 
			boot_information.kernel_location, 
			boot_information.kernel_location + boot_information.kernel_size);

	printf(	"Found Ram Disk: %X - %X\n", 
			boot_information.ramdisk_location, 
			boot_information.ramdisk_location + boot_information.ramdisk_size);

	sysclock_init();

	setup_syscalls();

	ramdisk_init();

	rdfs_init();

	load_drivers();

	for(;;)
	{
		__asm__ volatile("hlt");
	}
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
	size_t i = __atexitFuncCount;
	if(!f)
	{
		while(i--)
		{
			if(__atexitFuncs[i].destructorFunc)
			{
				(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
			}
		}
		return;
	}

	while(i--)
	{
		if(__atexitFuncs[i].destructorFunc == f)
		{
			(*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
			__atexitFuncs[i].destructorFunc = 0;
		}
	}
}