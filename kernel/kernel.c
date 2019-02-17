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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

extern struct multiboot_info* _multiboot;

struct tm sys_time;

extern uint32_t getEIP(void);

//extern uint32_t a20_enable;

__attribute__((interrupt)) void syscall(struct interrupt_frame* frame)
{
    puts("Syscall successfull!");
}

void kernel_main() 
{
	initialize_video();

	puts("Kernel Booted\n");
	
	idt_init();
	isrs_init();
	irqs_init();
	
	idt_install_handler(0x80, syscall, IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
	
	//printf("EIP=%X\n", getEIP());
	//while(1);
	//
	//printf("memset=%X\n", (uint32_t)&memset);
	//printf("strlen=%X\n", (uint32_t)&strlen);
	
	memmanager_init();
	
	keyboard_init();
	sysclock_init();
	
	floppy_init();

	filesystem_setup_drives();

	clear_screen();
	
	//uint8_t* p = memmanager_allocate_pages(16);
	
	//p[0] = 1;
	
	//return;
	
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
	
	printf("%u KB Low Memory\n%u KB High Memory\n\n", _multiboot->m_memoryLo, (_multiboot->m_memoryHi));
	
	sys_time = *localtime(&t_time);
	
	printf("EST Time: %s\n", asctime(&sys_time));
	
	FILE* rs232 = fopen("COM1:", "w");
	
	if(rs232 != NULL)
	{
		fprintf(rs232, "test rs232 %d", time(NULL));
	}

	enter_console();
	
	for(;;);
}
