#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <kernel/filesystem.h>

#include "fat12.h"

#define FAT12_EOF 0xFF8

#define DEFAULT_SECTOR_SIZE 512

static int fat12_mount_disk(filesystem_drive* d);
static size_t fat12_read_clusters(uint8_t* dest, size_t cluster, size_t num_bytes, const filesystem_drive* d);
static size_t fat12_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_drive* fd);
static void fat12_read_dir(directory_handle* dest, const file_handle* location, const filesystem_drive* fd);

struct fat12_drive
{
	size_t root_size;
	size_t fats_size;
	size_t root_location;
	size_t datasector;
	size_t cluster_size;
	size_t sectors_per_cluster;
	size_t bytes_per_sector;
	size_t root_entries;
	size_t reserved_sectors;

	size_t blocks_per_sector;

	size_t cached_fat_sector; //the sector index of the cached fat data
	uint8_t* fat; //the file allocation table
};

typedef struct fat12_drive fat12_drive;

enum directory_attributes
{
	READ_ONLY = 0x01,
	HIDDEN = 0x02,
	SYSTEM = 0x04,
	VOLUME_ID = 0x08,
	DIRECTORY = 0x10,
	ARCHIVE = 0x20,
	LFN = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID
};

typedef struct 
{
	char 		name[8];
	char 		extension[3];
	uint8_t 	attributes;
	uint8_t		reserved;
	uint8_t		created_time_ms;
	uint16_t	created_time;
	uint16_t	created_date;
	uint16_t	accessed_date;
	uint16_t	first_cluster_hi;
	uint16_t	modified_time;
	uint16_t	modified_date;
	uint16_t	first_cluster;
	uint32_t	file_size;
} __attribute__((packed)) fat_directory_entry;

typedef struct
{
	uint8_t		boot_code[3];
	uint8_t		oem_ID[8];
	uint16_t	bytes_per_sector;
	uint8_t		sectors_per_cluster;
	uint16_t	reserved_sectors;
	uint8_t		number_of_FATs;	
	uint16_t	root_entries;	
	uint16_t	total_sectors;	
	uint8_t		media;			
	uint16_t	sectors_per_FAT;	
	uint16_t	sectors_per_track;
	uint16_t	heads_per_cylinder;
	uint8_t		hidden_sectors;
	uint32_t	total_sectors_big;
} __attribute__((packed)) bpb;

filesystem_driver fat_driver = {
	fat12_mount_disk,
	fat12_get_relative_cluster,
	fat12_read_clusters,
	fat12_read_dir
};

void fat_init()
{
	filesystem_add_driver(&fat_driver);
}

static inline size_t fat12_cluster_to_lba(const fat12_drive* d, size_t cluster)
{
	return (cluster - 2) * d->sectors_per_cluster + d->datasector;
}

static int fat12_read_bios_block(const filesystem_drive* fd, uint8_t* boot_sector)
{
	fat12_drive* d = (fat12_drive*)fd->fs_impl_data;

	filesystem_read_blocks_from_disk(fd, 0, boot_sector, DEFAULT_SECTOR_SIZE / fd->minimum_block_size);

	bpb* bios_block = (bpb*)boot_sector;

	if(bios_block->boot_code[0] != 0xEB || bios_block->boot_code[2] != 0x90)
	{
		return UNKNOWN_FILESYSTEM;
	}

	d->root_size 			= (sizeof(fat_directory_entry) * bios_block->root_entries) / bios_block->bytes_per_sector; //number of sectors in root dir
	d->fats_size 			= bios_block->number_of_FATs * bios_block->sectors_per_FAT;
	d->root_location 		= d->fats_size + bios_block->reserved_sectors;
	d->datasector 			= d->root_location + d->root_size;
	d->cluster_size 		= bios_block->sectors_per_cluster * bios_block->bytes_per_sector;
	d->sectors_per_cluster 	= bios_block->sectors_per_cluster;
	d->bytes_per_sector 	= bios_block->bytes_per_sector;
	d->root_entries 		= bios_block->root_entries;
	d->reserved_sectors		= bios_block->reserved_sectors;
	d->blocks_per_sector	= d->bytes_per_sector / fd->minimum_block_size;

	if(d->bytes_per_sector < fd->minimum_block_size)
	{
		return DRIVE_NOT_SUPPORTED;
	}

	return MOUNT_SUCCESS;
}

