#ifndef DYNAMIC_OBJECT_H
#define DYNAMIC_OBJECT_H

#include <stddef.h>
#include <util/hash.h>

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

	void* linker_data;

	hash_map* lib_set;
	hash_map* symbol_map;
	hash_map* glob_data_symbol_map;
} dynamic_object;

#endif