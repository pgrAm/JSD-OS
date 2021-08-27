#ifndef RDFS_H
#define RDFS_H

#include <stdbool.h>
#include "../kernel/filesystem.h"

bool rdfs_mount_disk(filesystem_drive* d);
fs_index rdfs_get_relative_location(fs_index location, size_t byte_offset, const filesystem_drive* fd);
fs_index rdfs_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_drive* fd);
void rdfs_read_dir(directory_handle* dest, fs_index location, const filesystem_drive* fd);

#endif