#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <time.h>
#include <stdbool.h>

#include "syscall.h"

#include <api/files.h>

typedef size_t fs_index; 

//information about a file on disk
typedef struct 
{
	char* name;
	char* type;
	
	char* full_name;
	
	size_t disk;
	fs_index location_on_disk;
	
	size_t size;
	
	time_t time_created;
	time_t time_modified;
} 
file_handle;

//an instance of an open file
typedef struct 
{
	file_handle* file;
	uint8_t* buffer;
	size_t seekpos;
	fs_index location_on_disk;
} 
file_stream;	

//information about a directory on disk
typedef struct 
{
	const char* name;
	size_t num_files;
	
	file_handle* file_list;
	size_t drive;
}
directory_handle;

struct filesystem_driver;
typedef struct filesystem_driver filesystem_driver;

typedef struct 
{
	bool mounted;
	directory_handle root;
	void* impl_data;
	filesystem_driver* driver;
}
filesystem_drive;

SYSCALL_HANDLER directory_handle* filesystem_get_root_directory(size_t drive);

void filesystem_seek_file(file_stream* f, size_t pos);

inline size_t filesystem_get_size(file_stream* f)
{
	return f->file->size;
}

SYSCALL_HANDLER int filesystem_get_file_info(file_info* dst, const file_handle* src);
SYSCALL_HANDLER file_handle* filesystem_get_file_in_dir(const directory_handle* d, size_t index);
SYSCALL_HANDLER directory_handle* filesystem_open_directory(const char* name, int flags);
SYSCALL_HANDLER file_stream* filesystem_open_file(const char* name, int flags);
SYSCALL_HANDLER file_stream* filesystem_open_handle(file_handle* f, int flags);
SYSCALL_HANDLER int filesystem_read_file(void* dst, size_t len, file_stream* f);
SYSCALL_HANDLER int filesystem_close_file(file_stream* f);

file_handle* filesystem_find_file_in_dir(const directory_handle* d, const char* name);
file_handle* filesystem_find_file(const char* name);

int filesystem_setup_drives();

#endif