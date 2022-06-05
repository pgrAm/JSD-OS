#ifndef FS_DRIVER_H
#define FS_DRIVER_H

#include <kernel/filesystem.h>

#ifdef __cplusplus
extern "C" {
#endif
struct filesystem_driver;
typedef struct filesystem_driver filesystem_driver;

struct disk_driver;
typedef struct disk_driver disk_driver;

struct directory_stream;
typedef struct directory_stream directory_stream;

struct filesystem_virtual_drive;
typedef struct filesystem_virtual_drive filesystem_virtual_drive;

struct filesystem_drive;
typedef struct filesystem_drive filesystem_drive;

typedef int (*partition_func)(filesystem_drive*, filesystem_virtual_drive*, size_t block_size);

void filesystem_write_to_disk(const filesystem_drive* d, size_t block, size_t offset, const uint8_t* buf, size_t num_bytes);
void filesystem_read_from_disk(const filesystem_drive* d, size_t block, size_t offset, uint8_t* buf, size_t num_bytes);

struct filesystem_driver
{
	mount_status (*mount_disk)(filesystem_virtual_drive* d);
	fs_index(*read_chunks)(uint8_t* dest, fs_index location, size_t offset, size_t num_bytes, const file_data_block* file, const filesystem_virtual_drive* fd);
	fs_index(*write_chunks)(const uint8_t* dest, fs_index location, size_t offset, size_t num_bytes, const file_data_block* file, const filesystem_virtual_drive* fd);
	file_size_t(*allocate_chunks)(fs_index location, size_t num_bytes, const file_data_block* file, const filesystem_virtual_drive* d);
	void (*read_dir)(directory_stream* dest, const file_data_block* dir, const filesystem_virtual_drive* fd);
	void (*flush_file)(const file_data_block* file, const filesystem_virtual_drive* fd);
	void (*create_file)(const char* name, size_t name_len, uint32_t flags, directory_stream* dir, const filesystem_virtual_drive* fd);
	int (*delete_file)(const file_data_block* file, const filesystem_virtual_drive* fd);
};

struct disk_driver
{
	void (*read_blocks)(void* driver_data, size_t block_number, uint8_t* buf, size_t num_blocks);
	void (*write_blocks)(void* driver_data, size_t block_number, const uint8_t* buf, size_t num_blocks);
	uint8_t* (*allocate_buffer)(size_t size);
	int (*free_buffer)(uint8_t* buffer, size_t size);
};

void filesystem_add_driver(const filesystem_driver* fs_drv);
filesystem_drive* filesystem_add_drive(const disk_driver* disk_drv, void* driver_data, size_t block_size, size_t num_blocks);
void filesystem_add_virtual_drive(filesystem_drive* disk, fs_index begin, size_t size);
void filesystem_add_partitioner(partition_func p);

file_stream* filesystem_create_stream(const file_data_block* f);

#ifdef __cplusplus
}

#include <string>
#include <vector>

//information about a directory on disk
struct directory_stream
{
	file_data_block data;
	std::vector<file_handle> file_list;
};

//represents a partition on a drive
struct filesystem_virtual_drive
{
	filesystem_virtual_drive(filesystem_drive* disk, fs_index begin, size_t size);

	filesystem_drive* disk;
	void* fs_impl_data;
	const filesystem_driver* fs_driver;
	size_t id;
	fs_index first_block;
	size_t num_blocks;
	size_t block_size;

	file_handle root_dir;

	bool mounted;
	bool read_only;
};

inline void filesystem_write(const filesystem_virtual_drive* d, size_t block, size_t offset, const uint8_t* buf, size_t num_bytes)
{
	filesystem_write_to_disk(d->disk, block + d->first_block, offset, buf, num_bytes);
}
inline void filesystem_read(const filesystem_virtual_drive* d, size_t block, size_t offset, uint8_t* buf, size_t num_bytes)
{
	filesystem_read_from_disk(d->disk, block + d->first_block, offset, buf, num_bytes);
}


#endif

#endif