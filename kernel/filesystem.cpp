#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

extern "C" {
	#include <kernel/memorymanager.h>
}

#include <kernel/filesystem.h>
#include <kernel/fs_driver.h>

#include <vector>
#include <string>

#define CHUNK_READ_SIZE 1024

//an instance of an open file
struct file_stream
{
	const file_handle* file;
	uint8_t* buffer;
	size_t seekpos;
	fs_index location_on_disk;
};

size_t default_drive = 0;
std::vector<filesystem_drive*> drives;
std::vector<filesystem_driver*> fs_drivers;

extern "C" void filesystem_add_driver(filesystem_driver* fs_drv)
{
	fs_drivers.push_back(fs_drv);
}

filesystem_drive* filesystem_add_drive(disk_driver* disk_drv, void* driver_data, size_t block_size)
{
	auto drive = new filesystem_drive{};
	drive->fs_impl_data = nullptr;
	drive->fs_driver = nullptr;
	drive->dsk_impl_data = driver_data;
	drive->dsk_driver = disk_drv;
	drive->mounted = false;
	drive->index = drives.size();
	drive->minimum_block_size = block_size;

	drives.push_back(drive);

	return drives.back();
}

void filesystem_set_default_drive(size_t index)
{
	default_drive = index;
}

int filesystem_setup_drives()
{
	return drives.size();
}

SYSCALL_HANDLER directory_handle* filesystem_open_directory_handle(const file_handle* f, int flags)
{
	if (f == nullptr || !(f->flags & IS_DIR)) { return nullptr; }

	directory_handle* d = new directory_handle();

	drives[f->disk]->fs_driver->read_dir(d, f, drives[f->disk]);

	return d;
}

SYSCALL_HANDLER 
directory_handle* filesystem_get_root_directory(size_t driveNumber)
{
	if(driveNumber < drives.size())
	{
		filesystem_drive* d = drives[driveNumber];
		
		if(!d->mounted)
		{
			if(d->fs_driver == nullptr)
			{
				for(size_t i = 0; i < fs_drivers.size(); i++)
				{
					d->fs_driver = fs_drivers[i];

					int mount_result = d->fs_driver->mount_disk(d);

					if(mount_result != UNKNOWN_FILESYSTEM && mount_result != DRIVE_NOT_SUPPORTED)
					{
						d->mounted = (mount_result == MOUNT_SUCCESS);
						break;
					}
					d->fs_driver = nullptr;
					d->fs_impl_data = nullptr;
				}
			}
			else
			{
				d->mounted = d->fs_driver->mount_disk(d);
			}
		}
		
		if(d->mounted)
		{
			return &d->root;
		}
	}

	return nullptr;
}

static file_handle* filesystem_find_file_in_dir(const directory_handle* d, const char* name)
{
	if(d != nullptr)
	{
		for(size_t i = 0; i < d->num_files; i++)
		{
			//printf("%s\n", d->file_list[i].full_name);
			if(strcasecmp(d->file_list[i].full_name.c_str(), name) == 0)
			{			
				return &d->file_list[i];
			}
		}
	}
	
	return nullptr;
}

SYSCALL_HANDLER 
file_handle* filesystem_find_file_by_path(const directory_handle* d, const char* name)
{
	if(name == nullptr) 
	{ 
		return nullptr; 
	};

	if (d == nullptr)
	{
		d = filesystem_get_root_directory(default_drive);
	}

	std::string path{name};
	size_t name_begin = 0;
	size_t path_begin = path.find('/');
	while (path_begin == name_begin) //name begins with a '/'
	{
		name_begin++;
		path_begin = path.find('/', name_begin);
	}

	//if there are still '/'s in the path
	if (path_begin != std::string::npos)
	{
		auto dirname = path.substr(name_begin, path_begin - name_begin);

		file_handle* fd = filesystem_find_file_in_dir(d, dirname.c_str());

		if(fd == nullptr || !(fd->flags & IS_DIR)) 
		{ 
			return nullptr; 
		}

		if (path_begin + 1 >= path.size()) //nothing follows the last '/'
		{
			return fd; //just return the dir
		}

		directory_handle* dir = filesystem_open_directory_handle(fd, 0);

		auto remaining_path = path.substr(path_begin);

		file_handle* f = filesystem_find_file_by_path(dir, remaining_path.c_str());

		//filesystem_close_directory(dir);
		return f;
	}
	else
	{
		auto remaining_path = path.substr(name_begin);
		return filesystem_find_file_in_dir(d, remaining_path.c_str());
	}
}

file_stream* filesystem_create_stream(const file_handle* f)
{
	file_stream* stream = new file_stream();

	stream->location_on_disk = 0;
	stream->seekpos = 0;
	stream->file = f;
	stream->buffer = filesystem_allocate_buffer(drives[f->disk], CHUNK_READ_SIZE);

	return stream;
}

