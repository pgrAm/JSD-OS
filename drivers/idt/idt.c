#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "idt.h"
#include "../video.h"
#include "../portio.h"

struct idt_entry
{
    uint16_t 	base_lo;
    uint16_t 	sel;        /* Our kernel segment goes here! */
    uint8_t 	always0;     /* This will ALWAYS be set to 0! */
    uint8_t 	flags;       /* Set using the above table! */
    uint16_t	base_hi;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* Declare an IDT of 256 entries. Although we will only use the
*  first 32 entries in this tutorial, the rest exists as a bit
*  of a trap. If any undefined IDT entry is hit, it normally
*  will cause an "Unhandled Interrupt" exception. Any descriptor
*  for which the 'presence' bit is cleared (0) will generate an
*  "Unhandled Interrupt" exception */
struct idt_entry idt[256];
struct idt_ptr idtp;

/* This exists in 'start.asm', and is used to load our IDT */
extern void idt_load();

void idt_setup_handler(uint8_t i, uint32_t base, uint16_t sel, uint8_t flags)
{
	idt[i].flags = flags;
	idt[i].sel = sel;
	idt[i].always0 = 0;
	idt[i].base_lo = base & 0xffff;
	idt[i].base_hi = (base >> 16) & 0xffff;
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

void irq_install_handler(size_t irq, void (*handler)(struct interrupt_info *r))
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
	
    idt_setup_handler(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_setup_handler(33, (uint32_t)irq1, 0x08, 0x8E);
    idt_setup_handler(34, (uint32_t)irq2, 0x08, 0x8E);
	idt_setup_handler(35, (uint32_t)irq3, 0x08, 0x8E);
	idt_setup_handler(36, (uint32_t)irq4, 0x08, 0x8E);
    idt_setup_handler(37, (uint32_t)irq5, 0x08, 0x8E);
    idt_setup_handler(38, (uint32_t)irq6, 0x08, 0x8E);
	idt_setup_handler(39, (uint32_t)irq7, 0x08, 0x8E);
    idt_setup_handler(40, (uint32_t)irq8, 0x08, 0x8E);
	idt_setup_handler(41, (uint32_t)irq9, 0x08, 0x8E);
    idt_setup_handler(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_setup_handler(43, (uint32_t)irq11, 0x08, 0x8E);
	idt_setup_handler(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_setup_handler(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_setup_handler(46, (uint32_t)irq14, 0x08, 0x8E);
	idt_setup_handler(47, (uint32_t)irq15, 0x08, 0x8E);
}

extern uint32_t getcr2reg(void);

#include <stdio.h>

/* All of our Exception handling Interrupt Service Routines will
*  point to this function. This will tell us what exception has
*  happened! Right now, we simply halt the system by hitting an
*  endless loop. All ISRs disable interrupts while they are being
*  serviced as a 'locking' mechanism to prevent an IRQ from
*  happening and messing up kernel data structures */
void fault_handler(struct interrupt_info *r)
{
    if (r->int_no < 32)
    {
        print_string(exception_messages[r->int_no]);
        print_string(" Exception. System Halted!\n");
		
		printf("CS=%X\n", r->cs);
		printf("EIP=%X\n", r->eip);
		printf("ESP=%X\n", r->esp);
		
		if (r->int_no == 14)
		{
			if(r->err_code & 0x04)
			{
				print_string("user ");
			}
			
			if(r->err_code & 0x02)
			{
				print_string("write ");
			}
			else
			{
				print_string("read ");
			}
				
			if(r->err_code & 0x01)
			{
				print_string("protection ");
			}
			else
			{
				print_string("invalid page ");
			}
			
			print_string("violation\n");
			
			printf("At Adress %X", getcr2reg());
		}
		else if(r->int_no == 13)
		{
			printf("ERR CODE = %X\n", r->err_code);
		}
		
        for (;;);
    }
}

void irq_handler(struct interrupt_info *r)
{
    /* This	is a blank function pointer */
    void (*handler)(struct interrupt_info *r);

	//memset(irq_routines, 0, sizeof(void*) * 16);
	
    /* Find out if we have a custom handler to run for this
    *  IRQ, and then finally, run it */
    handler = irq_routines[r->int_no - 32];
	
    if(handler)
    {
        handler(r);
    }

    /* If the IDT entry that was invoked was greater than 40
    *  (meaning IRQ8 - 15), then we need to send an EOI to
    *  the slave controller */
    if (r->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    /* In either case, we need to send an EOI to the master
    *  interrupt controller too */
    outb(0x20, 0x20);
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

    idt_setup_handler(0, (uint32_t)isr0, 0x08, 0x8E);
    idt_setup_handler(1, (uint32_t)isr1, 0x08, 0x8E);
    idt_setup_handler(2, (uint32_t)isr2, 0x08, 0x8E);
    idt_setup_handler(3, (uint32_t)isr3, 0x08, 0x8E);
    idt_setup_handler(4, (uint32_t)isr4, 0x08, 0x8E);
    idt_setup_handler(5, (uint32_t)isr5, 0x08, 0x8E);
    idt_setup_handler(6, (uint32_t)isr6, 0x08, 0x8E);
    idt_setup_handler(7, (uint32_t)isr7, 0x08, 0x8E);

    idt_setup_handler(8, (uint32_t)isr8, 0x08, 0x8E);
    idt_setup_handler(9, (uint32_t)isr9, 0x08, 0x8E);
    idt_setup_handler(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_setup_handler(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_setup_handler(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_setup_handler(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_setup_handler(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_setup_handler(15, (uint32_t)isr15, 0x08, 0x8E);

    idt_setup_handler(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_setup_handler(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_setup_handler(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_setup_handler(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_setup_handler(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_setup_handler(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_setup_handler(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_setup_handler(23, (uint32_t)isr23, 0x08, 0x8E);

    idt_setup_handler(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_setup_handler(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_setup_handler(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_setup_handler(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_setup_handler(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_setup_handler(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_setup_handler(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_setup_handler(31, (uint32_t)isr31, 0x08, 0x8E);
}