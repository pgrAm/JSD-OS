#ifndef FAT12_H
#define FAT12_H

#include <stdbool.h>
#include "../kernel/filesystem.h"

typedef enum
{
	TYPE_FLOPPY = 0
} physicalType;

typedef struct 
{
	bool mounted;
	size_t index; //physical index
	physicalType type; //physical type
	
	size_t root_size; 	
	size_t fats_size;	
	size_t root_location;
	size_t datasector; 	
	size_t cluster_size;
	size_t sectors_per_cluster;
	size_t bytes_per_sector;
	size_t root_entries;
	size_t reserved_sectors;
	
	size_t cached_fat_sector; //the sector index of the cached fat data
	uint8_t* fat; //the file allocation table
} fat12_drive;

void fat12_mount_disk(filesystem_drive *d, size_t logicalDriveNumber);
size_t fat12_read_clusters(uint8_t* dest, size_t bufferSize, size_t cluster, const filesystem_drive *d);
size_t fat12_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_drive* fd);
#endif