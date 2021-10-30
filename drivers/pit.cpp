#include <stdio.h>
#include <time.h>

#include <kernel/sysclock.h>
#include <kernel/interrupt.h>
#include <kernel/locks.h>
#include "pit.h"

#define PIT_TICK_RATE 1193182

//1 tick = 1 / 1193182 seconds
static uint16_t pit_timer_divisor = 0xFFFF;
static tick_t pit_time_elapsed_count = 0;

static inline uint16_t read_pit_counter()
{
	uint16_t val;

	__asm__ volatile("mov $0x00, %%al\n"
					 "out %%al, $0x43\n"
					 "in $0x40, %%al\n"
					 "mov %%al, %%ah\n"
					 "in $0x40, %%al\n"
					 "xchg %%al, %%ah\n"
					 : "=a"(val)
					 :
					 : );
	return val;
}

tick_t pit_get_ticks()
{
	int_lock l = lock_interrupts();

	uint16_t pit_counter = read_pit_counter();

	//checks if we missed an interrupt, if so just pretend we are measuring from precisely the tick before the interrupt would have happened
	//this is needed because:
	//if the PIT counter rolls over between the time when we disable interrupts and when we read it's value
	//then the timer will go backward and that is very bad
	if(irq_is_requested(0)) //check the interrupt request register for irq0
	{
		pit_counter = 0;
	}

	uint16_t count = (pit_timer_divisor - pit_counter);

	tick_t time_val = pit_time_elapsed_count + count;

	unlock_interrupts(l);

	return time_val;
}

static void pit_set_irq_period(uint32_t divisor)
{
	outb(0x43, 0x36);             			// Set our command byte 0x36
	outb(0x40, divisor & 0xFF);  			// Set low byte of divisor
	outb(0x40, divisor >> 8);     			// Set high byte of divisor

	pit_timer_divisor = divisor - 1;
}

static INTERRUPT_HANDLER void pit_irq(interrupt_frame* r)
{
	pit_time_elapsed_count += pit_timer_divisor;

	acknowledge_irq(0);
}

tick_t pit_get_tick_rate()
{
	return PIT_TICK_RATE;
}

void pit_init()
{
	pit_set_irq_period(0xFFFF); //set our the PIT interrupt rate to minimum speed
	irq_install_handler(0, pit_irq);
}