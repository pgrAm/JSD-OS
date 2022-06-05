#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/drives.h>
#include <kernel/kassert.h>

//an instance of an open file
struct file_stream
{
	file_data_block file;
	bool modified;
};

file_stream* filesystem_create_stream(const file_data_block* f)
{
	k_assert(f);
	return new file_stream { *f, false };
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

file_size_t filesystem_read_file(file_size_t offset, void* dst_buf, size_t len,
								 file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);

	if(!(s->file.flags & IS_DIR))
	{
		if(offset >= s->file.size)
		{
			return 0; // EOF
		}

		if(offset + len > s->file.size)
		{
			//only read what is available
			len = s->file.size - offset;
		}
	}

	auto drive = filesystem_get_drive(s->file.disk_id);

	uint8_t* dst_ptr = (uint8_t*)dst_buf;

	drive->fs_driver->read_chunks(dst_ptr, s->file.location_on_disk, offset,
								  len, &s->file, drive);
	return len;
}

void filesystem_allocate_space(file_stream* s, fs_index location,
							   file_size_t requested_size)
{
	auto drive = filesystem_get_drive(s->file.disk_id);
	auto allocated_size =
		drive->fs_driver->allocate_chunks(location, requested_size, &s->file,
										  drive);

	s->file.size = std::min(allocated_size, requested_size);
}

file_size_t filesystem_write_file(file_size_t offset, const void* dst_buf,
							 size_t len, file_stream* s)
{
	k_assert(dst_buf);
	k_assert(s);
	k_assert(!(s->file.flags & IS_READONLY));

	auto drive = filesystem_get_drive(s->file.disk_id);

	k_assert(drive->fs_driver->write_chunks);

	if(!(s->file.flags & IS_DIR) && offset + len >= s->file.size)
	{
		filesystem_allocate_space(s, s->file.location_on_disk, offset + len);
		len = s->file.size - offset;
	}

	const uint8_t* dst_ptr = (const uint8_t*)dst_buf;

	drive->fs_driver->write_chunks(dst_ptr, s->file.location_on_disk, offset,
								   len, &s->file, drive);
	s->modified = true;
	return len;
}

SYSCALL_HANDLER file_size_t syscall_read_file(file_size_t offset, void* dst,
											  size_t len, file_stream* f)
{
	if(f == nullptr || dst == nullptr)
	{
		return 0;
	}
	return filesystem_read_file(offset, dst, len, f);
}

SYSCALL_HANDLER file_size_t syscall_write_file(file_size_t offset,
											   const void* dst, size_t len,
											   file_stream* f)
{
	if(f == nullptr || dst == nullptr || (f->file.flags & IS_READONLY))
	{
		return 0;
	}
	return filesystem_write_file(offset, dst, len, f);
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