#ifndef IDT_H
#define IDT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <drivers/portio.h>
typedef struct
{
    uint32_t gs, fs, es, ds;      /* pushed the segs last */
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    uint32_t int_no, err_code;    /* our 'push byte #' and ecodes do this */
    uint32_t eip, cs, eflags, sp, ss;   /* pushed by the processor automatically */ 
} __attribute__((packed)) interrupt_info;

#define IDT_INT_PRESENT	0x80
#define IDT_INT_RING(r)	((r << 5) & 0x60)
#define IDT_GATE_INT_32	0x0E

#define IDT_HARDWARE_INTERRUPT (IDT_INT_PRESENT | IDT_INT_RING(0) | IDT_GATE_INT_32)
#define IDT_SOFTWARE_INTERRUPT (IDT_INT_PRESENT | IDT_INT_RING(3) | IDT_GATE_INT_32)
#define IDT_SEGMENT_KERNEL 0x08
#define IDT_SEGMENT_USER 0x18

typedef struct
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
} interrupt_frame;

#define INTERRUPT_HANDLER __attribute__((interrupt))

typedef INTERRUPT_HANDLER void (irq_func)(interrupt_frame* r);

INT_CALLABLE void acknowledge_irq(size_t irq);
void interrupts_init();
void isr_install_handler(size_t vector, irq_func r, bool user);
void isr_uninstall_handler(size_t irq);
void irq_install_handler(size_t irq, irq_func r);
void irq_uninstall_handler(size_t irq);
void irq_enable(size_t irq, bool enabled);

bool irq_is_requested(size_t irq);

#ifdef __cplusplus
}
#endif
#endif