#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <kernel/memorymanager.h>
#include <kernel/filesystem.h>
#include <kernel/fs_driver.h>
#include <kernel/kassert.h>

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

#define DEFAULT_CHUNK_READ_SIZE 1024

//an instance of an open file
struct file_stream
{
	file_data_block file;
	size_t seekpos;
	fs_index location_on_disk;
	uint8_t* buffer;
};

using fs_drive_list = std::vector<filesystem_virtual_drive*>;
using fs_part_map = std::vector<fs_drive_list>;

static std::vector<filesystem_drive*> drives;
static std::vector<filesystem_virtual_drive*> virtual_drives;
static std::vector<const filesystem_driver*> fs_drivers;
static std::vector<partition_func> partitioners;

static fs_part_map partition_map;

size_t filesystem_get_num_drives()
{
	return virtual_drives.size();
}

static bool filesystem_mount_drive(filesystem_virtual_drive* drive)
{
	k_assert(drive);

	if(!drive->mounted)
	{
		if(drive->fs_driver == nullptr)
		{
			for(auto& driver : fs_drivers)
			{
				drive->fs_driver = driver;
				int mount_result = driver->mount_disk(drive);

				if(mount_result != UNKNOWN_FILESYSTEM && mount_result != DRIVE_NOT_SUPPORTED)
				{
					drive->mounted = (mount_result == MOUNT_SUCCESS);
					break;
				}
				drive->fs_driver = nullptr;
				drive->fs_impl_data = nullptr;
			}
		}
		else
		{
			k_assert(drive->fs_driver);
			drive->mounted = drive->fs_driver->mount_disk(drive);
		}
	}

	return drive->mounted;
}

static void filesystem_read_drive_partitions(filesystem_drive* drive, filesystem_virtual_drive* base)
{
	k_assert(!base || base->disk == drive);
	k_assert(!base || !base->mounted);

	for(size_t i = 0; i < partitioners.size(); i++)
	{
		if(partitioners[i](drive, base) == 0)
			return;
	}

	if(!base)
	{
		//no known partitioning on drive
		filesystem_add_virtual_drive(drive, 0, drive->num_blocks);
	}
}

extern "C" void filesystem_add_partitioner(partition_func p)
{
	partitioners.push_back(p);

	for(auto drive : drives)
	{
		auto& partitions = partition_map[drive->index];
		if(partitions.size() > 1)
		{
			continue;
		}
		if(partitions.size() == 1)
		{
			if(!partitions[0]->mounted)
			{
				filesystem_read_drive_partitions(drive, partitions[0]);
			}
			continue;
		}

		filesystem_read_drive_partitions(drive, nullptr);
	}
}

extern "C" void filesystem_add_virtual_drive(filesystem_drive* disk, fs_index begin, size_t size)
{
	auto drive = new filesystem_virtual_drive{
		.disk = disk,
		.fs_impl_data = nullptr,
		.fs_driver = nullptr,
		.id = virtual_drives.size(),
		.first_block = begin,
		.num_blocks = size,
		.chunk_read_size =	disk->minimum_block_size > DEFAULT_CHUNK_READ_SIZE ?
							disk->minimum_block_size : DEFAULT_CHUNK_READ_SIZE,
		.mounted = false
	};

	virtual_drives.push_back(drive);

	k_assert(disk->index < partition_map.size());
	partition_map[disk->index].push_back(drive);
}

extern "C" void filesystem_add_driver(const filesystem_driver* fs_drv)
{
	fs_drivers.push_back(fs_drv);
}

filesystem_drive* filesystem_add_drive(const disk_driver* disk_drv, void* driver_data, size_t block_size, size_t num_blocks)
{
	auto drive = new filesystem_drive
	{
		.drv_impl_data = driver_data,
		.driver = disk_drv,
		.index = drives.size(),
		.minimum_block_size = block_size,
		.num_blocks = num_blocks
	};

	drives.push_back(drive);
	partition_map.push_back(fs_drive_list{});

	filesystem_read_drive_partitions(drive, nullptr);

	return drives.back();
}

SYSCALL_HANDLER directory_handle* filesystem_open_directory_handle(const file_handle* f, int flags)
{
	if (f == nullptr || !(f->data.flags & IS_DIR)) { return nullptr; }

	directory_handle* d = new directory_handle{
		.name = nullptr,
		.file_list = nullptr,
		.num_files = 0,
		.disk_id = f->data.disk_id
	};

	auto drive = virtual_drives[f->data.disk_id];

	drive->fs_driver->read_dir(d, &f->data, drive);

	return d;
}

