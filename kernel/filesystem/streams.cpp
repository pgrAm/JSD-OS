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
	bool is_dirty;
};

static void filesystem_flush_buffer(file_stream* s);

file_stream* filesystem_create_stream(const file_data_block* f)
{
	k_assert(f);
	auto drive = filesystem_get_drive(f->disk_id);
	return new file_stream
	{
		*f, 0, 0,
		filesystem_allocate_buffer(drive->disk, drive->chunk_read_size),
		false
	};
}

file_stream* filesystem_open_file_handle(const file_handle* f, int mode)
{
	k_assert(f);

	file_data_block data = f->data;

	if(mode & FILE_WRITE)
	{
		if(data.flags & IS_READONLY)
			return nullptr;

		auto drive = filesystem_get_drive(data.disk_id);

		if(drive->read_only)
			return nullptr;
	}
	else
	{
		data.flags |= IS_READONLY;
	}

	if(data.flags & IS_DIR) { return nullptr; }

	auto stream = filesystem_create_stream(&data);

	if(mode & FILE_APPEND)
	{
		stream->seekpos = data.size;
	}

	return stream;
}

file_stream* filesystem_open_file(const directory_stream* rel,
								  std::string_view path,
								  int mode)
{
	k_assert(rel);

	if(const file_handle* f = filesystem_find_file_by_path(rel, path))
		return filesystem_open_file_handle(f, mode);
	else
		return nullptr;
}

int filesystem_close_file(file_stream* s)
{
	if(s == nullptr)
	{
		return -1;
	}

	filesystem_flush_buffer(s);

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

static void filesystem_flush_buffer(file_stream* s)
{
	if(s->is_dirty)
	{
		printf("flushing...\n");

		auto drive = filesystem_get_drive(s->file.disk_id);
		k_assert(drive->fs_driver->write_chunks);
		drive->fs_driver->write_chunks(	s->buffer, s->location_on_disk,
										drive->chunk_read_size, drive);
		s->is_dirty = false;

		printf("flushed\n");
	}
}

static fs_index filesystem_read_chunk(file_stream* s, fs_index chunk_index)
{
	//printf("read chunk %d\n", chunk_index);

	auto drive = filesystem_get_drive(s->file.disk_id);
	if(s->location_on_disk == chunk_index)
	{
		//great we already have data in the buffer
		return filesystem_get_next_location_on_disk(s, drive->chunk_read_size, chunk_index);
	}

	filesystem_flush_buffer(s);

	s->location_on_disk = chunk_index;

	return drive->fs_driver->read_chunks(s->buffer, chunk_index,
										 drive->chunk_read_size, drive);
}

struct fs_chunks
{
	size_t start_offset;	//offset to start in the first chunk
	size_t start_size;		//size of the first chunk

	size_t num_full_chunks;	//number of complete chunks

	size_t end_size;		//size of the last chunk
};

static fs_chunks filesystem_chunkify(size_t offset, size_t length, size_t chunk_size)
{
	size_t first_chunk = offset / chunk_size;
	size_t start_offset = offset % chunk_size;

	size_t last_chunk = (offset + length) / chunk_size;
	size_t end_size = (offset + length) % chunk_size;

	if(first_chunk == last_chunk)
	{
		end_size = 0; //start and end are in the same chunk
	}

	size_t start_size = ((length - end_size) % chunk_size);
	size_t num_chunks = (length - (start_size + end_size)) / chunk_size;

	return {start_offset, start_size, num_chunks, end_size};
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

		if(s->seekpos + len > s->file.size)
		{
			//only read what is available
			len = s->file.size - s->seekpos;
		}
	}

	auto drive = filesystem_get_drive(s->file.disk_id);

	fs_index location = filesystem_resolve_location_on_disk(s);

	auto chunks = filesystem_chunkify(s->seekpos, len, drive->chunk_read_size);

	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	if(chunks.start_size != 0)
	{
		location = filesystem_read_chunk(s, location);
		memcpy(dst_ptr, s->buffer + chunks.start_offset, chunks.start_size);
		dst_ptr += chunks.start_size;
	}

	if(chunks.num_full_chunks)
	{
		if(drive->disk->driver->allocate_buffer == nullptr)
		{
			auto size = drive->chunk_read_size * chunks.num_full_chunks;

			location = drive->fs_driver->read_chunks(dst_ptr, location, size, drive);
			dst_ptr += size;
		}
		else
		{
			auto num_chunks = chunks.num_full_chunks;
			while(num_chunks--)
			{
				location = filesystem_read_chunk(s, location);
				memcpy(dst_ptr, s->buffer, drive->chunk_read_size);
				dst_ptr += drive->chunk_read_size;
			}
		}
	}

	if(chunks.end_size != 0)
	{
		filesystem_read_chunk(s, location);
		memcpy(dst_ptr, s->buffer, chunks.end_size);
	}

	s->seekpos += len;

	return len;
}

