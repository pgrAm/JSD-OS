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
	IS_DIR = 0x01u,
	IS_READONLY = 0x02u,
	DRIVER_RESERVED = 0xFFFF0000u
};

enum file_mode
{
	FILE_READ = 0x01,
	FILE_WRITE = 0x02,
	FILE_CREATE = 0x08
};

typedef uint64_t file_size_t;

typedef struct
{
	file_size_t size;
	uint32_t flags;
	time_t time_created;
	time_t time_modified;
	size_t full_path_len;
	char full_path[MAX_PATH];
} file_info;

#ifdef __cplusplus
}

#include <string>
#include <string_view>
#include <algorithm>
#include <ctype.h>

constexpr bool filesystem_names_identical(std::string_view a, std::string_view b)
{
	if(a.size() != b.size())
		return false;
	return std::equal(a.cbegin(), a.cend(), b.cbegin(), [](char c_a, char c_b) {
		return toupper(c_a) == toupper(c_b);
	});
}

inline std::string filesystem_create_path(std::string_view dir,
										  std::string_view name)
{
	using namespace std::literals;

	if(name == "."sv)
		return std::string{dir};
	else if(name == ".."sv)
	{
		auto slash = dir.find_last_of('/');

		return std::string{dir.substr(0, slash)};
	}
	else
		return (std::string{dir} + '/').append(name);
}

#endif

#endif