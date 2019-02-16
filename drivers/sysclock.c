#include "idt/idt.h"

#include <stdio.h>
#include <time.h>

#include "cmos.h"
#include "sysclock.h"

static volatile clock_t timer_ticks = 0;	//This represents the number of PIT ticks since bootup 
										//It should be used for timing NOT timekeeping

static volatile time_t sysclock_master_time = 0; 	//Epoch time, counting seconds since January 1st 1970
													//This will serve as our master system clock
													//It should be 64 bits and won't overflow until the end of time

static int utc_offset = -5*3600; //5 hours in seconds

struct tm sysclock_time;

void sysclock_set_master_time(time_t newtime)
{
	sysclock_master_time = newtime;
}

time_t sysclock_get_master_time(void)
{
	return sysclock_master_time;
}

clock_t sysclock_get_ticks(void)
{
	return timer_ticks;
}

void sysclock_set_utc_offset(int offset)
{
	utc_offset = offset;
}

int sysclock_get_utc_offset(void)
{
	return utc_offset;
}

void sysclock_phase(uint32_t hz)
{
    uint32_t divisor = 1193180 / hz;     	/* Calculate our divisor */
    outb(0x43, 0x36);             			/* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);  			/* Set low byte of divisor */
    outb(0x40, divisor >> 8);     			/* Set high byte of divisor */
}

void sysclock_irq(struct interrupt_info *r)
{
    timer_ticks++;
	
	if(timer_ticks % CLOCKS_PER_SEC == 0)
	{
		sysclock_master_time++;
	}
}

/* Sets up the system clock by installing the timer handler
*  into IRQ0 */
void sysclock_init()
{
	timer_ticks = 0;
    
    irq_install_handler(0, sysclock_irq); //setup our PIT interrupt to sound off every tick
	
	sysclock_phase(CLOCKS_PER_SEC); //set our the PIT interrupt rate to once per millisecond
	
	sysclock_get_date_time(&sysclock_time); //read time values from the RTC
	
	sysclock_set_master_time(mktime(&sysclock_time)); //Set our epoch time based on what we read from the RTC
}

void sysclock_sleep(uint32_t ticks)
{
    uint32_t eticks;
	
    eticks = timer_ticks + ticks;
    while(timer_ticks < eticks);
}

uint8_t century_register = 0;

int sysclock_get_weekday(int month, int day, int year) //returns the day of the week using Sakamoto's Algorithm
{
	static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	year -= month < 3;
    return (year + year/4 - year/100 + year/400 + t[month - 1] + day) % 7;
} 

void sysclock_get_date_time(struct tm* result)
{
	uint8_t century;
    uint8_t last_second;
    uint8_t last_minute;
    uint8_t last_hour;
    uint8_t last_day;
    uint8_t last_month;
    uint8_t last_year;
    uint8_t last_century;
    //uint8_t registerB;

	while(cmos_get_update_flag());
	
	if(century_register != 0) 
	{
		century = cmos_get_register(century_register);
    }
	
	result->tm_sec = cmos_get_register(0x00);
    result->tm_min = cmos_get_register(0x02);
	result->tm_hour = cmos_get_register(0x04);
    result->tm_mday = cmos_get_register(0x07);
    result->tm_mon = cmos_get_register(0x08);
	result->tm_year = cmos_get_register(0x09);
	
	do 
	{
		last_second = result->tm_sec;
		last_minute = result->tm_min;
		last_hour = result->tm_hour;
		last_day = result->tm_mday;
		last_month = result->tm_mon;
		last_year = result->tm_year;
		last_century = century;
	
		while(cmos_get_update_flag());
		
		result->tm_sec = cmos_get_register(0x00);
		result->tm_min = cmos_get_register(0x02);
		result->tm_hour = cmos_get_register(0x04);
		result->tm_mday = cmos_get_register(0x07);
		result->tm_mon = cmos_get_register(0x08);
		result->tm_year = cmos_get_register(0x09);
		
		if(century_register != 0) 
		{
			century = cmos_get_register(century_register);
		}	
    } 
	while(	(last_second != result->tm_sec) ||
			(last_minute != result->tm_min) || 
			(last_hour != result->tm_hour) 	||
			(last_day != result->tm_mday) 	||
			(last_month != result->tm_mon) 	|| 
			(last_year != result->tm_year) 	||
			(last_century != century));
	
	uint8_t registerB = cmos_get_register(0x0B);
	
	if(!(registerB & 0x04)) //if we got values as BCD, then convert to binary
	{
		result->tm_sec 	= 	(result->tm_sec & 0x0F) 	+ ((result->tm_sec / 16) * 10);
		result->tm_min 	= 	(result->tm_min & 0x0F) 	+ ((result->tm_min / 16) * 10);
		result->tm_hour = 	((result->tm_hour & 0x0F) 	+ (((result->tm_hour & 0x70) / 16) * 10) ) | (result->tm_hour & 0x80);
		result->tm_mday = 	(result->tm_mday & 0x0F) 	+ ((result->tm_mday / 16) * 10);
		result->tm_mon	= 	(result->tm_mon & 0x0F) 	+ ((result->tm_mon / 16) * 10);
		result->tm_year = 	(result->tm_year & 0x0F) 	+ ((result->tm_year / 16) * 10);
		
		if(century_register != 0) 
		{
			century = (century & 0x0F) + ((century / 16) * 10);
		}
    }
	
	if(!(registerB & 0x02) && (result->tm_hour & 0x80)) 
	{
		result->tm_hour = ((result->tm_hour & 0x7F) + 12) % 24;
    }

    if(century_register != 0) 
	{
		result->tm_year += century * 100;
    } 
	else 
	{
		result->tm_year += (2014 / 100) * 100;
		
		if(result->tm_year < 2014)
		{
			result->tm_year += 100;
		}
    }
	
	result->tm_wday = sysclock_get_weekday(result->tm_mon, result->tm_mday, result->tm_year);
	
	result->tm_mon -= 1;
	result->tm_year -= 1900;
}