void filesystem_allocate_space(file_stream* s, fs_index location, size_t requested_size)
{
	auto drive = filesystem_get_drive(s->file.disk_id);

	size_t allocated = drive->fs_driver->allocate_chunks(location,
														 requested_size,
														 drive);

	size_t allocated_bytes = drive->chunk_read_size * allocated;

	s->file.size = std::min(allocated_bytes, requested_size);
}

int filesystem_write_file(const void* dst_buf, size_t len, file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);
	k_assert(!(s->file.flags & IS_READONLY));

	auto drive = filesystem_get_drive(s->file.disk_id);

	k_assert(drive->fs_driver->write_chunks);

	fs_index location = filesystem_resolve_location_on_disk(s);

	if(s->seekpos + len >= s->file.size)
	{
		printf("allocating file\n");
		filesystem_allocate_space(s, location, s->seekpos + len);

		len = s->file.size - s->seekpos;
	}

	printf("writing %d bytes\n", len);

	auto chunks = filesystem_chunkify(s->seekpos, len, drive->chunk_read_size);

	const uint8_t* dst_ptr = (uint8_t*)dst_buf;

	if(chunks.start_size != 0)
	{
		location = filesystem_read_chunk(s, location);
		memcpy(s->buffer + chunks.start_offset, dst_ptr, chunks.start_size);
		s->is_dirty = true;
		dst_ptr += chunks.start_size;
	}

	if(chunks.num_full_chunks)
	{
		if(drive->disk->driver->allocate_buffer == nullptr)
		{
			auto size = drive->chunk_read_size * chunks.num_full_chunks;

			location = drive->fs_driver->write_chunks(dst_ptr, location, size, drive);
			dst_ptr += size;
		}
		else
		{
			auto num_chunks = chunks.num_full_chunks;
			while(num_chunks--)
			{
				filesystem_flush_buffer(s);
				memcpy(s->buffer, dst_ptr, drive->chunk_read_size);
				s->is_dirty = true;
				dst_ptr += drive->chunk_read_size;
			}
		}
	}

	if(chunks.end_size != 0)
	{
		filesystem_read_chunk(s, location);
		memcpy(s->buffer, dst_ptr, chunks.end_size);
		s->is_dirty = true;
	}

	s->seekpos += len;

	printf("file written\n");

	return len;
}

void filesystem_seek_file(file_stream* f, size_t pos)
{
	k_assert(f);

	f->seekpos = pos;
}

uint8_t* filesystem_allocate_buffer(const filesystem_drive* d, size_t size)
{
	k_assert(d);
	k_assert(d->driver);
	if(d->driver->allocate_buffer != nullptr)
	{
		return d->driver->allocate_buffer(size);
	}
	return (uint8_t*)malloc(size);
}

int filesystem_free_buffer(const filesystem_drive* d, uint8_t* buffer, size_t size)
{
	k_assert(d);
	k_assert(d->driver);
	if(d->driver->free_buffer != nullptr)
	{
		return d->driver->free_buffer(buffer, size);
	}
	free(buffer);
	return 0;
}

SYSCALL_HANDLER int syscall_read_file(void* dst, size_t len, file_stream* f)
{
	if(f == nullptr || dst == nullptr)
	{
		return 0;
	}
	return filesystem_read_file(dst, len, f);
}

SYSCALL_HANDLER int syscall_write_file(const void* dst, size_t len, file_stream* f)
{
	if(f == nullptr || dst == nullptr || (f->file.flags & IS_READONLY))
	{
		return 0;
	}
	return filesystem_write_file(dst, len, f);
}

SYSCALL_HANDLER
file_stream* syscall_open_file(const directory_stream* rel,
							   const char* path,
							   size_t path_len,
							   int mode)
{
	if(!rel || !path)
		return nullptr;
	else
		return filesystem_open_file(rel, {path, path_len}, mode);
}

SYSCALL_HANDLER
file_stream* syscall_open_file_handle(const file_handle* f, int mode)
{
	if(f == nullptr) { return nullptr; }

	return filesystem_open_file_handle(f, mode);
}

SYSCALL_HANDLER int syscall_close_file(file_stream* stream)
{
	if(stream == nullptr)
	{
		return -1;
	}

	filesystem_close_file(stream);
	return 0;
}