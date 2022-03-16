#include <stdlib.h>

#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/drives.h>

#include <kernel/kassert.h>

//an instance of an open file
struct file_stream
{
	file_data_block file;
	size_t seekpos;
	fs_index location_on_disk;
	uint8_t* buffer;
};

file_stream* filesystem_create_stream(const file_data_block* f)
{
	k_assert(f);
	auto drive = filesystem_get_drive(f->disk_id);
	return new file_stream
	{
		*f, 0, 0,
		filesystem_allocate_buffer(drive->disk, drive->chunk_read_size)
	};
}

file_stream* filesystem_open_file_handle(const file_handle* f, int flags)
{
	k_assert(f);

	if(f->data.flags & IS_DIR) { return nullptr; }

	return filesystem_create_stream(&f->data);
}

file_stream* filesystem_open_file(const directory_stream* rel,
								  std::string_view path,
								  int flags)
{
	k_assert(rel);

	if(const file_handle* f = filesystem_find_file_by_path(rel, path))
		return filesystem_open_file_handle(f, flags);
	else
		return nullptr;
}

int filesystem_close_file(file_stream* s)
{
	if(s == nullptr)
	{
		return -1;
	}

	auto drive = filesystem_get_drive(s->file.disk_id);

	if(filesystem_free_buffer(drive->disk, s->buffer, drive->chunk_read_size) == 0)
	{
		//printf("file buffer freed at v=%X, p=%X\n",
		//	   stream->buffer, p);
	}
	delete s;
	return 0;
}

//finds the fs_index for the next chunk after an offset from the original
static fs_index filesystem_get_next_location_on_disk(const file_stream* s,
													 size_t byte_offset,
													 fs_index chunk_index)
{
	auto drive = filesystem_get_drive(s->file.disk_id);
	auto driver = drive->fs_driver;

	return driver->get_relative_location(chunk_index, byte_offset, drive);
}

//finds the fs_index for the first chunk in the file
static fs_index filesystem_resolve_location_on_disk(const file_stream* s)
{
	auto drive = filesystem_get_drive(s->file.disk_id);
	return filesystem_get_next_location_on_disk(s,
												s->seekpos & ~(drive->chunk_read_size - 1),
												s->file.location_on_disk);
}

static fs_index filesystem_read_chunk(file_stream* s, fs_index chunk_index)
{
	//printf("filesystem_read_chunk from cluster %d\n", chunk_index);

	auto drive = filesystem_get_drive(s->file.disk_id);
	if(s->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(s, drive->chunk_read_size, chunk_index);
	}

	s->location_on_disk = chunk_index;

	return drive->fs_driver->read_chunks(s->buffer, chunk_index,
										 drive->chunk_read_size, drive);
}

int filesystem_read_file(void* dst_buf, size_t len, file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);

	if(!(s->file.flags & IS_DIR))
	{
		if(s->seekpos >= s->file.size)
		{
			return -1; // EOF
		}

		if(s->seekpos + len >= s->file.size)
		{
			//only read what is available
			len = s->file.size - s->seekpos;
		}
	}

	auto drive = filesystem_get_drive(s->file.disk_id);

	fs_index location = filesystem_resolve_location_on_disk(s);
	//printf("seekpos %X starts at cluster %d\n", f->seekpos, location);

	size_t startchunk = s->seekpos / drive->chunk_read_size;
	size_t endchunk = (s->seekpos + len) / drive->chunk_read_size;

	size_t buf_start = s->seekpos % drive->chunk_read_size;
	size_t buf_end_size = (s->seekpos + len) % drive->chunk_read_size;

	if(startchunk == endchunk)
	{
		buf_end_size = 0; //start and end are in the same chunk
	}

	size_t buf_start_size = ((len - buf_end_size) % drive->chunk_read_size);
	size_t num_chunks = (len - (buf_start_size + buf_end_size)) / drive->chunk_read_size;

	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	if(buf_start_size != 0)
	{
		location = filesystem_read_chunk(s, location);
		memcpy(dst_ptr, s->buffer + buf_start, buf_start_size);
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
				location = filesystem_read_chunk(s, location);
				memcpy(dst_ptr, s->buffer, drive->chunk_read_size);
				dst_ptr += drive->chunk_read_size;
			}
		}
	}

	if(buf_end_size != 0)
	{
		filesystem_read_chunk(s, location);
		memcpy(dst_ptr, s->buffer, buf_end_size);
	}

	s->seekpos += len;

	return len;
}

void filesystem_seek_file(file_stream* f, size_t pos)
{
	k_assert(f);

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

SYSCALL_HANDLER int syscall_read_file(void* dst_buf, size_t len, file_stream* f)
{
	if(f == nullptr || dst_buf == nullptr)
	{
		return 0;
	}
	return filesystem_read_file(dst_buf, len, f);
}

SYSCALL_HANDLER
file_stream* syscall_open_file(const directory_stream* rel,
							   const char* path,
							   size_t path_len,
							   int flags)
{
	if(!rel || !path)
		return nullptr;
	else
		return filesystem_open_file(rel, {path, path_len}, flags);
}

SYSCALL_HANDLER int syscall_close_file(file_stream* stream)
{
	if(stream == nullptr)
	{
		return -1;
	}

	delete stream;
	return 0;
}