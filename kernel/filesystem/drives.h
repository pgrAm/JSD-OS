#ifndef FS_DRIVES_H
#define FS_DRIVES_H

struct filesystem_virtual_drive;

#ifdef __cplusplus
extern "C" {
#else
typedef struct filesystem_virtual_drive filesystem_virtual_drive;
#endif

size_t filesystem_get_num_drives();

filesystem_virtual_drive* filesystem_get_drive(size_t index);

#ifdef __cplusplus
}
#endif

#endif