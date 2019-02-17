#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <time.h>
#include <stdbool.h>

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
directory;

typedef enum
{
	FORMAT_FAT12 = 0,
	FORMAT_FAT32
}driveFormats;

typedef struct 
{
	bool mounted;
	directory root;
	driveFormats format;
	void* impl_data;
}
filesystem_drive;

directory* filesystem_mount_root_directory(size_t drive);

file_stream* filesystem_open_file(const char* name);
file_stream* filesystem_open_handle(file_handle* f);

void filesystem_seek_file(file_stream* f, size_t pos);

inline size_t filesystem_get_size(file_stream* f)
{
	return f->file->size;
}

int filesystem_read_file(void* dst, size_t len, file_stream* f);
int filesystem_close_file(file_stream* f);

file_handle* filesystem_find_file_in_dir(directory* d, const char* name);
file_handle* filesystem_find_file(const char* name);

int filesystem_setup_drives();

#endif