SYSCALL_HANDLER 
file_stream* filesystem_open_file_handle(file_handle* f, int flags)
{	
	if (f == nullptr || f->flags & IS_DIR) { return nullptr; }

	return filesystem_create_stream(f);
}

SYSCALL_HANDLER 
int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	if (src == nullptr || dst == nullptr) return -1;
	//we should also check that these adresses are mapped properly
	//maybe do a checksum too?

	memcpy(dst->name, src->full_name.c_str(), src->full_name.size() + 1);
	dst->size = src->size;
	dst->flags = src->flags;
	dst->time_created = src->time_created;
	dst->time_modified = src->time_modified;

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
	//delete [] d->file_list;
	//delete d;
	return 0;
}

SYSCALL_HANDLER 
file_stream* filesystem_open_file(const directory_handle* rel, 
								  const char* name, 
								  int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, name);
	return filesystem_open_file_handle(f, flags);
}

SYSCALL_HANDLER int filesystem_close_file(file_stream* stream)
{
	if(filesystem_free_buffer(drives[stream->file->disk], stream->buffer, CHUNK_READ_SIZE) == 0)
	{
		//printf("file buffer freed at v=%X, p=%X\n",
		//	   stream->buffer, p);
	}
	delete stream;
	return 0;
}

SYSCALL_HANDLER 
directory_handle* filesystem_open_directory(const directory_handle* rel,
											const char* name, 
											int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, name);
	return filesystem_open_directory_handle(f, flags);
}
	
//finds the fs_index for the next chunk after an offset from the original
static fs_index filesystem_get_next_location_on_disk(const file_stream* f,
													 size_t byte_offset, fs_index chunk_index)
{
	return drives[f->file->disk]->fs_driver->get_relative_location(chunk_index, 
																   byte_offset, 
																   drives[f->file->disk]);
}	

//finds the fs_index for the first chunk in the file
static fs_index filesystem_resolve_location_on_disk(const file_stream* f)
{
	return filesystem_get_next_location_on_disk(f, 
												f->seekpos & ~(CHUNK_READ_SIZE-1), 
												f->file->location_on_disk);
}	
	
static fs_index filesystem_read_chunk(file_stream* f, fs_index chunk_index)
{
	//printf("filesystem_read_chunk from cluster %d\n", chunk_index);

	if(f->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(f, CHUNK_READ_SIZE, chunk_index);
	}
	
	f->location_on_disk = chunk_index;
	
	filesystem_drive* d = drives[f->file->disk];
	
	return d->fs_driver->read_chunks(f->buffer, chunk_index, CHUNK_READ_SIZE, d);
}

SYSCALL_HANDLER int filesystem_read_file(void* dst_buf, size_t len, file_stream* f)
{
	if(f == nullptr || f->file == nullptr)
	{
		return 0;
	}

	if(!(f->file->flags & IS_DIR))
	{
		if(f->seekpos >= f->file->size)
		{
			return -1; // EOF
		}

		if(f->seekpos + len >= f->file->size)
		{
			//only read what is available
			len = f->file->size - f->seekpos;
		}
	}
	
	fs_index location = filesystem_resolve_location_on_disk(f);
	//printf("seekpos %X starts at cluster %d\n", f->seekpos, location);

	size_t startchunk = f->seekpos / CHUNK_READ_SIZE;
	size_t endchunk = (f->seekpos + len) / CHUNK_READ_SIZE;

	size_t buf_start = f->seekpos % CHUNK_READ_SIZE;
	size_t buf_end_size = (f->seekpos + len) % CHUNK_READ_SIZE;
	
	if(startchunk == endchunk)
	{
		buf_end_size = 0; //start and end are in the same chunk
	}
	
	size_t buf_start_size = ((len - buf_end_size) % CHUNK_READ_SIZE);
	size_t numChunks = (len - (buf_start_size + buf_end_size)) / CHUNK_READ_SIZE;
	
	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	if(buf_start_size != 0)
	{
		location = filesystem_read_chunk(f, location);	
		memcpy(dst_ptr, f->buffer + buf_start, buf_start_size);
		dst_ptr += buf_start_size;
	}
	
	while(numChunks--)
	{
		location = filesystem_read_chunk(f, location);
		memcpy(dst_ptr, f->buffer, CHUNK_READ_SIZE);
		dst_ptr += CHUNK_READ_SIZE;
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

void filesystem_read_blocks_from_disk(const filesystem_drive* d,
									  size_t block_number, 
									  uint8_t* buf, 
									  size_t num_blocks)
{
	d->dsk_driver->read_blocks(d, block_number, buf, num_blocks);
}

uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size)
{
	if(d->dsk_driver->allocate_buffer != NULL)
	{
		return d->dsk_driver->allocate_buffer(size);
	}
	return (uint8_t*)malloc(size);
}

int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size)
{
	if(d->dsk_driver->free_buffer != NULL)
	{
		return d->dsk_driver->free_buffer(buffer, size);
	}
	free(buffer);
	return 0;
}