SYSCALL_HANDLER 
directory_handle* filesystem_get_root_directory(size_t drive_number)
{
	if(drive_number < virtual_drives.size())
	{
		auto drive = virtual_drives[drive_number];
		
		if(filesystem_mount_drive(drive))
		{
			return &drive->root;
		}
	}

	return nullptr;
}

static file_handle* filesystem_find_file_in_dir(const directory_handle* d, std::string_view name)
{
	if(d != nullptr)
	{
		for(size_t i = 0; i < d->num_files; i++)
		{
			//printf("%s\n", std::string(d->file_list[i].full_name).c_str());
			if(filesystem_names_identical(d->file_list[i].full_name, name))
			{		
				return &d->file_list[i];
			}
		}
	}

	return nullptr;
}

static file_handle found;

SYSCALL_HANDLER 
file_handle* filesystem_find_file_by_path(const directory_handle* d, const char* name, size_t name_len)
{
	if(name == nullptr || d == nullptr)
	{ 
		return nullptr; 
	};

	std::string_view path{name, name_len};

	//printf("%s\n", std::string(path).c_str());

	size_t name_begin = 0;
	size_t path_begin = path.find('/');
	while (path_begin == name_begin) //name begins with a '/'
	{
		name_begin++;
		path_begin = path.find('/', name_begin);
	}

	//if there are still '/'s in the path
	if (path_begin != std::string_view::npos)
	{
		auto dirname = path.substr(name_begin, path_begin - name_begin);

		file_handle* fd = filesystem_find_file_in_dir(d, dirname);

		if(fd == nullptr || !(fd->data.flags & IS_DIR))
		{ 
			return nullptr; 
		}

		if (path_begin + 1 >= path.size()) //nothing follows the last '/'
		{
			return fd; //just return the dir
		}

		directory_handle* dir = filesystem_open_directory_handle(fd, 0);

		auto remaining_path = path.substr(path_begin);


		file_handle fh;
		auto f = filesystem_find_file_by_path(dir, remaining_path.data(), remaining_path.size());

		if(f) fh = *f;

		filesystem_close_directory(dir);

		if(f)
		{
			found = fh;
			return &found;
		}
		return nullptr;
	}
	else
	{
		auto remaining_path = path.substr(name_begin);
		return filesystem_find_file_in_dir(d, remaining_path);
	}
}

file_stream* filesystem_create_stream(const file_data_block* f)
{
	if(f->disk_id >= virtual_drives.size())
	{
		return nullptr;
	}

	auto drive = virtual_drives[f->disk_id];

	return new file_stream
	{
		*f, 0, 0,
		filesystem_allocate_buffer(drive->disk, drive->chunk_read_size)
	};
}

SYSCALL_HANDLER 
file_stream* filesystem_open_file_handle(const file_handle* f, int flags)
{	
	if (f == nullptr || f->data.flags & IS_DIR) { return nullptr; }

	return filesystem_create_stream(&f->data);
}

SYSCALL_HANDLER 
int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	if (src == nullptr || dst == nullptr) return -1;
	//we should also check that these adresses are mapped properly
	//maybe do a checksum too?

	memcpy(dst->name, src->full_name.c_str(), src->full_name.size() + 1);
	dst->size = src->data.size;
	dst->flags = src->data.flags;
	dst->time_created = src->time_created;
	dst->time_modified = src->time_modified;
	dst->name_len = src->full_name.size();

	return 0;
}

SYSCALL_HANDLER 
file_handle* filesystem_get_file_in_dir(const directory_handle* d, size_t index)
{
	if (d != nullptr && index < d->num_files)
	{
		return &(d->file_list[index]);
	}

	return nullptr;
}

SYSCALL_HANDLER 
int filesystem_close_directory(directory_handle* d)
{
	if(d != nullptr)
	{
		if(&(virtual_drives[d->disk_id]->root) != d)
		{
			delete[] d->file_list;
			delete d;
		}
	}
	return 0;
}

SYSCALL_HANDLER 
file_stream* filesystem_open_file(const directory_handle* rel, 
								  const char* path, 
								  size_t path_len,
								  int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, path, path_len);
	return filesystem_open_file_handle(f, flags);
}

SYSCALL_HANDLER int filesystem_close_file(file_stream* stream)
{
	if(stream == nullptr)
	{
		return -1;
	}

	auto drive = virtual_drives[stream->file.disk_id];

	if(filesystem_free_buffer(drive->disk, stream->buffer, drive->chunk_read_size) == 0)
	{
		//printf("file buffer freed at v=%X, p=%X\n",
		//	   stream->buffer, p);
	}
	delete stream;
	return 0;
}
 
