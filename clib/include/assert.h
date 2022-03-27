#ifndef ASSERT_H
#define ASSERT_H

#ifdef __KERNEL
#include <kernel/kassert.h>
#define assert(exp) k_assert(exp)
#else
#define assert(exp)
#endif

#endif