#ifndef TIME_H
#define TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef NULL
#define NULL (void*)0
#endif

typedef int64_t time_t;  
typedef time_t clock_t;

struct tm 
{
	int tm_sec;		// seconds after the minute — [0, 60]
	int tm_min; 	// minutes after the hour — [0, 59]
	int tm_hour; 	// hours since midnight — [0, 23]
	int tm_mday;	// day of the month — [1, 31]
	int tm_mon;		// months since January — [0, 11]
	int tm_year; 	// years since 1900
	int tm_wday; 	// days since Sunday — [0, 6]
	int tm_yday; 	// days since January 1 — [0, 365]
	int tm_isdst; 	// Daylight Saving Time flag
};

#define CLOCKS_PER_SEC 1000

clock_t clock(void);

time_t mktime(struct tm* timeptr);

time_t time(time_t* timer);

struct tm* gmtime (const time_t* timer);

char* asctime(const struct tm *timeptr);
char* ctime (const time_t * timer);

struct tm* localtime(const time_t* timer);

#ifdef __cplusplus
}
#endif

#endif