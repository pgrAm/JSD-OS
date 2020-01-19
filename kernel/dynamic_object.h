#ifndef DYNAMIC_OBJECT_H
#define DYNAMIC_OBJECT_H

#include <stddef.h>

typedef struct
{
	void* pointer;
	size_t num_pages;
} segment;

typedef struct
{
	void* entry_point;
	segment* segments;
	size_t num_segments;
	void* dynamic_section;
	void* sym_table;
} dynamic_object;

#endif