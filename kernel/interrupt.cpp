#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <bit>
#include <array>

#include <kernel/memorymanager.h>
#include <kernel/task.h>
#include <kernel/interrupt.h>
#include <kernel/display.h>
#include <kernel/tss.h>
#include <drivers/portio.h>

enum {
	PIC1_COMMAND_PORT = 0x20,
	PIC2_COMMAND_PORT = 0xA0,

	PIC_EOI_CMD = 0x20,
	PIC_GET_IRR_CMD = 0x0A,
	PIC_GET_ISR_CMD = 0x0B
};

struct __attribute__((packed)) idt_entry
{
	uint16_t 	address_low = 0;
	uint16_t 	seg_select = 0;
	uint8_t 	always0 = 0;
	uint8_t 	flags = 0;
	uint16_t	address_high = 0;
};

struct __attribute__((packed)) idt_ptr
{
	uint16_t limit;
	uint32_t base;
};

extern "C" void idt_load();

static constexpr idt_entry idt_make_entry(void* address, uint16_t sel, uint8_t flags)
{
	auto addr = std::bit_cast<uintptr_t>(address);

	return {
		.address_low = (uint16_t)(addr & 0xffff),
		.seg_select = sel,
		.always0 = 0,
		.flags = flags,
		.address_high = (uint16_t)((addr >> 16) & 0xffff)
	};
}

extern "C" {
	extern INTERRUPT_HANDLER void isr0(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr1(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr2(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr3(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr4(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr5(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr6(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr7(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr8(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr9(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr10(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr11(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr12(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr13(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr14(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr15(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr16(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr17(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr18(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr19(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr20(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr21(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr22(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr23(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr24(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr25(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr26(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr27(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr28(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr29(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr30(interrupt_frame* r);
	extern INTERRUPT_HANDLER void isr31(interrupt_frame* r);
}

static INTERRUPT_HANDLER void irq_stub1(interrupt_frame* r)
{
	acknowledge_irq(0);
}

static INTERRUPT_HANDLER void irq_stub2(interrupt_frame* r)
{
	acknowledge_irq(8);
}

static constexpr std::array<irq_func*, 48> handlers = {
	isr0,
	isr1,
	isr2,
	isr3,
	isr4,
	isr5,
	isr6,
	isr7,

	isr8,
	isr9,
	isr10,
	isr11,
	isr12,
	isr13,
	isr14,
	isr15,

	isr16,
	isr17,
	isr18,
	isr19,
	isr20,
	isr21,
	isr22,
	isr23,

	isr24,
	isr25,
	isr26,
	isr27,
	isr28,
	isr29,
	isr30,
	isr31,

	irq_stub1,  //32
	irq_stub1,
	irq_stub1,
	irq_stub1,
	irq_stub1,
	irq_stub1,
	irq_stub1,
	irq_stub1,

	irq_stub2,  //40
	irq_stub2,
	irq_stub2,
	irq_stub2,
	irq_stub2,
	irq_stub2,
	irq_stub2,
	irq_stub2
};

static idt_entry idt[256];

idt_ptr idtp{
	.limit = (sizeof(idt_entry) * 256) - 1,
	.base = (uint32_t)&idt
};

void idt_install_handler(uint8_t i, void* address, uint16_t sel, uint8_t flags)
{
	idt[i] = idt_make_entry(address, sel, flags);
}

static constexpr std::string_view exception_messages[] =
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

void isr_install_handler(uint8_t vector, irq_func r, bool user)
{
	idt_install_handler(vector, (void*)r, user ? IDT_SEGMENT_USER : IDT_SEGMENT_KERNEL, IDT_SOFTWARE_INTERRUPT);
}

void isr_uninstall_handler(uint8_t vector)
{
	idt_install_handler(vector, nullptr, IDT_SEGMENT_KERNEL, 0);
}

void irq_install_handler(uint8_t irq, irq_func handler)
{
	idt_install_handler(32 + irq, (void*)handler, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

// This clears the handler for a given IRQ
void irq_uninstall_handler(uint8_t irq)
{
	auto irq_func = irq < 8 ? irq_stub1 : irq_stub2;
	idt_install_handler(32 + irq, (void*)irq_func, IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
}

void irq_enable(uint8_t irq, bool enabled)
{
	auto port = PIC1_COMMAND_PORT;
	if(irq >= 8)
	{
		port = PIC2_COMMAND_PORT;
		irq -= 8;
	}

	if(enabled)
	{
		outb(port + 1, inb(port + 1) & ~(uint8_t)(1 << irq));
	}
	else
	{
		outb(port + 1, inb(port + 1) | (uint8_t)(1 << irq));
	}
}

bool irq_is_requested(uint8_t irq)
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

INT_CALLABLE void acknowledge_irq(uint8_t irq)
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

static inline uint64_t
rdmsr(uint32_t msrid)
{
	uint32_t low;
	uint32_t high;
	__asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msrid));
	return (uint64_t)low << 0 | (uint64_t)high << 32;
}

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

		display_mode requested = {
			80, 25,
			0,0,0,
			FORMAT_DONT_CARE,
			DISPLAY_TEXT_MODE
		};
		display_mode actual;
		set_display_mode(&requested, &actual);

		print_string("Unhandled ");
		print_string(exception_messages[r->int_no]);
		print_string(" Exception, System Halted!\n");

		printf("GS=%08X FS=%08X\nES=%08X DS=%08X\nSS=%08X CS=%08X\nEIP=%08X ESP=%08X\n", 
			   r->gs, r->fs, r->es, r->ds, r->ss, r->cs, r->eip, r->esp);

		if(r->int_no == 13) //GPF
		{
			printf("EFLAGS=%X\n", r->eflags);
			printf("%X\n", r->err_code);
		}
		else if(r->int_no == 14) //page fault
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

			printf("At address %X\n", getcr2reg());

			printf("GSBASE=%X\n", get_TLS_seg_base());

			//printf("cr3 = %X\n", get_page_directory());
			//printf("flags= %X\n", memmanager_get_page_flags(getcr2reg()));
		}
		else if(r->int_no == 13)
		{
			printf("ERR CODE = %X\n", r->err_code);
		}

		if(r->cs == 0x08)
		{
			print_string("The kernel has crashed, we're boned so just restart\n");
			__asm__ volatile ("cli;hlt");
		}

		__asm__ volatile("cli;hlt");

		exit_process(-1);
	}
}

void interrupts_init()
{
	irq_remap();

	memset(&idt, 0, sizeof(idt));

	static_assert(handlers.size() <= (uint8_t)~0);
	for(size_t i = 0; i < handlers.size(); i++)
	{
		idt_install_handler((uint8_t)i, (void*)handlers[i], IDT_SEGMENT_KERNEL, IDT_HARDWARE_INTERRUPT);
	}

	// Points the processor's internal register to the new IDT
	idt_load();
}