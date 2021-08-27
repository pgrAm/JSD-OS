#ifndef FAT12_H
#define FAT12_H

#include <stdbool.h>
#include "../kernel/filesystem.h"

struct fat12_drive;
typedef struct fat12_drive fat12_drive;

bool fat12_mount_disk(filesystem_drive *d);
size_t fat12_read_clusters(uint8_t* dest, size_t cluster, size_t num_bytes, const filesystem_drive *d);
size_t fat12_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_drive* fd);
void fat12_read_dir(directory_handle* dest, fs_index location, const filesystem_drive* fd);
#endif