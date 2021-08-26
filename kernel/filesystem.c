#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "filesystem.h"
#include <memorymanager.h>
#include "../drivers/fat12.h"

#define NUM_DRIVES 2

#define CHUNK_READ_SIZE 1024

filesystem_drive drives[NUM_DRIVES];

struct filesystem_driver
{
	void (*mount_disk)(filesystem_drive* d);
	fs_index (*get_relative_location)(fs_index location, size_t byte_offset, const filesystem_drive* fd);
	fs_index (*read_chunks)(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd);
	void (*read_dir)(directory_handle* dest, fs_index location, const filesystem_drive* fd);
};

filesystem_driver fat_driver = {
	fat12_mount_disk,
	fat12_get_relative_cluster,
	fat12_read_clusters,
	fat12_read_dir
};

int filesystem_setup_drives()
{
	fat12_drive* floppy0 = (fat12_drive*)malloc(sizeof(fat12_drive));
	floppy0->type = TYPE_FLOPPY;
	floppy0->index = 0;
	
	fat12_drive* floppy1 = (fat12_drive*)malloc(sizeof(fat12_drive));
	floppy1->type = TYPE_FLOPPY;
	floppy1->index = 1;
	
	drives[0].impl_data = (void*)floppy0;
	drives[0].mounted = false;
	drives[0].driver = &fat_driver;
	drives[0].index = 0;

	drives[1].impl_data = (void*)floppy1;
	drives[1].mounted = false;
	drives[1].driver = &fat_driver;
	drives[1].index = 1;
	
	return NUM_DRIVES;
}

SYSCALL_HANDLER directory_handle* filesystem_open_directory_handle(file_handle* f, int flags)
{
	if (f == NULL || !(f->flags & IS_DIR)) { return NULL; }

	directory_handle* d = (directory_handle*)malloc(sizeof(directory_handle));

	drives[f->disk].driver->read_dir(d, f->location_on_disk, &drives[f->disk]);

	return d;
}

SYSCALL_HANDLER directory_handle* filesystem_get_root_directory(size_t driveNumber)
{
	if(driveNumber > NUM_DRIVES)
	{
		return NULL;
	}
	else
	{
		filesystem_drive* d = &drives[driveNumber];
		
		if(!d->mounted)
		{
			d->driver->mount_disk(d);
			d->mounted = true;
		}
		
		return &d->root;
	}
}

file_handle* filesystem_find_file_in_dir(const directory_handle* d, const char* name)
{
	if(d != NULL)
	{
		for(size_t i = 0; i < d->num_files; i++)
		{
			if(strcasecmp(d->file_list[i].full_name, name) == 0)
			{			
				return &d->file_list[i];
			}
		}
	}
	
	return NULL;
}

SYSCALL_HANDLER file_handle* filesystem_find_file_by_path(const directory_handle* d, const char* name)
{
	if(name == NULL) { return NULL; };

	if (d == NULL)
	{
		d = filesystem_get_root_directory(0);
	}

	const char* path = strchr(name, '/');
	while (path == name) //name begins with a '/'
	{
		name++;
		path = strchr(name, '/');
	}

	if (path != NULL)
	{
		char dirname[MAX_PATH];
		size_t name_len = path - name;
		memcpy(dirname, name, name_len);
		dirname[name_len] = '\0';

		file_handle* fd = filesystem_find_file_in_dir(d, dirname);

		if(fd == NULL || !(fd->flags & IS_DIR)) { return NULL; }

		if (path[1] == '\0') //nothing follows the last '/'
		{
			return fd; //just return the dir
		}

		directory_handle* dir = filesystem_open_directory_handle(fd, 0);

		file_handle* f = filesystem_find_file_by_path(dir, path);

		//filesystem_close_directory(dir);
		return f;
	}
	else
	{
		return filesystem_find_file_in_dir(d, name);
	}
}

