#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "filesystem.h"
#include "../drivers/fat12.h"

#define NUM_DRIVES 2

#define CHUNK_READ_SIZE 1024

filesystem_drive drives[NUM_DRIVES];

int filesystem_setup_drives()
{
	fat12_drive* floppy0 = (fat12_drive*)malloc(sizeof(fat12_drive));
	floppy0->type = TYPE_FLOPPY;
	floppy0->index = 0;
	
	fat12_drive* floppy1 = (fat12_drive*)malloc(sizeof(fat12_drive));
	floppy1->type = TYPE_FLOPPY;
	floppy1->index = 1;
	
	drives[0].format = FORMAT_FAT12;
	drives[0].impl_data = (void*)floppy0;
	drives[0].mounted = false;
	
	drives[1].format = FORMAT_FAT12;
	drives[1].impl_data = (void*)floppy1;
	drives[1].mounted = false;
	
	return NUM_DRIVES;
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
			switch(d->format)
			{
				case FORMAT_FAT12:
					fat12_mount_disk(d, driveNumber);
					d->mounted = true;
				break;
				default:
					return NULL;
			}
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

file_handle* filesystem_find_file_on_disk(size_t driveNumber, const char* name)
{
	const directory_handle* root = filesystem_get_root_directory(driveNumber);

	return filesystem_find_file_in_dir(root, name);
}

file_handle* filesystem_find_file(const char* name)
{
	return filesystem_find_file_on_disk(0, name);
}

SYSCALL_HANDLER file_stream* filesystem_open_handle(file_handle* f, int flags)
{	
	file_stream* stream = (file_stream*)malloc(sizeof(file_stream));
	
	stream->location_on_disk = 0;
	stream->seekpos = 0;
	stream->file = f;
	stream->buffer = (uint8_t*)malloc(CHUNK_READ_SIZE);
	
	return stream;
}

SYSCALL_HANDLER int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	if (src == NULL || dst == NULL) return -1;
	//we should also check that these adresses are mapped properly
	//maybe do a checksum too?

	strcpy(dst->name, src->full_name);
	dst->size = src->size;
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

SYSCALL_HANDLER directory_handle* filesystem_open_directory(const char* name, int flags)
{
	return NULL;
}

SYSCALL_HANDLER file_stream* filesystem_open_file(const char* name, int flags)
{
	file_handle* f = filesystem_find_file(name);
	
	return (f != NULL) ? filesystem_open_handle(f, flags) : NULL;
}
	
fs_index filesystem_get_next_location_on_disk(const file_stream* f, fs_index chunk_index)
{
	return fat12_get_next_cluster(chunk_index, &drives[f->file->disk]);
}	
	
fs_index filesystem_resolve_location_on_disk(const file_stream* f)
{
	return fat12_get_cluster(f->file->location_on_disk, f->seekpos, &drives[f->file->disk]);
}	
	
fs_index filesystem_read_chunk(file_stream* f, fs_index chunk_index)
{
	if(f->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(f, chunk_index);
	}
	
	f->location_on_disk = chunk_index;
	
	filesystem_drive* d = &drives[f->file->disk];
	
	switch(d->format)
	{
		case FORMAT_FAT12:
			return fat12_read_clusters(f->buffer, CHUNK_READ_SIZE, chunk_index, d);
		break;
		default:
			return 0;
	}
	
	return 0;
}

SYSCALL_HANDLER int filesystem_read_file(void* dst, size_t len, file_stream* f)
{
	if(f == NULL)
	{
		return 0;
	}	
	
	fs_index location = filesystem_resolve_location_on_disk(f);

	size_t buf_start = f->seekpos % CHUNK_READ_SIZE;
	size_t buf_end_size = (f->seekpos + len) % CHUNK_READ_SIZE;
	
	if(len < CHUNK_READ_SIZE)
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
	free(f->buffer);
	free(f);
	return 0;
}
