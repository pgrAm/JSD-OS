#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <kernel/filesystem/fs_driver.h>
#include "rdfs.h"

static int rdfs_mount_disk(filesystem_virtual_drive* d);
static fs_index rdfs_get_relative_location(fs_index location, size_t byte_offset, const filesystem_virtual_drive* fd);
static fs_index rdfs_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_virtual_drive* fd);
static void rdfs_read_dir(directory_stream* dest, const file_data_block* f, const filesystem_virtual_drive* fd);

static filesystem_driver rdfs_driver = {
	rdfs_mount_disk,
	rdfs_get_relative_location,
	rdfs_read_chunks,
	nullptr,
	nullptr,
	rdfs_read_dir
};

extern "C" void rdfs_init(void)
{
	filesystem_add_driver(&rdfs_driver);
}

static int rdfs_mount_disk(filesystem_virtual_drive* fd)
{
	if(fd->disk->minimum_block_size > 1 || fd->disk->driver->allocate_buffer != NULL)
	{
		return DRIVE_NOT_SUPPORTED;
	}

	uint8_t magic[4];
	filesystem_read_blocks_from_disk(fd, 0, &magic[0], sizeof(magic));

	if(memcmp(magic, "RDSK", 4) == 0)
	{
		fd->root_dir = {
			.name = {},
			.data = {sizeof(magic), fd->id, 0, IS_DIR},
			.time_created = 0,
			.time_modified = 0,
		};

		return MOUNT_SUCCESS;
	}
	return UNKNOWN_FILESYSTEM;
}

static fs_index rdfs_get_relative_location(fs_index location, size_t byte_offset, const filesystem_virtual_drive* fd)
{
	return location + byte_offset;
}

static fs_index rdfs_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_virtual_drive* fd)
{
	filesystem_read_blocks_from_disk(fd, location, dest, num_bytes);
	return location + num_bytes;
}

typedef struct
{
	char 		name[8];
	char 		extension[3];
	uint8_t		attributes;
	uint32_t	offset;
	uint32_t	size;
	time_t		modified;
	time_t		created;
} __attribute__((packed)) rdfs_dir_entry;

static std::string_view read_field(const char* field, size_t size)
{
	std::string_view f(field, size);
	if(auto pos = f.find('\0'); pos != std::string_view::npos)
	{
		return f.substr(0, pos);
	}
	return f;
}

static void rdfs_read_dir(directory_stream* dest, const file_data_block* f, const filesystem_virtual_drive* fd)
{
	fs_index location = f->location_on_disk;

	uint32_t num_files;
	filesystem_read_blocks_from_disk(fd, location, (uint8_t*)&num_files, sizeof(uint32_t));

	fs_index disk_location = location + sizeof(uint32_t);
	for(size_t i = 0; i < num_files; i++)
	{
		rdfs_dir_entry entry;
		filesystem_read_blocks_from_disk(fd, disk_location, (uint8_t*)&entry, sizeof(rdfs_dir_entry));
		disk_location += sizeof(rdfs_dir_entry);

		file_handle file;

		file.name = read_field(entry.name, 8);
		if(auto ext = read_field(entry.extension, 3); !ext.empty())
		{
			file.name += '.';
			file.name += ext;
		}

		file.time_created = entry.created;
		file.time_modified = entry.modified;

		uint32_t flags = 0;
		if(entry.attributes & IS_DIR)
		{
			flags |= IS_DIR;
		}

		file.data = {
			.location_on_disk = entry.offset,
			.disk_id = fd->id,
			.size = entry.size,
			.flags = flags
		};

		dest->file_list.push_back(file);
	}

}
