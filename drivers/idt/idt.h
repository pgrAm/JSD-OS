#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <string.h>

struct interrupt_info
{
    uint32_t gs, fs, es, ds;      /* pushed the segs last */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    uint32_t int_no, err_code;    /* our 'push byte #' and ecodes do this */
    uint32_t eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
} __attribute__((packed));

void idt_install_handler(uint8_t i, uint32_t base, uint16_t sel, uint8_t flags);
void idt_init();
void irq_install_handler(size_t irq, void (*handler)(struct interrupt_info *r));
void irq_uninstall_handler(size_t irq);
void isrs_init();
void irqs_init();
#endif