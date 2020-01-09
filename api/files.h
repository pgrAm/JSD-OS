#ifndef FILES_H
#define FILES_H

#include <stddef.h>
#include <time.h>

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
	char name[MAX_PATH];
} file_info;

#endif