directory_handle* filesystem_open_directory(const directory_handle* rel,
											const char* path, 
											size_t path_len,
											int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, path, path_len);
	return filesystem_open_directory_handle(f, flags);
}
	
//finds the fs_index for the next chunk after an offset from the original
static fs_index filesystem_get_next_location_on_disk(const file_stream* f,
													 size_t byte_offset, 
													 fs_index chunk_index)
{
	auto drive = virtual_drives[f->file.disk_id];
	auto driver = drive->fs_driver;

	return driver->get_relative_location(chunk_index, byte_offset, drive);
}	

//finds the fs_index for the first chunk in the file
static fs_index filesystem_resolve_location_on_disk(const file_stream* f)
{
	auto drive = virtual_drives[f->file.disk_id];
	return filesystem_get_next_location_on_disk(f, 
												f->seekpos & ~(drive->chunk_read_size-1),
												f->file.location_on_disk);
}

static fs_index filesystem_read_chunk(file_stream* f, fs_index chunk_index)
{
	//printf("filesystem_read_chunk from cluster %d\n", chunk_index);

	auto drive = virtual_drives[f->file.disk_id];
	if(f->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(f, drive->chunk_read_size, chunk_index);
	}
	
	f->location_on_disk = chunk_index;
	
	return drive->fs_driver->read_chunks(f->buffer, chunk_index, 
										 drive->chunk_read_size, drive);
}

SYSCALL_HANDLER int filesystem_read_file(void* dst_buf, size_t len, file_stream* f)
{
	if(f == nullptr)
	{
		return 0;
	}

	if(!(f->file.flags & IS_DIR))
	{
		if(f->seekpos >= f->file.size)
		{
			return -1; // EOF
		}

		if(f->seekpos + len >= f->file.size)
		{
			//only read what is available
			len = f->file.size - f->seekpos;
		}
	}

	auto drive = virtual_drives[f->file.disk_id];
	
	fs_index location = filesystem_resolve_location_on_disk(f);
	//printf("seekpos %X starts at cluster %d\n", f->seekpos, location);

	size_t startchunk = f->seekpos / drive->chunk_read_size;
	size_t endchunk = (f->seekpos + len) / drive->chunk_read_size;

	size_t buf_start = f->seekpos % drive->chunk_read_size;
	size_t buf_end_size = (f->seekpos + len) % drive->chunk_read_size;
	
	if(startchunk == endchunk)
	{
		buf_end_size = 0; //start and end are in the same chunk
	}
	
	size_t buf_start_size = ((len - buf_end_size) % drive->chunk_read_size);
	size_t num_chunks = (len - (buf_start_size + buf_end_size)) / drive->chunk_read_size;
	
	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	if(buf_start_size != 0)
	{
		location = filesystem_read_chunk(f, location);	
		memcpy(dst_ptr, f->buffer + buf_start, buf_start_size);
		dst_ptr += buf_start_size;
	}
	
	if(num_chunks)
	{
		if(drive->disk->driver->allocate_buffer == nullptr)
		{
			auto size = drive->chunk_read_size * num_chunks;

			location = drive->fs_driver->read_chunks(dst_ptr, location, size, drive);
			dst_ptr += size;
		}
		else
		{
			while(num_chunks--)
			{
				location = filesystem_read_chunk(f, location);
				memcpy(dst_ptr, f->buffer, drive->chunk_read_size);
				dst_ptr += drive->chunk_read_size;
			}
		}
	}
	
	if(buf_end_size != 0)
	{
		filesystem_read_chunk(f, location);
		memcpy(dst_ptr, f->buffer, buf_end_size);
	}
	
	f->seekpos += len;
	
	return len;
}

void filesystem_seek_file(file_stream* f, size_t pos)
{
	f->seekpos = pos;
}

void filesystem_read_blocks_from_disk(const filesystem_virtual_drive* d,
									  size_t block_number, 
									  uint8_t* buf, 
									  size_t num_blocks)
{
	d->disk->driver->read_blocks(d->disk, d->first_block + block_number, buf, num_blocks);
}

uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size)
{
	if(d->driver->allocate_buffer != nullptr)
	{
		return d->driver->allocate_buffer(size);
	}
	return (uint8_t*)malloc(size);
}

int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size)
{
	if(d->driver->free_buffer != nullptr)
	{
		return d->driver->free_buffer(buffer, size);;
	}
	free(buffer);
	return 0;
}
