#ifndef SYSCLOCK_H
#define SYSCLOCK_H
#ifdef __cplusplus
extern "C" {
#endif
#include <time.h>

#include <kernel/syscall.h>

void sysclock_set_utc_offset(int offset);
void sysclock_init();

typedef enum {
	SECONDS = 1,
	MILLISECONDS = SECONDS * 1000,
	MICROSECONDS = MILLISECONDS * 1000
} clock_unit;

typedef uint64_t tick_t;

void sysclock_sleep(size_t time, clock_unit unit);

clock_t sysclock_get_ticks();
size_t sysclock_get_rate();
SYSCALL_HANDLER time_t sysclock_get_master_time(void);

SYSCALL_HANDLER clock_t syscall_get_ticks(size_t* rate); //return the ticks since the system booted
SYSCALL_HANDLER int sysclock_get_utc_offset(void); //returns the UTC offset in seconds

#ifdef __cplusplus
}
#endif
#endif