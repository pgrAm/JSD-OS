#ifndef PIT_H
#define PIT_H

#include <kernel/sysclock.h>

#ifdef __cplusplus
extern "C" {
#endif

tick_t pit_get_tick_rate();
tick_t pit_get_ticks();
void pit_init();

#ifdef __cplusplus
}
#endif
#endif