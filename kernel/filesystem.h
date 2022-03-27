#ifndef FILESYSTEM_H
#define FILESYSTEM_H

struct file_handle;
struct file_stream;
struct directory_stream;

#include <time.h>
#include <stdbool.h>

#include <kernel/syscall.h>
#include <api/files.h>

#ifdef __cplusplus

#include <string_view>

extern "C" {

size_t filesystem_get_num_drives();
const file_handle* filesystem_get_root_directory(size_t drive);
const file_handle* filesystem_find_file_by_path(const directory_stream* rel, std::string_view path);
int filesystem_get_file_info(file_info* dst, const file_handle* src);

file_stream* filesystem_open_file_handle(const file_handle* f, int mode);
file_stream* filesystem_open_file(const directory_stream* rel, std::string_view path, int mode);
int filesystem_read_file(void* buf, size_t len, file_stream* f);
int filesystem_write_file(const void* buf, size_t len, file_stream* f);
void filesystem_seek_file(file_stream* f, size_t pos);
size_t filesystem_get_pos(file_stream* f);
int filesystem_close_file(file_stream* f);

directory_stream* filesystem_open_directory_handle(const file_handle* f, int flags);
directory_stream* filesystem_open_directory(const directory_stream* rel, std::string_view path, int flags);
int filesystem_close_directory(directory_stream* dir);

#else
typedef struct file_handle file_handle;
typedef struct file_stream file_stream;
typedef struct directory_stream directory_stream;
#endif

SYSCALL_HANDLER const file_handle* syscall_get_root_directory(size_t drive);
SYSCALL_HANDLER const file_handle* syscall_get_file_in_dir(const directory_stream* d, size_t index);
SYSCALL_HANDLER const file_handle* syscall_find_file_by_path(const directory_stream* rel, const char* path, size_t path_len);
SYSCALL_HANDLER int syscall_get_file_info(file_info* dst, const file_handle* src);
SYSCALL_HANDLER directory_stream* syscall_open_directory_handle(const file_handle* f, int flags);
SYSCALL_HANDLER int syscall_close_directory(directory_stream* dir);
SYSCALL_HANDLER file_stream* syscall_open_file_handle(const file_handle* f, int mode);
SYSCALL_HANDLER file_stream* syscall_open_file(const directory_stream* rel, const char* path, size_t path_len, int mode);
SYSCALL_HANDLER int syscall_read_file(void* dst, size_t len, file_stream* f);
SYSCALL_HANDLER int syscall_write_file(const void* dst, size_t len, file_stream* f);
SYSCALL_HANDLER int syscall_close_file(file_stream* f);

typedef enum {
	MOUNT_SUCCESS = 0,

	//means the drive does not contain a filesystem that can be handled by the driver
	UNKNOWN_FILESYSTEM = 1,

	//means the drive failed to mount for some reason
	MOUNT_FAILURE = 2,

	//means the drive cannot be handled by the driver
	DRIVE_NOT_SUPPORTED = 3
} mount_status;

#ifdef __cplusplus
}

namespace fs
{
	namespace {
		class stream_base {
		public:
			int read(void* dst, size_t len)
			{
				return filesystem_read_file(dst, len, get_ptr());
			}

			void seek(size_t pos)
			{
				filesystem_seek_file(get_ptr(), pos);
			}

			operator bool() const
			{
				return !!get_ptr();
			}

		protected:
			explicit stream_base(file_stream* f)
				: m_stream{f}
			{}

			file_stream* get_ptr() { return m_stream; }
			const file_stream* get_ptr() const { return m_stream; }

		private:
			file_stream* m_stream;
		};

		class dir_stream_base {
		public:
			const file_handle* find_file_by_path(std::string_view path)
			{
				return filesystem_find_file_by_path(get_ptr(), path);
			}

			operator bool() const
			{
				return !!get_ptr();
			}

			directory_stream* get_ptr() { return m_stream; }
			const directory_stream* get_ptr() const { return m_stream; }

		protected:
			explicit dir_stream_base(directory_stream* f)
				: m_stream{f}
			{}

		private:
			directory_stream* m_stream;
		};
	}

	class stream : public stream_base
	{
	public:
		class ref : public stream_base
		{
		public:
			ref(stream& s)
				: stream_base{s.get_ptr()}
			{}
		};

		stream(const file_handle* f, int flags)
			: stream_base{filesystem_open_file_handle(f, flags)}
		{}
		stream(const directory_stream* rel, std::string_view path, int flags)
			: stream_base{filesystem_open_file(rel, path, flags)}
		{}
		explicit stream(file_stream* f)
			: stream_base{f}
		{}

		stream(const stream&) = delete;
		stream& operator=(const stream&) = delete;

		~stream()
		{
			filesystem_close_file(get_ptr());
		}
	};

	class dir_stream : public dir_stream_base
	{
	public:
		class ref : public dir_stream_base
		{
		public:
			ref(dir_stream& s)
				: dir_stream_base{s.get_ptr()}
			{}
		};

		dir_stream(const file_handle* f, int flags)
			: dir_stream_base{filesystem_open_directory_handle(f, flags)}
		{}
		dir_stream(const ref rel, std::string_view path, int flags)
			: dir_stream_base{filesystem_open_directory(rel.get_ptr(), path, flags)}
		{}
		dir_stream(const directory_stream* rel, std::string_view path, int flags)
			: dir_stream_base{filesystem_open_directory(rel, path, flags)}
		{}
		explicit dir_stream(directory_stream* f)
			: dir_stream_base{f}
		{}

		dir_stream(const dir_stream&) = delete;
		dir_stream& operator=(const dir_stream&) = delete;

		~dir_stream()
		{
			filesystem_close_directory(get_ptr());
		}
	};

	using stream_ref = stream::ref;
	using dir_stream_ref = dir_stream::ref;
}


#endif

#endif