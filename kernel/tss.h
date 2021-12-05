#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss;

tss* create_TSS(uintptr_t stack_addr);

#endif