#include <stdio.h>
#include <time.h>

#include <kernel/sysclock.h>
#include <drivers/cmos.h>

static uint8_t century_register = 0;

static int cmos_get_weekday(int month, int day, int year) //returns the day of the week using Sakamoto's Algorithm
{
	static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	year -= month < 3;
	return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

clock_t cmos_get_date_time(struct tm* result)
{
	while(cmos_get_update_flag());

	clock_t sample_time = sysclock_get_ticks();

	result->tm_sec = cmos_get_register(0x00);
	result->tm_min = cmos_get_register(0x02);
	result->tm_hour = cmos_get_register(0x04);
	result->tm_mday = cmos_get_register(0x07);
	result->tm_mon = cmos_get_register(0x08);
	result->tm_year = cmos_get_register(0x09);

	uint8_t century = 0;
	if(century_register != 0)
	{
		century = cmos_get_register(century_register);
	}

	uint8_t last_second;
	uint8_t last_minute;
	uint8_t last_hour;
	uint8_t last_day;
	uint8_t last_month;
	uint8_t last_year;
	uint8_t last_century;

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
	} while((last_second != result->tm_sec) ||
		   (last_minute != result->tm_min) ||
		   (last_hour != result->tm_hour) ||
		   (last_day != result->tm_mday) ||
		   (last_month != result->tm_mon) ||
		   (last_year != result->tm_year) ||
		   (last_century != century));

	uint8_t registerB = cmos_get_register(0x0B);

	if(!(registerB & 0x04)) //if we got values as BCD, then convert to binary
	{
		result->tm_sec = (result->tm_sec & 0x0F) + ((result->tm_sec / 16) * 10);
		result->tm_min = (result->tm_min & 0x0F) + ((result->tm_min / 16) * 10);
		result->tm_hour = ((result->tm_hour & 0x0F) + (((result->tm_hour & 0x70) / 16) * 10)) | (result->tm_hour & 0x80);
		result->tm_mday = (result->tm_mday & 0x0F) + ((result->tm_mday / 16) * 10);
		result->tm_mon = (result->tm_mon & 0x0F) + ((result->tm_mon / 16) * 10);
		result->tm_year = (result->tm_year & 0x0F) + ((result->tm_year / 16) * 10);

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

	result->tm_wday = cmos_get_weekday(result->tm_mon, result->tm_mday, result->tm_year);

	result->tm_mon -= 1;
	result->tm_year -= 1900;

	return sample_time;
}