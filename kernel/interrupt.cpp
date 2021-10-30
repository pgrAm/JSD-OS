#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include <kernel/memorymanager.h>
#include <kernel/task.h>
#include <kernel/interrupt.h>
#include <kernel/display.h>
#include <drivers/portio.h>

enum {
    PIC1_COMMAND_PORT = 0x20,
    PIC2_COMMAND_PORT = 0xA0,

    PIC_EOI_CMD = 0x20,
    PIC_GET_IRR_CMD = 0x0A
};

struct __attribute__((packed)) idt_entry
{
    uint16_t 	address_low;
    uint16_t 	seg_select;
    uint8_t 	always0;
    uint8_t 	flags;
    uint16_t	address_high;
};

struct __attribute__((packed)) idt_ptr
{
    uint16_t limit;
    uint32_t base;
};

idt_entry idt[256];
idt_ptr idtp;

extern "C" void idt_load();

void idt_install_handler(uint8_t i, void* address, uint16_t sel, uint8_t flags)
{
    idt[i].flags = flags;
    idt[i].seg_select = sel;
    idt[i].always0 = 0;
    idt[i].address_low = ((uint32_t)address) & 0xffff;
    idt[i].address_high = (((uint32_t)address) >> 16) & 0xffff;
}

const char* exception_messages[] =
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

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security",
    "Reserved"
};

INTERRUPT_HANDLER void irq_stub1(interrupt_frame* r)
{
    acknowledge_irq(0);
}

INTERRUPT_HANDLER void irq_stub2(interrupt_frame* r)
{
    acknowledge_irq(8);
}

void isr_install_handler(size_t vector, irq_func r, bool user)
{
    idt_install_handler(vector, (void*)r, user ? IDT_SEGMENT_USER : IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
}

void isr_uninstall_handler(size_t vector)
{
    idt_install_handler(vector, nullptr, IDT_SEGMENT_KERNEL, 0);
}

void irq_install_handler(size_t irq, irq_func handler)
{
    idt_install_handler(32 + irq, (void*)handler, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

// This clears the handler for a given IRQ
void irq_uninstall_handler(size_t irq)
{
    auto irq_func = irq < 8 ? irq_stub1 : irq_stub2;
    idt_install_handler(32 + irq, (void*)irq_func, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

bool irq_is_requested(size_t irq)
{
    auto port = PIC1_COMMAND_PORT;
    if(irq >= 8)
    {
        port = PIC2_COMMAND_PORT;
        irq -= 8;
    }

    auto irq_mask = 1 << irq;

    outb(port, PIC_GET_IRR_CMD);
    return inb(port) & irq_mask;
}

void acknowledge_irq(size_t irq)
{
    if(irq >= 8)
    {
        outb(PIC2_COMMAND_PORT, PIC_EOI_CMD);
    }
    outb(PIC1_COMMAND_PORT, PIC_EOI_CMD);
}

void irq_remap(void)
{
    outb(PIC1_COMMAND_PORT, 0x11); //Init PIC#1
    outb(PIC2_COMMAND_PORT, 0x11); //Init PIC#2

    outb(PIC1_COMMAND_PORT + 1, 0x20); //PIC#1 sends interrupt 32-39
    outb(PIC2_COMMAND_PORT + 1, 0x28); //PIC#2 sends interrupt 40-47

    outb(PIC1_COMMAND_PORT + 1, 0x04); //Inform PIC#1 that PIC#2 is at 0x02
    outb(PIC2_COMMAND_PORT + 1, 0x02);

    outb(PIC1_COMMAND_PORT + 1, 0x01);
    outb(PIC2_COMMAND_PORT + 1, 0x01);

    outb(PIC1_COMMAND_PORT + 1, 0x0);
    outb(PIC2_COMMAND_PORT + 1, 0x0);
}

extern "C" uint32_t getcr2reg(void);

#include <stdio.h>

extern "C" void fault_handler(interrupt_info * r)
{
    if(r->int_no < 32)
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
            printf("%X\n", r->err_code);
        }
        else if(r->int_no == 14) //page fault
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

extern "C" {
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
}

void idt_init()
{
    // Sets the special IDT pointer up
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)(&idt);

    //Clear out the entire IDT, initializing it to zeros
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    // Points the processor's internal register to the new IDT
    idt_load();
}

void interrupts_init()
{
    idt_init();

    idt_install_handler(0, (void*)isr0, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(1, (void*)isr1, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(2, (void*)isr2, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(3, (void*)isr3, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(4, (void*)isr4, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(5, (void*)isr5, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(6, (void*)isr6, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(7, (void*)isr7, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    idt_install_handler(8, (void*)isr8, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(9, (void*)isr9, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(10, (void*)isr10, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(11, (void*)isr11, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(12, (void*)isr12, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(13, (void*)isr13, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(14, (void*)isr14, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(15, (void*)isr15, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    idt_install_handler(16, (void*)isr16, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(17, (void*)isr17, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(18, (void*)isr18, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(19, (void*)isr19, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(20, (void*)isr20, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(21, (void*)isr21, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(22, (void*)isr22, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(23, (void*)isr23, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    idt_install_handler(24, (void*)isr24, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(25, (void*)isr25, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(26, (void*)isr26, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(27, (void*)isr27, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(28, (void*)isr28, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(29, (void*)isr29, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(30, (void*)isr30, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
    idt_install_handler(31, (void*)isr31, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);

    irq_remap();

    for(size_t i = 0; i < 8; i++)
    {
        irq_install_handler(i, irq_stub1);
    }
    for(size_t i = 8; i < 16; i++)
    {
        irq_install_handler(i, irq_stub2);
    }
}