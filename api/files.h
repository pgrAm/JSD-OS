#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_PATH 255

enum file_flags
{
	IS_DIR = 0x01
};

typedef struct
{
	size_t size;
	uint32_t flags;
	time_t time_created;
	time_t time_modified;
	size_t name_len;
	char name[MAX_PATH];
} file_info;

#ifdef __cplusplus
}
#include <string_view>
#include <algorithm>
#include <ctype.h>

inline bool filesystem_names_identical(std::string_view a, std::string_view b)
{
	if(a.size() != b.size())
		return false;
	return std::equal(a.cbegin(), a.cend(), b.cbegin(), [](char c_a, char c_b) {
		return toupper(c_a) == toupper(c_b);
	});
}
#endif

#endif