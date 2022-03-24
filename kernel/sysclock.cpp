#include <stdio.h>
#include <time.h>

#include <kernel/interrupt.h>
#include <kernel/locks.h>
#include <drivers/pit.h>
#include <drivers/cmos.h>

#include "sysclock.h"

clock_t sysclock_get_date_time(struct tm* result);

static volatile clock_t timer_ticks = 0;	//This represents the number of PIT ticks since bootup 
											//It should be used for timing NOT timekeeping

static volatile time_t sysclock_begin_time = 0; 	//Epoch time, counting seconds since January 1st 1970
													//This will serve as our master system clock
													//It should be 64 bits and won't overflow until the end of time

static int utc_offset = -5 * 3600; //5 hours in seconds

struct tm sysclock_time;

size_t sysclock_get_rate()
{
	return pit_get_tick_rate();
}

clock_t sysclock_get_ticks()
{
	return pit_get_ticks();
}

SYSCALL_HANDLER time_t sysclock_get_master_time(void)
{
	return sysclock_begin_time + (pit_get_ticks() / pit_get_tick_rate());
}

SYSCALL_HANDLER clock_t syscall_get_ticks(size_t* rate)
{
	if(rate != NULL)
	{
		*rate = sysclock_get_rate();
	}

	return sysclock_get_ticks();
}

void sysclock_set_utc_offset(int offset)
{
	utc_offset = offset;
}

int sysclock_get_utc_offset(void)
{
	return utc_offset;
}

// Sets up the system clock
void sysclock_init()
{
	timer_ticks = 0;

	pit_init();

	const auto tick_rate = pit_get_tick_rate();

	const clock_t sample = cmos_get_date_time(&sysclock_time); //read time values from the RTC

	const time_t rtc_time = mktime(&sysclock_time); //Set our epoch time based on what we read from the RTC

	sysclock_begin_time = rtc_time - (sample / tick_rate);
}

void sysclock_sleep(size_t time, clock_unit unit)
{
	clock_t begin = sysclock_get_ticks();
	clock_t timer_end = begin + (time * pit_get_tick_rate()) / unit;
	while(sysclock_get_ticks() < timer_end);
}

