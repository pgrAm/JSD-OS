#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "interrupt.h"
#include "../drivers/video.h"
#include "../drivers/portio.h"

struct idt_entry
{
    uint16_t 	address_low;
    uint16_t 	seg_select;
    uint8_t 	always0; 
    uint8_t 	flags;
    uint16_t	address_high;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

/* This exists in 'start.asm', and is used to load our IDT */
extern void idt_load();

void idt_install_handler(uint8_t i, void* address, uint16_t sel, uint8_t flags)
{
	idt[i].flags = flags;
	idt[i].seg_select = sel;
	idt[i].always0 = 0;
	idt[i].address_low = ((uint32_t)address) & 0xffff;
	idt[i].address_high = (((uint32_t)address) >> 16) & 0xffff;
}

/* Installs the IDT */
void idt_init()
{
    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)(&idt);

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    /* Add any new ISRs to the IDT here using idt_set_gate */

    /* Points the processor's internal register to the new IDT */
    idt_load();
}

const char *exception_messages[] =
{
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void *irq_routines[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void irq_install_handler(size_t irq, void (*handler)(interrupt_info *r))
{
    irq_routines[irq] = handler;
}

/* This clears the handler for a given IRQ */
void irq_uninstall_handler(size_t irq)
{
    irq_routines[irq] = 0;
}

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

void irq_remap(void)
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

/* We first remap the interrupt controllers, and then we install
*  the appropriate ISRs to the correct entries in the IDT. This
*  is just like installing the exception handlers */
void irqs_init()
{
	memset(irq_routines, 0, sizeof(void*) * 16); //clear all irqs

    irq_remap();
	
    idt_install_handler(32, irq0, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(33, irq1, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(34, irq2, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(35, irq3, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(36, irq4, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(37, irq5, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(38, irq6, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(39, irq7, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(40, irq8, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(41, irq9, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(42, irq10, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(43, irq11, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(44, irq12, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(45, irq13, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(46, irq14, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	idt_install_handler(47, irq15, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

extern uint32_t getcr2reg(void);

#include <stdio.h>

void send_eoi(interrupt_info * r)
{
	if (r->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}

void fault_handler(interrupt_info *r)
{
    if (r->int_no < 32)
    {
        //set_video_mode(90, 60, VIDEO_TEXT_MODE);
        //clear_screen();

        printf("%s Exception. System Halted!\n", exception_messages[r->int_no]);
		
        printf("GS=%X\n", r->gs);
        printf("FS=%X\n", r->fs);
        printf("ES=%X\n", r->es);
        printf("DS=%X\n", r->ds);
        printf("SS=%X\n", r->ss);
		printf("CS=%X\n", r->cs);
		printf("EIP=%X\n", r->eip);
		printf("ESP=%X\n", r->esp);

		if (r->int_no == 14)
		{
			if(r->err_code & 0x04)
			{
				printf("user ");
			}
			
			if(r->err_code & 0x02)
			{
				printf("write ");
			}
			else
			{
				printf("read ");
			}
				
			if(r->err_code & 0x01)
			{
				printf("protection ");
			}
			else
			{
				printf("invalid page ");
			}
			
			printf("violation\n");
			
			printf("At Adress %X", getcr2reg());
		}
		else if(r->int_no == 13)
		{
			printf("ERR CODE = %X\n", r->err_code);
		}
		
        for (;;);
    }
}

void irq_handler(interrupt_info *r)
{
    /* This	is a blank function pointer */
    void (*handler)(interrupt_info *r);

	//memset(irq_routines, 0, sizeof(void*) * 16);
	
    /* Find out if we have a custom handler to run for this
    *  IRQ, and then finally, run it */
    handler = irq_routines[r->int_no - 32];
	
    if(handler)
    {
        handler(r);
    }

	send_eoi(r);
}

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

/* This is a very repetitive function... it's not hard, it's
*  just annoying. As you can see, we set the first 32 entries
*  in the IDT to the first 32 ISRs. We can't use a for loop
*  for this, because there is no way to get the function names
*  that correspond to that given entry. We set the access
*  flags to 0x8E. This means that the entry is present, is
*  running in ring 0 (kernel level), and has the lower 5 bits
*  set to the required '14', which is represented by 'E' in
*  hex. */
void isrs_init()
{

    idt_install_handler(0, 		isr0, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(1, 		isr1, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(2, 		isr2, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(3, 		isr3, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(4, 		isr4, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(5, 		isr5, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(6, 		isr6, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(7, 		isr7, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	
    idt_install_handler(8, 		isr8, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(9, 		isr9, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(10, 	isr10, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(11, 	isr11, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(12, 	isr12, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(13, 	isr13, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(14, 	isr14, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(15, 	isr15, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    idt_install_handler(16, 	isr16, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(17, 	isr17, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(18, 	isr18, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(19, 	isr19, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(20, 	isr20, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(21, 	isr21, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(22, 	isr22, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(23, 	isr23, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    idt_install_handler(24, 	isr24, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(25, 	isr25, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(26, 	isr26, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(27, 	isr27, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(28, 	isr28, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(29, 	isr29, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(30, 	isr30, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(31, 	isr31, 	IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}