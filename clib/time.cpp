#include <time.h>
#include <stdio.h>

#ifndef __KERNEL
#include <sys/syscalls.h>
#else
#include <kernel/sys/syscalls.h>
#include <kernel/sysclock.h>
#endif

static inline constexpr int is_leap_year(int a)
{
	return (a % 4 == 0) && ((a % 100) || (a % 400 == 0)) ? 1 : 0;
}
//throughout this file "the beginning of time" will refer to January 1, 1970
constexpr auto beginning_of_time  = 1970;
constexpr auto moment_of_creation = 0; //January 1, 1970
constexpr auto gregorian_cycle_len =
	146097; //a gregorian cycle is 146,097 days long
constexpr auto years_per_cycle = 400;
//constexpr auto LEAP_DAYS_PER_CYCLE	  = 97;
constexpr auto leap_days_before_epoch = 477;
constexpr auto days_per_year		  = 365;
constexpr auto days_before_epoch =
	days_per_year * beginning_of_time + leap_days_before_epoch;
constexpr auto days_per_4_years	  = days_per_year * 4 + 1;
constexpr auto days_per_100_years = days_per_year * 25 - 1;

constexpr auto seconds_per_day	  = 86400;
constexpr auto seconds_per_hour	  = 3600;
constexpr auto seconds_per_minute = 60;

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
	static constexpr char wdays[][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
  
	static constexpr char months[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
  
	static char result[128];

	snprintf(result, 128, "%s %s %02d %02d:%02d:%02d %04d\n", wdays[t->tm_wday], months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, t->tm_year + 1900);
  
	return result;
}

time_t time(time_t* timer)
{
	time_t the_current_time = master_time();

	if(timer != nullptr)
	{
		*timer = the_current_time;
	}
	
	return the_current_time;
}

clock_t __c_get_clock_tick_rate()
{
#ifndef __KERNEL
	size_t rate;
	clock_ticks(&rate);
	return rate;
#else
	return sysclock_get_rate();
#endif
}

clock_t clock(void)
{
#ifndef __KERNEL
	return clock_ticks(nullptr);
#else
	return sysclock_get_ticks();
#endif
}

static constexpr int8_t days_per_month[2][12] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static constexpr uint16_t ordinal_date[2][12] = {
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

time_t mktime(struct tm* timeptr)
{
	int actual_year = timeptr->tm_year + 1900;

	time_t time = moment_of_creation; //set the timestamp to January 1, 1970
	
	time += timeptr->tm_sec;
	time += timeptr->tm_min * seconds_per_minute;
	time += timeptr->tm_hour * seconds_per_hour;
	time += (timeptr->tm_mday - 1) * seconds_per_day;
	time += ordinal_date[is_leap_year(actual_year)][timeptr->tm_mon] * 86400;
	
	int last_year = actual_year - 1;

	//there were 477 leap years between 0 AD and the beginning of time, so subtract that
	time += (((actual_year - beginning_of_time) * 365) + ((last_year / 4) - (last_year / 100) + (last_year / 400) - leap_days_before_epoch)) * 86400;
	
	return time;
}

struct tm* gmtime(const time_t* timer)
{
	const auto seconds = *timer; //currtime in seconds

	date_ret.tm_sec = (int)(seconds % 60);
	
	const auto minutes = seconds / 60;
	date_ret.tm_min = (int)(minutes % 60);
	date_ret.tm_sec += (date_ret.tm_sec < 0) ? (date_ret.tm_min--, 60) : 0; //correct for dates before the beginning of time

	const auto hours = minutes / 60;
	date_ret.tm_hour = (int)(hours % 24);
	date_ret.tm_min += (date_ret.tm_min < 0) ? (date_ret.tm_hour--, 60) : 0; //correct for dates before the beginning of time

	auto days	 = hours / 24;
	date_ret.tm_wday = (int)((days + 4) % 7);
		
	date_ret.tm_hour += (date_ret.tm_hour < 0) ? (days--, 24) : 0; //correct for dates before the beginning of time
	date_ret.tm_wday += (date_ret.tm_wday < 0) ? 7 : 0; //correct for dates before the beginning of time

	time_t days_since_1bc = days_before_epoch + days;

	const int year_of_last_cycle = years_per_cycle * (int)(days_since_1bc / gregorian_cycle_len);
	const int days_since_cycle = days_since_1bc % gregorian_cycle_len;

	const int last_centennial	= year_of_last_cycle 
								+ 100 * (days_since_cycle / days_per_100_years);
	const int days_since_centennial = days_since_cycle % days_per_100_years;

	const int last_leap_year	= last_centennial 
								+ 4 * (days_since_centennial / days_per_4_years);
	const int days_since_leap	= days_since_centennial % days_per_4_years;

	const int current_year		= last_leap_year 
								+ (days_since_leap / days_per_year);
	int day_of_year				= days_since_leap % days_per_year;

	date_ret.tm_yday = day_of_year;

	int month = 0; //the current month
	time_t dpm; //number of days in the current month
		
	while(day_of_year >= (dpm = (time_t)days_per_month[is_leap_year(current_year)][month])) //this has a constant running time but it still sucks
	{
		day_of_year -= dpm;
		month++;
	}

	date_ret.tm_mday = (int)day_of_year + 1;
	date_ret.tm_mon = month;
	date_ret.tm_year = current_year - 1900;
	
	return &date_ret;
}