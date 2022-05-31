#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss;

tss* create_TSS(uintptr_t stack_addr);
void reload_TLS_seg();
uintptr_t get_TLS_seg_base();
void set_TLS_seg_base(uintptr_t val);

#endif