static time_t fat12_time_to_time_t(uint16_t date, uint16_t time)
{
	struct tm file_time = {

		.tm_sec = (time & 0x001F) << 1,
		.tm_min = (time & 0x7E0) >> 5,
		.tm_hour = (time & 0xF800) >> 11,

		.tm_mday = (date & 0x001F),
		.tm_mon = ((date & 0x01E0) >> 5) - 1,
		.tm_year = ((date & 0xFE00) >> 9) + 80,

		.tm_wday = 0,
		.tm_yday = 0,
		.tm_isdst = 0
	};

	return mktime(&file_time);
}

static size_t str_terminate(char* str, size_t max_len)
{
	size_t ext_length = 0;
	while(ext_length < max_len && str[ext_length] != ' ')
	{
		ext_length++;
	}
	str[ext_length] = '\0';

	return ext_length;
}

static bool fat12_read_dir_entry(file_handle* dest, const fat_directory_entry* entry, size_t disk_num)
{
	if(entry->name[0] == 0) {
		return false;
	} //end of directory

	char* full_name = (char*)malloc(13 * sizeof(char));

	dest->name = (char*)malloc(9 * sizeof(char));
	memcpy(dest->name, entry->name, 8);
	str_terminate(dest->name, 8);

	dest->type = (char*)malloc(4 * sizeof(char));
	memcpy(dest->type, entry->extension, 3);
	size_t ext_length = str_terminate(dest->type, 3);

	strcpy(full_name, strtok(dest->name, " "));
	if(ext_length > 0)
	{
		strcat(full_name, ".");
		strcat(full_name, strtok(dest->type, " "));
	}

	dest->full_name = full_name;
	dest->size = entry->file_size;

	dest->time_created = fat12_time_to_time_t(entry->created_date, entry->created_time);
	dest->time_modified = fat12_time_to_time_t(entry->modified_date, entry->modified_time);

	dest->disk = disk_num;
	dest->location_on_disk = entry->first_cluster;

	uint32_t flags = 0;

	if(entry->attributes & DIRECTORY)
	{
		flags |= IS_DIR;
	}

	dest->flags = flags;

	return true;
}

static void fat12_read_root_dir(directory_handle* dest, const filesystem_drive* fd)
{
	fat12_drive* d = (fat12_drive*)fd->fs_impl_data;

	size_t max_num_files = d->root_entries;

	dest->name = "";
	dest->file_list = (file_handle*)malloc(max_num_files * sizeof(file_handle));
	dest->drive = fd->index;

	size_t buffer_size = d->root_entries * sizeof(fat_directory_entry);

	fat_directory_entry* root_directory = 
		(fat_directory_entry*)filesystem_allocate_buffer(fd, buffer_size);

	filesystem_read_blocks_from_disk(fd, d->root_location, 
									 (uint8_t*)root_directory, 
									 d->root_size * d->blocks_per_sector);

	size_t num_files = 0;
	for(size_t i = 0; i < max_num_files; i++)
	{
		if(root_directory[i].attributes & LFN)
		{
			continue;
		}

		if(!fat12_read_dir_entry(&dest->file_list[num_files], 
								 &root_directory[i], fd->index)) 
		{
			break;
		} //end of directory

		num_files++;
	}

	dest->num_files = num_files;

	filesystem_free_buffer(fd, (uint8_t*)root_directory, buffer_size);
}

/*char* strdup(const char* str1)
{
	size_t len = strlen(str1);
	char* result = (char*)malloc(len);
	memcpy(result, str1, len + 1);
	return result;
}*/

