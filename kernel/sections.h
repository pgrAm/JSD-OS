#ifndef _SECTIONS_H
#define _SECTIONS_H

#define RECLAIMABLE __attribute__((section(".reclaimable_text")))
#define RECLAIMABLE_DATA __attribute__((section(".reclaimable_data")))
#define RECLAIMABLE_BSS __attribute__((section(".reclaimable_bss")))
#endif