#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <time.h>
#include <stdbool.h>

#include <kernel/syscall.h>
#include <api/files.h>

typedef size_t fs_index; 

//information about a file on disk
typedef struct 
{
	char* name;
	char* type;
	
	char* full_name;
	
	uint32_t flags;

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

struct disk_driver;
typedef struct disk_driver disk_driver;

typedef struct 
{
	bool mounted;
	directory_handle root;
	void* fs_impl_data;
	filesystem_driver* fs_driver;
	void* dsk_impl_data;
	disk_driver* dsk_driver;
	uint32_t index;
	size_t minimum_block_size;
}
filesystem_drive;

SYSCALL_HANDLER directory_handle* filesystem_get_root_directory(size_t drive);

void filesystem_seek_file(file_stream* f, size_t pos);

inline size_t filesystem_get_size(file_stream* f)
{
	return f->file->size;
}


SYSCALL_HANDLER file_handle* filesystem_get_file_in_dir(const directory_handle* d, size_t index);
SYSCALL_HANDLER file_handle* filesystem_find_file_by_path(const directory_handle* rel, const char* name);

SYSCALL_HANDLER int filesystem_get_file_info(file_info* dst, const file_handle* src);

SYSCALL_HANDLER directory_handle* filesystem_open_directory_handle(file_handle* f, int flags);
SYSCALL_HANDLER directory_handle* filesystem_open_directory(const directory_handle* rel, const char* name, int flags);
SYSCALL_HANDLER int filesystem_close_directory(directory_handle* dir);

SYSCALL_HANDLER file_stream* filesystem_open_file_handle(file_handle* f, int flags);
SYSCALL_HANDLER file_stream* filesystem_open_file(const directory_handle* rel, const char* name, int flags);
SYSCALL_HANDLER int filesystem_read_file(void* dst, size_t len, file_stream* f);
SYSCALL_HANDLER int filesystem_close_file(file_stream* f);

//driver interface

void filesystem_read_blocks_from_disk(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);
uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size);
int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size);

int filesystem_setup_drives();

struct filesystem_driver
{
	bool (*mount_disk)(filesystem_drive* d);
	fs_index(*get_relative_location)(fs_index location, size_t byte_offset, const filesystem_drive* fd);
	fs_index(*read_chunks)(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd);
	void (*read_dir)(directory_handle* dest, fs_index location, const filesystem_drive* fd);
};

struct disk_driver
{
	void (*read_blocks)(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);
	uint8_t* (*allocate_buffer)(size_t size);
	int (*free_buffer)(uint8_t* buffer, size_t size);
};

filesystem_drive* filesystem_add_drive(disk_driver* disk_drv, void* driver_data, size_t block_size);

void filesystem_set_default_drive(size_t index);

#endif