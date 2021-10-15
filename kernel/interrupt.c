#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <kernel/memorymanager.h>
#include <kernel/task.h>
#include <kernel/interrupt.h>
#include <kernel/display.h>
#include <drivers/portio.h>

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

// This exists in 'interrupt.asm', and is used to load our IDT
extern void idt_load();

void idt_install_handler(uint8_t i, void* address, uint16_t sel, uint8_t flags)
{
	idt[i].flags = flags;
	idt[i].seg_select = sel;
	idt[i].always0 = 0;
	idt[i].address_low = ((uint32_t)address) & 0xffff;
	idt[i].address_high = (((uint32_t)address) >> 16) & 0xffff;
}

void idt_init()
{
    // Sets the special IDT pointer up
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)(&idt);

    //Clear out the entire IDT, initializing it to zeros
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    // Points the processor's internal register to the new IDT
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
    "SIMD Floating-Point",
    "Virtualization",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reservedt",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security",
    "Reserved"
};

INTERRUPT_HANDLER void irq_stub(interrupt_frame* r)
{
    send_eoi(41);
}

void irq_install_handler(size_t irq, irq_func handler)
{
    idt_install_handler(32 + irq, handler, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

// This clears the handler for a given IRQ
void irq_uninstall_handler(size_t irq)
{
    idt_install_handler(32 + irq, irq_stub, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

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
    irq_remap();
	
    for (size_t i = 32; i < 48; i++)
    {
        idt_install_handler(32, irq_stub, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    }
}

extern uint32_t getcr2reg(void);

#include <stdio.h>

void fault_handler(interrupt_info*r)
{
    if (r->int_no < 32)
    {
        if(r->int_no == 14 && 
           memmanager_handle_page_fault(r->err_code, getcr2reg())) //page fault
        {
            return; //page fault handled, we can resume execution
        }

        //set_video_mode(90, 60, VIDEO_TEXT_MODE);
        //clear_screen();

        printf("Unhandled %s Exception. System Halted!\n", exception_messages[r->int_no]);
		
        printf("GS=%X\n", r->gs);
        printf("FS=%X\n", r->fs);
        printf("ES=%X\n", r->es);
        printf("DS=%X\n", r->ds);
        printf("SS=%X\n", r->ss);
		printf("CS=%X\n", r->cs);
		printf("EIP=%X\n", r->eip);
		printf("ESP=%X\n", r->esp);

        if(r->int_no == 13) //GPF
        {
            printf("EFLAGS=%X\n", r->eflags);
        }
		else if (r->int_no == 14) //page fault
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
			
			printf("At address %X\n", getcr2reg());
            //printf("cr3 = %X\n", get_page_directory());
            //printf("flags= %X\n", memmanager_get_page_flags(getcr2reg()));
		}
		else if(r->int_no == 13)
		{
			printf("ERR CODE = %X\n", r->err_code);
		}
		
        if(r->cs == 0x08)
        {
            puts("The kernel has crashed, we're boned so just restart\n");
            __asm__ volatile ("cli;hlt");
        }

        exit_process(-1);
    }
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