static void fat12_read_dir(directory_handle* dest, const file_handle* dir, const filesystem_drive* fd)
{
	file_stream* dir_stream = filesystem_create_stream(dir);

	size_t max_num_files = 100;// dir->size / sizeof(fat_directory_entry);

	//printf("\ndir %u\n", dir->size);

	dest->name = "";// strdup(dir->full_name);
	dest->file_list = (file_handle*)malloc(max_num_files * sizeof(file_handle));
	dest->drive = fd->index;

	size_t num_files = 0;

	for(size_t i = 0; i < max_num_files; i++)
	{
		fat_directory_entry entry;
		if(filesystem_read_file(&entry, sizeof(fat_directory_entry), dir_stream) 
		   != sizeof(fat_directory_entry))
		{
			break;
		}

		if(entry.attributes & LFN) 
		{
			continue;
		}

		if (!fat12_read_dir_entry(&dest->file_list[num_files], &entry, fd->index))
		{
			break; 
		}

		num_files++;
	}

	dest->num_files = num_files;

	filesystem_close_file(dir_stream);
}

static size_t fat12_get_next_cluster(size_t cluster, const filesystem_drive* fd)
{
	fat12_drive* d = (fat12_drive*)fd->fs_impl_data;
	
	size_t fat_offset = cluster + (cluster / 2);
	size_t fat_sector = d->reserved_sectors + (fat_offset / d->bytes_per_sector);
	size_t ent_offset = fat_offset % d->bytes_per_sector;
	
	if(d->cached_fat_sector != fat_sector)
	{
		filesystem_read_blocks_from_disk(fd, fat_sector, d->fat, d->blocks_per_sector);

		d->cached_fat_sector = fat_sector;
	}
	
	uint16_t table_value = *(uint16_t*)&d->fat[ent_offset];

	table_value = (cluster & 0x0001) ? table_value >> 4 : table_value & 0x0FFF;
	
	return table_value;
}

static size_t fat12_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_drive* fd)
{
	fat12_drive* f = (fat12_drive*)fd->fs_impl_data;

	size_t num_clusters = byte_offset / f->cluster_size;

	while (num_clusters--)
	{
		cluster = fat12_get_next_cluster(cluster, fd);

		if (cluster >= FAT12_EOF) { break; }
	}

	return cluster;
}

static int fat12_mount_disk(filesystem_drive* d)
{
	fat12_drive* f = malloc(sizeof(fat12_drive));
	d->fs_impl_data = f;

	uint8_t* boot_sector = filesystem_allocate_buffer(d, DEFAULT_SECTOR_SIZE);

	int err = fat12_read_bios_block(d, boot_sector);
	if(err != MOUNT_SUCCESS)
	{
		filesystem_free_buffer(d, boot_sector, DEFAULT_SECTOR_SIZE);
		free(f);
		d->fs_impl_data = NULL;
		return err;
	}

	//allocate fat sector
	if(f->bytes_per_sector > DEFAULT_SECTOR_SIZE)
	{
		filesystem_free_buffer(d, boot_sector, DEFAULT_SECTOR_SIZE);
		f->fat = filesystem_allocate_buffer(d, f->bytes_per_sector);
	}
	else
	{
		f->fat = boot_sector;
	}

	f->cached_fat_sector = 0;
	
	fat12_read_root_dir(&d->root, d);

	return MOUNT_SUCCESS;
}

static size_t fat12_read_clusters(uint8_t* dest, size_t cluster, size_t bufferSize, const filesystem_drive *d)
{
	if (cluster >= FAT12_EOF)
	{
		//puts("read attempted, but eof reached");
		return cluster;
	}

	fat12_drive* f = (fat12_drive*)d->fs_impl_data;
	
	size_t num_clusters = bufferSize / f->cluster_size;

	while(num_clusters--)
	{
		//printf("Reading from cluster %d to %X\n", cluster, dest);
		//printf("lba %d\n", fat12_cluster_to_lba(f, cluster));
		filesystem_read_blocks_from_disk(d, fat12_cluster_to_lba(f, cluster), dest, f->sectors_per_cluster * f->blocks_per_sector);

		//printf("value = %X\n", *((uint32_t*)dest));

		dest += f->cluster_size;
		
		cluster = fat12_get_next_cluster(cluster, d);
		
		if(cluster >= FAT12_EOF)
		{
			//puts("eof reached");
			break;
		}			
	}
	
	return cluster;
}

