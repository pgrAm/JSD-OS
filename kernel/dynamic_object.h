#ifndef DYNAMIC_OBJECT_H
#define DYNAMIC_OBJECT_H
#ifdef __cplusplus

#include <stddef.h>
#include <kernel/util/hash.h>

#include <vector>

typedef struct
{
	void* pointer;
	size_t num_pages;
} segment;

struct dynamic_object
{
	void* entry_point = nullptr;
	std::vector<segment> segments;

	void* linker_data = nullptr;

	hash_map* lib_set = nullptr;
	hash_map* symbol_map = nullptr;
	hash_map* glob_data_symbol_map = nullptr;
};

#endif
#endif