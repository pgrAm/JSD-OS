#ifndef LAPIC_H
#define LAPIC_H

#include <vector>

struct cpu_core
{
	size_t lapic_id;
};

void init_smp(uintptr_t lapic_phys_addr, std::vector<cpu_core>& cores);

#endif
