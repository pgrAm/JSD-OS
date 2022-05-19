#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/drives.h>
#include <kernel/kassert.h>

//an instance of an open file
struct file_stream
{
	file_data_block file;
	size_t seekpos;
	bool modified;
};

file_stream* filesystem_create_stream(const file_data_block* f)
{
	k_assert(f);
	return new file_stream { *f, 0, false };
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

file_stream* filesystem_open_file(directory_stream* rel,
								  std::string_view path,
								  int mode)
{
	k_assert(rel);

	if(auto f = find_file_by_path(rel, path, mode, 0))
		return filesystem_open_file_handle(&(*f), mode);
	else
		return nullptr;
}

int filesystem_close_file(file_stream* s)
{
	if(s == nullptr)
	{
		return -1;
	}

	if(s->modified)
	{
		k_assert(!(s->file.flags & IS_READONLY));

		auto drive = filesystem_get_drive(s->file.disk_id);
		k_assert(drive->fs_driver->flush_file);

		drive->fs_driver->flush_file(&s->file, drive);
	}

	delete s;
	return 0;
}

size_t filesystem_read_file(void* dst_buf, size_t len, file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);

	if(!(s->file.flags & IS_DIR))
	{
		if(s->seekpos >= s->file.size)
		{
			return 0; // EOF
		}

		if(s->seekpos + len > s->file.size)
		{
			//only read what is available
			len = s->file.size - s->seekpos;
		}
	}

	auto drive = filesystem_get_drive(s->file.disk_id);

	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	drive->fs_driver->read_chunks(dst_ptr, s->file.location_on_disk, s->seekpos, len, &s->file, drive);

	s->seekpos += len;

	return len;
}

void filesystem_allocate_space(file_stream* s, fs_index location, size_t requested_size)
{
	auto drive = filesystem_get_drive(s->file.disk_id);

	size_t allocated_size = 
		drive->fs_driver->allocate_chunks(location, requested_size, &s->file, drive);

	s->file.size = std::min(allocated_size, requested_size);
}

size_t filesystem_write_file(const void* dst_buf, size_t len, file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);
	k_assert(!(s->file.flags & IS_READONLY));

	auto drive = filesystem_get_drive(s->file.disk_id);

	k_assert(drive->fs_driver->write_chunks);

	if(!(s->file.flags & IS_DIR) && s->seekpos + len >= s->file.size)
	{
		filesystem_allocate_space(s, s->file.location_on_disk, s->seekpos + len);
		len = s->file.size - s->seekpos;
	}

	const uint8_t* dst_ptr = (const uint8_t*)dst_buf;

	drive->fs_driver->write_chunks(dst_ptr, s->file.location_on_disk, s->seekpos, len, &s->file, drive);

	s->seekpos += len;
	s->modified = true;

	return len;
}

size_t filesystem_get_pos(file_stream* f)
{
	k_assert(f);
	return f->seekpos;
}

void filesystem_seek_file(file_stream* f, size_t pos)
{
	k_assert(f);
	f->seekpos = pos;
}

SYSCALL_HANDLER size_t syscall_read_file(void* dst, size_t len, file_stream* f)
{
	if(f == nullptr || dst == nullptr)
	{
		return 0;
	}
	return filesystem_read_file(dst, len, f);
}

SYSCALL_HANDLER size_t syscall_write_file(const void* dst, size_t len,
										  file_stream* f)
{
	if(f == nullptr || dst == nullptr || (f->file.flags & IS_READONLY))
	{
		return 0;
	}
	return filesystem_write_file(dst, len, f);
}

SYSCALL_HANDLER
file_stream* syscall_open_file(directory_stream* rel,
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