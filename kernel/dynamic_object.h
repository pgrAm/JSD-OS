#ifndef DYNAMIC_OBJECT_H
#define DYNAMIC_OBJECT_H
#ifdef __cplusplus

#include <stddef.h>
#include <kernel/util/hash.h>
#include <common/task_data.h>

#include <vector>

struct segment
{
	void* pointer;
	size_t num_pages;
};

struct dynamic_object
{
	using sym_map = hash_map<std::string_view, uintptr_t>;

	void* entry_point = nullptr;
	std::vector<segment> segments;

	tls_image_data tls_image = {
		.alignment	= 1,
		.image_size = 0,
		.total_size = 0,
		.pointer	= nullptr,
	};

	void* linker_data = nullptr;

	sym_map* lib_set = nullptr;
	sym_map* symbol_map = nullptr;
	sym_map* glob_data_symbol_map = nullptr;
	
	constexpr dynamic_object(sym_map* libs, sym_map* symbols, sym_map* glob_data)
		: lib_set(libs)
		, symbol_map(symbols)
		, glob_data_symbol_map(glob_data) {}
};

#endif
#endif