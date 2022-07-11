#ifndef CPU_H
#define CPU_H

#include <kernel/tss.h>

struct cpu_state
{
	cpu_state* self = this;
	arch_data arch;
};

#endif