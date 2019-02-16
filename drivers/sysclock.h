#ifndef SYSCLOCK_H
#define SYSCLOCK_H

#include "idt/idt.h"
#include <time.h>

int sysclock_get_utc_offset(void); //returns the UTC offset in seconds
void sysclock_set_utc_offset(int offset);
void sysclock_get_date_time(struct tm* result);
void sysclock_phase(uint32_t hz);
void sysclock_irq(struct interrupt_info *r);
void sysclock_init();
void sysclock_sleep(uint32_t ticks);
clock_t sysclock_get_ticks(void); //return the ticks since the system booted

void sysclock_set_master_time(time_t newtime);
time_t sysclock_get_master_time(void);

#endif