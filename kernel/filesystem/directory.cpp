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
#include <optional>
#include <charconv>

directory_stream* filesystem_open_directory_handle(const file_handle* f, int flags)
{
	k_assert(f);

	if(!(f->data.flags & IS_DIR)) { return nullptr; }

	directory_stream* d = new directory_stream{f->data};

	auto drive = filesystem_get_drive(d->data.disk_id);

	k_assert(d);
	drive->fs_driver->read_dir(d, &d->data, drive);

	return d;
}

int filesystem_delete_file(const file_handle* f)
{
	k_assert(f);
	auto drive = filesystem_get_drive(f->data.disk_id);

	if(drive->read_only)
	{
		return -1;
	}

	if(f->data.flags & IS_DIR)
	{
		using namespace std::literals;

		fs::dir_stream dir{f, 0};

		if(dir.get_ptr()->file_list.size() > 2)
		{
			return -1; //dir not empty!
		}

		for(auto f : dir.get_ptr()->file_list)
		{
			if(f.name != "."sv && f.name != ".."sv)
			{
				return -1;
			}
		}
	}

	k_assert(drive->fs_driver->delete_file);
	return drive->fs_driver->delete_file(&f->data, drive);
}

template<typename I>
static I find_file(I begin, I end, std::string_view fname)
{
	return std::find_if(begin, end, [fname](auto&& a) {
		return filesystem_names_identical(a.name, fname);
						});
}

static std::optional<file_handle> filesystem_create_file(directory_stream& d, std::string_view fname, int flags)
{
	auto drive = filesystem_get_drive(d.data.disk_id);

	if((d.data.flags & IS_READONLY) || drive->read_only)
	{
		return std::nullopt;
	}

	k_assert(drive->fs_driver->create_file);
	drive->fs_driver->create_file(fname.data(), fname.size(), static_cast<uint32_t>(flags), &d, drive);

	return d.file_list.back();
}

static std::optional<file_handle> do_find_file_by_path(directory_stream* d, std::string_view path, int mode, int flags)
{
	k_assert(d);

	if(size_t begin = path.find_first_not_of('/'); begin != std::string_view::npos)
	{
		path = path.substr(begin);
	}
	else
	{
		return std::nullopt; //our path is only made of '/'s
	}

	//if there are still '/'s in the path
	if(size_t dir_end = path.find('/'); dir_end != std::string_view::npos)
	{
		auto it = find_file(d->file_list.cbegin(),
							d->file_list.cend(), path.substr(0, dir_end));

		if(it != d->file_list.cend() && it->data.flags & IS_DIR)
		{
			if(const auto remaining_path = path.substr(dir_end + 1); remaining_path.empty())
			{
				//nothing follows the last '/'
				return *it; //just return the dir
			}
			else if(fs::dir_stream dir{&(*it), 0}; !!dir)
			{
				return dir.find_file_by_path(remaining_path, mode, flags);
			}
		}
	}
	else
	{
		auto it = find_file(d->file_list.cbegin(),
							d->file_list.cend(), path);

		if(it != d->file_list.cend())
		{
			return *it;
		}
		else if(mode & FILE_CREATE)
		{
			return filesystem_create_file(*d, path, flags);
		}
	}

	return std::nullopt;
}

std::optional<file_handle> find_file_by_path(directory_stream* d, std::string_view path, int mode, int flags)
{
	size_t colon = path.find_first_of(':');

	if(colon != path.npos)
	{
		size_t drive = 0;
		if(auto r = std::from_chars(path.cbegin(), path.cend(), drive);
		   r.ec == std::errc::invalid_argument)
		{
			return std::nullopt;
		}

		if(auto r_dir = filesystem_get_root_directory(drive); !!r_dir)
		{
			if(colon + 1 == path.size() ||
			   path.find_first_not_of('/', colon + 1) == path.npos)
			{
				return *r_dir;
			}

			fs::dir_stream ds{r_dir, 0};

			return do_find_file_by_path(ds.get_ptr(), path.substr(colon + 1), mode, flags);
		}
	}

	if(!d)
		return std::nullopt;
	else
		return do_find_file_by_path(d, path, mode, flags);
}

int filesystem_get_file_info(file_info* dst, const file_handle* src)
{
	k_assert(dst);
	k_assert(src);

	k_assert(src->name.size() < MAX_PATH);

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
	if(auto f = find_file_by_path(rel, path, mode, IS_DIR))
		return filesystem_open_directory_handle(&(*f), 0);
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
		return new file_handle{d->file_list[index]};
	}

	return nullptr;
}

SYSCALL_HANDLER
const file_handle* syscall_find_file_by_path(directory_stream* d, const char* name, size_t name_len, int mode, int flags)
{
	if(name == nullptr)
	{
		return nullptr;
	};

	if(auto file = find_file_by_path(d, std::string_view{name, name_len}, mode, flags))
	{
		return new file_handle{*file};
	}
	return nullptr;
}

SYSCALL_HANDLER int syscall_dispose_file_handle(const file_handle* f)
{
	if(f)
	{
		delete f;
	}
	return 0;
}


int syscall_delete_file(const file_handle* f)
{
	return f ? filesystem_delete_file(f) : -1;
}
