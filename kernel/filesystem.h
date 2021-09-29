#ifndef FILESYSTEM_H
#define FILESYSTEM_H

typedef size_t fs_index;
struct file_handle;
struct file_stream;
struct directory_handle;

#ifdef __cplusplus
extern "C" {
#else
typedef struct file_handle file_handle;
typedef struct file_stream file_stream;
typedef struct directory_handle directory_handle;
#endif

#include <time.h>
#include <stdbool.h>

#include <kernel/syscall.h>
#include <api/files.h>

SYSCALL_HANDLER directory_handle* filesystem_get_root_directory(size_t drive);

void filesystem_seek_file(file_stream* f, size_t pos);
void filesystem_set_default_drive(size_t index);

SYSCALL_HANDLER file_handle* filesystem_get_file_in_dir(const directory_handle* d, size_t index);
SYSCALL_HANDLER file_handle* filesystem_find_file_by_path(const directory_handle* rel, const char* name);

SYSCALL_HANDLER int filesystem_get_file_info(file_info* dst, const file_handle* src);

SYSCALL_HANDLER directory_handle* filesystem_open_directory_handle(const file_handle* f, int flags);
SYSCALL_HANDLER directory_handle* filesystem_open_directory(const directory_handle* rel, const char* name, int flags);
SYSCALL_HANDLER int filesystem_close_directory(directory_handle* dir);

SYSCALL_HANDLER file_stream* filesystem_open_file_handle(file_handle* f, int flags);
SYSCALL_HANDLER file_stream* filesystem_open_file(const directory_handle* rel, const char* name, int flags);
SYSCALL_HANDLER int filesystem_read_file(void* dst, size_t len, file_stream* f);
SYSCALL_HANDLER int filesystem_close_file(file_stream* f);

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
#endif

#endif