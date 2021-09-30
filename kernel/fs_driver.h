#ifndef FS_DRIVER_H
#define FS_DRIVER_H

#include <kernel/filesystem.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef size_t fs_index;

struct filesystem_driver;
typedef struct filesystem_driver filesystem_driver;

struct disk_driver;
typedef struct disk_driver disk_driver;

//information about a directory on disk
struct directory_handle
{
	const char* name;
	size_t num_files;

	file_handle* file_list;
	size_t drive;
};

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
	size_t chunk_read_size;
}
filesystem_drive;

file_stream* filesystem_create_stream(const file_handle* f);

void filesystem_read_blocks_from_disk(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_blocks);
uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size);
int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size);

int filesystem_setup_drives();

struct filesystem_driver
{
	int (*mount_disk)(filesystem_drive* d);
	fs_index(*get_relative_location)(fs_index location, size_t byte_offset, const filesystem_drive* fd);
	fs_index(*read_chunks)(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd);
	void (*read_dir)(directory_handle* dest, const file_handle* dir, const filesystem_drive* fd);
};

struct disk_driver
{
	void (*read_blocks)(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);
	uint8_t* (*allocate_buffer)(size_t size);
	int (*free_buffer)(uint8_t* buffer, size_t size);
};

void filesystem_add_driver(filesystem_driver* fs_drv);
filesystem_drive* filesystem_add_drive(disk_driver* disk_drv, void* driver_data, size_t block_size);

#ifdef __cplusplus
}

#include <string>

//information about a file on disk
struct file_handle
{
	std::string name;
	std::string type;
	std::string full_name;

	uint32_t flags;

	size_t disk;
	fs_index location_on_disk;

	size_t size;

	time_t time_created;
	time_t time_modified;
};
#endif

#endif