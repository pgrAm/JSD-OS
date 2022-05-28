#ifndef DRIVER_LOADER_H
#define DRIVER_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <kernel/filesystem.h>

void load_drivers();

SYSCALL_HANDLER int load_driver(directory_stream* cwd,
								 const file_handle* file);

#ifdef __cplusplus
}
#endif
#endif