SYSCALL_HANDLER file_stream* filesystem_open_file_handle(file_handle* f, int flags)
{	
	if (f == NULL || f->flags & IS_DIR) { return NULL; }

	file_stream* stream = (file_stream*)malloc(sizeof(file_stream));
	
	stream->location_on_disk = 0;
	stream->seekpos = 0;
	stream->file = f;
	stream->buffer = memmanager_virtual_alloc(NULL, 1, PAGE_RW | PAGE_PRESENT);//(uint8_t*)malloc(CHUNK_READ_SIZE);

	return stream;
}

SYSCALL_HANDLER int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	if (src == NULL || dst == NULL) return -1;
	//we should also check that these adresses are mapped properly
	//maybe do a checksum too?

	strcpy(dst->name, src->full_name);
	dst->size = src->size;
	dst->flags = src->flags;
	dst->time_created = src->time_created;
	dst->time_modified = src->time_modified;

	return 0;
}

SYSCALL_HANDLER file_handle* filesystem_get_file_in_dir(const directory_handle* d, size_t index)
{
	if (d != NULL && index < d->num_files)
	{
		return &(d->file_list[index]);
	}

	return NULL;
}

SYSCALL_HANDLER int filesystem_close_directory(directory_handle* d)
{
	//free(d->file_list);
	free(d);
	return 0;
}

SYSCALL_HANDLER file_stream* filesystem_open_file(const directory_handle* rel, const char* name, int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, name);
	return filesystem_open_file_handle(f, flags);
}

SYSCALL_HANDLER directory_handle* filesystem_open_directory(const directory_handle* rel, const char* name, int flags)
{
	file_handle* f = filesystem_find_file_by_path(rel, name);
	return filesystem_open_directory_handle(f, flags);
}
	
//finds the fs_index for the next chunk after an offset from the original
fs_index filesystem_get_next_location_on_disk(const file_stream* f, size_t byte_offset, fs_index chunk_index)
{
	return drives[f->file->disk].driver->get_relative_location(chunk_index, byte_offset, &drives[f->file->disk]);
}	

//finds the fs_index for the first chunk in the file
fs_index filesystem_resolve_location_on_disk(const file_stream* f)
{
	return filesystem_get_next_location_on_disk(f, f->seekpos & ~(CHUNK_READ_SIZE-1), f->file->location_on_disk);
}	
	
fs_index filesystem_read_chunk(file_stream* f, fs_index chunk_index)
{
	//printf("filesystem_read_chunk from cluster %d\n", chunk_index);

	if(f->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(f, CHUNK_READ_SIZE, chunk_index);
	}
	
	f->location_on_disk = chunk_index;
	
	filesystem_drive* d = &drives[f->file->disk];
	
	return d->driver->read_chunks(f->buffer, chunk_index, CHUNK_READ_SIZE, d);
}

SYSCALL_HANDLER int filesystem_read_file(void* dst, size_t len, file_stream* f)
{
	if(f == NULL)
	{
		return 0;
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
	
	if(buf_start_size != 0)
	{
		location = filesystem_read_chunk(f, location);	
		memcpy(dst, f->buffer + buf_start, buf_start_size);
		dst += buf_start_size;
	}
	
	while(numChunks--)
	{
		location = filesystem_read_chunk(f, location);
		memcpy(dst, f->buffer, CHUNK_READ_SIZE);
		dst += CHUNK_READ_SIZE;
	}
	
	if(buf_end_size != 0)
	{
		filesystem_read_chunk(f, location);
		memcpy(dst, f->buffer, buf_end_size);
	}
	
	f->seekpos += len;
	
	return len;
}

void filesystem_seek_file(file_stream* f, size_t pos)
{
	f->seekpos = pos;
}

SYSCALL_HANDLER int filesystem_close_file(file_stream* f)
{
	memmanager_free_pages(f->buffer, 1);
	//free(f->buffer);
	free(f);
	return 0;
}
