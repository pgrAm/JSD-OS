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

struct directory_stream;
typedef struct directory_stream directory_stream;

struct filesystem_virtual_drive;
typedef struct filesystem_virtual_drive filesystem_virtual_drive;

typedef struct
{
	fs_index location_on_disk;
	size_t disk_id;
	size_t size;
	uint32_t flags;
}
file_data_block;

//TODO make this opaque
typedef struct
{
	void* drv_impl_data;
	const disk_driver* driver;
	size_t index;
	size_t minimum_block_size;
	size_t num_blocks;
}
filesystem_drive;

typedef int (*partition_func)(filesystem_drive*, filesystem_virtual_drive*);

void filesystem_write_to_disk(const filesystem_virtual_drive* d, size_t block, size_t offset, const uint8_t* buf, size_t num_bytes);
void filesystem_read_from_disk(const filesystem_virtual_drive* d, size_t block, size_t offset, uint8_t* buf, size_t num_bytes);
uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size);
int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size);

struct filesystem_driver
{
	int (*mount_disk)(filesystem_virtual_drive* d);
	fs_index(*read_chunks)(uint8_t* dest, fs_index location, size_t offset, size_t num_bytes, const filesystem_virtual_drive* fd);
	fs_index(*write_chunks)(const uint8_t* dest, fs_index location, size_t offset, size_t num_bytes, const filesystem_virtual_drive* fd);
	size_t(*allocate_chunks)(fs_index location, size_t num_bytes, const filesystem_virtual_drive* d);
	void (*read_dir)(directory_stream* dest, const file_data_block* dir, const filesystem_virtual_drive* fd);
};

struct disk_driver
{
	void (*read_blocks)(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_blocks);
	void (*write_blocks)(const filesystem_drive* d, size_t block_number, const uint8_t* buf, size_t num_blocks);
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

//information about a file on disk
struct file_handle
{
	std::string name;

	file_data_block data;

	time_t time_created;
	time_t time_modified;
};

//information about a directory on disk
struct directory_stream
{
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

	void write_block(size_t block,
					 size_t offset,
					 const uint8_t* buf,
					 size_t num_bytes) const;

	void read_block(size_t block,
					size_t offset,
					uint8_t* buf,
					size_t num_bytes) const;
private:

	struct cached_block {
		size_t index;
		uint8_t* data;
		bool dirty;
	};

	cached_block& block_rw(size_t block) const;

	mutable std::vector<cached_block> block_cache;
	size_t max_cached_blocks;
};

#include<stdio.h>

class filesystem_buffer
{
public:
	filesystem_buffer(const filesystem_drive* d, size_t s) :
		m_drive(d),
		m_buffer(filesystem_allocate_buffer(d, s)),
		m_size(s)
	{
	}
	
	~filesystem_buffer()
	{
		filesystem_free_buffer(m_drive, m_buffer, m_size);
	}

	uint8_t& operator[](size_t n)
	{
		return m_buffer[n];
	}

	uint8_t operator[](size_t n) const
	{
		return m_buffer[n];
	}

	uint8_t* data()
	{
		return m_buffer;
	}

	size_t size() const
	{
		return m_size;
	}

private:
	const filesystem_drive* m_drive;
	uint8_t* const m_buffer;
	size_t m_size;
};


#endif

#endif