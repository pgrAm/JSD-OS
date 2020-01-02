#include <time.h>
#include <stdio.h>
#include <sys/syscalls.h>

#define IS_LEAP_YEAR(a) ((a % 4 == 0) && ((a % 100) || (a % 400 == 0)))
#define DAYS_IN_YEAR(n) (IS_LEAP_YEAR(n) ? 366 : 365)

#define THE_BEGINNING_OF_TIME 1970 	//throughout this file "the beginning of time" will refer to:
#define THE_MOMENT_OF_CREATION 0 	//January 1, 1970
#define GREGORIAN_CYCLE 146097 		//a gregorian cycle is 146,097 days long
#define YEARS_PER_CYCLE 400
#define LEAP_DAYS_PER_CYCLE 97
#define LEAP_DAYS_BEFORE_EPOCH 477
#define DAYS_BEFORE_EPOCH (365 * THE_BEGINNING_OF_TIME + LEAP_DAYS_BEFORE_EPOCH)
#define DAYS_PER_4_YEARS (365 * 4 + 1)
#define DAYS_PER_100_YEARS (DAYS_PER_4_YEARS * 25 - 1)

static struct tm date_ret;

struct tm* localtime(const time_t* timer)
{
	time_t local_time = (*timer) + (time_t)get_utc_offset();
	
	return gmtime(&local_time);
}

char* ctime(const time_t * timer)
{
	return asctime(localtime(timer));
}

char* asctime(const struct tm *t)
{
	static const char wdays[][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
  
	static const char months[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
  
	static char result[26];
  
	sprintf(result, "%s %s %02d %02d:%02d:%02d %04d\n", wdays[t->tm_wday], months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, t->tm_year + 1900);
  
	return result;
}

time_t time(time_t* timer)
{
	time_t the_current_time = master_time();

	if(timer != NULL)
	{
		*timer = the_current_time;
	}
	
	return the_current_time;
}

clock_t clock(void)
{
	return clock_ticks();
}

const int8_t days_per_month[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

const uint16_t ordinal_date[2][12] = {
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

time_t mktime(struct tm* timeptr)
{
	uint32_t actual_year = timeptr->tm_year + 1900;

	time_t unix_time = THE_MOMENT_OF_CREATION; //set the timestamp to January 1, 1970
	
	unix_time += timeptr->tm_sec;
	unix_time += timeptr->tm_min * 60; 	//60 seconds in a minute
	unix_time += timeptr->tm_hour * 3600; //3600 seconds in an hour
	unix_time += (timeptr->tm_mday - 1) * 86400; //86400 seconds in a day, subtract one to put it in the range 0-30 instead of 1-31

	unix_time += ordinal_date[IS_LEAP_YEAR(actual_year)][timeptr->tm_mon] * 86400;
	
	uint32_t last_year = actual_year - 1;

	//there were 477 leap years between 0 AD and the beginning of time, so subtract that
	unix_time += (((actual_year - THE_BEGINNING_OF_TIME) * 365) + ((last_year / 4) - (last_year / 100) + (last_year / 400) - LEAP_DAYS_BEFORE_EPOCH)) * 86400;
	
	return unix_time;
}

struct tm* gmtime(const time_t* timer)
{
	time_t currtime = *timer; //currtime in seconds

	date_ret.tm_sec = (int)(currtime % 60);
	
    currtime /= 60; //currtime now in minutes
    date_ret.tm_min = (int)(currtime % 60);
	date_ret.tm_sec += (date_ret.tm_sec < 0) ? (date_ret.tm_min--, 60) : 0; //correct for dates before the beginning of time

    currtime /= 60; //currtime now in hours
    date_ret.tm_hour = (int)(currtime % 24);
	date_ret.tm_min += (date_ret.tm_min < 0) ? (date_ret.tm_hour--, 60) : 0; //correct for dates before the beginning of time

	time_t days = currtime / 24; //currtime now in days
	date_ret.tm_wday = (int)((days + 4) % 7);
		
	date_ret.tm_hour += (date_ret.tm_hour < 0) ? (days--, 24) : 0; //correct for dates before the beginning of time
	date_ret.tm_wday += (date_ret.tm_wday < 0) ? 7 : 0; //correct for dates before the beginning of time

	time_t days_since_1bc = DAYS_BEFORE_EPOCH + days + 1;

	int year = YEARS_PER_CYCLE * (days_since_1bc / GREGORIAN_CYCLE);
	int days_since_cycle = days_since_1bc %= GREGORIAN_CYCLE;

	year += 100 * (days_since_cycle / DAYS_PER_100_YEARS);
	days_since_cycle %= DAYS_PER_100_YEARS;

	year += 4 * (days_since_cycle / DAYS_PER_4_YEARS);
	days_since_cycle %= DAYS_PER_4_YEARS;

	year += (days_since_cycle / 365);
	days_since_cycle %= 365;

	date_ret.tm_yday = (int)days_since_cycle;

	uint32_t month = 0; //the current month
	time_t dpm; //number of days in the current month
		
	while(days_since_cycle >= (dpm = (time_t)days_per_month[IS_LEAP_YEAR(year)][month])) //this has a constant running time but it still sucks
	{
		days_since_cycle -= dpm;
		month++;
	}

	date_ret.tm_mday = (int)days_since_cycle + 1;
	date_ret.tm_mon = month;
	date_ret.tm_year = year - 1900;
	
	return &date_ret;
}