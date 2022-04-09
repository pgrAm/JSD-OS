#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/drives.h>
#include <kernel/kassert.h>

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>

directory_stream* filesystem_open_directory_handle(const file_handle* f, int flags)
{
	k_assert(f);

	if(!(f->data.flags & IS_DIR)) { return nullptr; }

	directory_stream* d = new directory_stream{f->data};

	auto drive = filesystem_get_drive(f->data.disk_id);

	k_assert(d);
	drive->fs_driver->read_dir(d, &f->data, drive);

	return d;
}

static const file_handle* filesystem_find_file_in_dir(const directory_stream& d, std::string_view fname)
{
	auto it = std::find_if(d.file_list.cbegin(), d.file_list.cend(), [fname](auto&& a) {
		return filesystem_names_identical(a.name, fname);
						   });
	return it != d.file_list.cend() ? &(*it) : nullptr;
}

const file_handle* filesystem_create_file(directory_stream* d, std::string_view fname, int flags)
{
	k_assert(d);

	auto drive = filesystem_get_drive(d->data.disk_id);

	if((d->data.flags & IS_READONLY) || drive->read_only)
	{
		return nullptr;
	}

	k_assert(drive->fs_driver->create_file);
	drive->fs_driver->create_file(fname.data(), fname.size(), static_cast<uint32_t>(flags), d, drive);

	return &d->file_list.back();
}

static file_handle found;

const file_handle* filesystem_find_file_by_path(directory_stream* d, std::string_view path, int mode, int flags)
{
	k_assert(d);

	if(size_t begin = path.find_first_not_of('/'); begin != std::string_view::npos)
	{
		path = path.substr(begin);
	}
	else
	{
		return nullptr; //our path is only made of '/'s
	}

	//if there are still '/'s in the path
	if(size_t dir_end = path.find('/'); dir_end != std::string_view::npos)
	{
		const file_handle* dh = filesystem_find_file_in_dir(*d, path.substr(0, dir_end));

		if(dh && (dh->data.flags & IS_DIR))
		{
			const auto remaining_path = path.substr(dir_end + 1);

			if(remaining_path.empty()) //nothing follows the last '/'
			{
				return dh; //just return the dir
			}

			fs::dir_stream dir{dh, 0};
			if(dir)
			{
				if(auto f = dir.find_file_by_path(remaining_path); !!f)
				{
					found = *f;
					return &found;// new file_handle{*f};
				}
			}
		}

		return nullptr;
	}
	else
	{
		auto f = filesystem_find_file_in_dir(*d, path);
		if(!f)
		{
			if(mode & FILE_CREATE)
			{
				return filesystem_create_file(d, path, flags);
			}
		}

		return f;
	}
}

int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	k_assert(dst);
	k_assert(src);

	memcpy(dst->name, src->name.c_str(), src->name.size() + 1);
	dst->name_len = src->name.size();

	dst->size = src->data.size;
	dst->flags = src->data.flags;
	dst->time_created = src->time_created;
	dst->time_modified = src->time_modified;

	return 0;
}

int filesystem_close_directory(directory_stream* stream)
{
	delete stream;
	return 0;
}

directory_stream* filesystem_open_directory(directory_stream* rel,
											std::string_view path,
											int mode)
{
	k_assert(rel);

	if(const file_handle* f = filesystem_find_file_by_path(rel, path, mode, IS_DIR))
		return filesystem_open_directory_handle(f, IS_DIR);
	else
		return nullptr;
}

SYSCALL_HANDLER directory_stream* syscall_open_directory_handle(const file_handle* f, int flags)
{
	if(f == nullptr) { return nullptr; }

	return filesystem_open_directory_handle(f, flags);
}

SYSCALL_HANDLER
int syscall_close_directory(directory_stream* stream)
{
	if(stream == nullptr)
	{
		return -1;
	}

	return filesystem_close_directory(stream);
}

SYSCALL_HANDLER
int syscall_get_file_info(file_info* dst, const file_handle* src)
{
	if(src == nullptr || dst == nullptr) return -1;
	//we should also check that these adresses are mapped properly
	//maybe do a checksum too?

	return filesystem_get_file_info(dst, src);
}

SYSCALL_HANDLER
const file_handle* syscall_get_file_in_dir(const directory_stream* d, size_t index)
{
	if(d != nullptr && index < d->file_list.size())
	{
		return &(d->file_list[index]);
	}

	return nullptr;
}

SYSCALL_HANDLER
const file_handle* syscall_find_file_by_path(directory_stream* d, const char* name, size_t name_len, int mode, int flags)
{
	if(name == nullptr || d == nullptr)
	{
		return nullptr;
	};

	return filesystem_find_file_by_path(d, std::string_view{name, name_len}, mode, flags);
}