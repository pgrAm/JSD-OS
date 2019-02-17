#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <string.h>

struct interrupt_frame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
} __attribute__((packed));

struct interrupt_info
{
    uint32_t gs, fs, es, ds;      /* pushed the segs last */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    uint32_t int_no, err_code;    /* our 'push byte #' and ecodes do this */
    uint32_t eip, cs, eflags, sp, ss;   /* pushed by the processor automatically */ 
} __attribute__((packed));

#define IDT_INT_PRESENT	0x80
#define IDT_INT_RING(r)	((r << 5) & 0x60)
#define IDT_GATE_INT_32	0x0E

#define IDT_HARDWARE_INTERRUPT (IDT_INT_PRESENT | IDT_INT_RING(0) | IDT_GATE_INT_32)
#define IDT_SOFTWARE_INTERRUPT (IDT_INT_PRESENT | IDT_INT_RING(3) | IDT_GATE_INT_32)
#define IDT_SEGMENT_KERNEL 0x08

void idt_install_handler(uint8_t i, void* address, uint16_t sel, uint8_t flags);
void idt_init();
void irq_install_handler(size_t irq, void (*handler)(struct interrupt_info *r));
void irq_uninstall_handler(size_t irq);
void isrs_init();
void irqs_init();
#endif