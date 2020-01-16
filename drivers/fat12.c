#include "../drivers/floppy.h"
#include "../drivers/fat12.h"
#include "../kernel/filesystem.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define FAT12_EOF 0xFF8

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

size_t fat12_cluster_to_lba(const fat12_drive* d, size_t cluster)
{
	return (cluster - 2) * d->sectors_per_cluster + d->datasector;
}

void fat12_read_bios_block(fat12_drive* d)
{
	uint8_t boot_sector[512];
	
	floppy_read_sectors(d->index, 0, boot_sector, 1);
	
	bpb* bios_block = (bpb*)(boot_sector + 0x00B);
	
	d->root_size 			= (sizeof(fat_directory_entry) * bios_block->root_entries) / bios_block->bytes_per_sector; //number of sectors in root dir
	d->fats_size 			= bios_block->number_of_FATs * bios_block->sectors_per_FAT;
	d->root_location 		= d->fats_size + bios_block->reserved_sectors;
	d->datasector 			= d->root_location + d->root_size;
	d->cluster_size 		= bios_block->sectors_per_cluster * bios_block->bytes_per_sector;
	d->sectors_per_cluster 	= bios_block->sectors_per_cluster;
	d->bytes_per_sector 	= bios_block->bytes_per_sector;
	d->root_entries 		= bios_block->root_entries;
	d->reserved_sectors		= bios_block->reserved_sectors;
}

time_t fat12_time_to_time_t(uint16_t date, uint16_t time)
{
	struct tm file_time;
	
	file_time.tm_sec	= (time & 0x001F) << 1;
	file_time.tm_min	= (time & 0x7E0) >> 5;
	file_time.tm_hour	= (time & 0xF800) >> 11;
	
	file_time.tm_mday	= (date & 0x001F);
	file_time.tm_mon	= ((date & 0x01E0) >> 5) - 1;
	file_time.tm_year	= ((date & 0xFE00) >> 9) + 80;
	
	return mktime(&file_time);
}

void fat12_do_read_dir(directory_handle* dest, fs_index location, size_t locicaldrivenum, const fat12_drive* selected_drive)
{
	uint8_t* raw_directory = (uint8_t*)malloc(selected_drive->bytes_per_sector);

	dest->name = "";
	dest->file_list = (file_handle*)malloc(selected_drive->root_entries * sizeof(file_handle));
	dest->drive = locicaldrivenum;

	size_t num_files = 0;

	for (size_t sector = 0; sector < 1; sector++)
	{
		size_t lba = (location == 0) ? selected_drive->root_location : fat12_cluster_to_lba(selected_drive, location);

		floppy_read_sectors(selected_drive->index, lba + sector, raw_directory, 1);

		for (size_t entry_number = 0; entry_number < (selected_drive->bytes_per_sector / sizeof(fat_directory_entry)); entry_number++)
		{
			fat_directory_entry* entry = (fat_directory_entry*)(raw_directory + entry_number * sizeof(fat_directory_entry));

			if (entry->name[0] == 0) { break; } //end of directory

			if (!(entry->attributes & LFN))
			{
				char* full_name = (char*)malloc(13 * sizeof(char));

				dest->file_list[num_files].name = (char*)malloc(9 * sizeof(char));
				memcpy(dest->file_list[num_files].name, entry->name, 8);
				dest->file_list[num_files].name[8] = '\0';

				dest->file_list[num_files].type = (char*)malloc(4 * sizeof(char));
				memcpy(dest->file_list[num_files].type, entry->extension, 3);

				//count chars in extenstion
				size_t ext_length = 0;
				for (; ext_length < 3 && entry->extension[ext_length] != ' '; ext_length++);
				dest->file_list[num_files].type[ext_length] = '\0';

				strcpy(full_name, strtok(dest->file_list[num_files].name, " "));
				if (ext_length > 0)
				{
					strcat(full_name, ".");
					strcat(full_name, strtok(dest->file_list[num_files].type, " "));
				}

				dest->file_list[num_files].full_name = full_name;

				dest->file_list[num_files].size = entry->file_size;

				dest->file_list[num_files].time_created = fat12_time_to_time_t(entry->created_date, entry->created_time);
				dest->file_list[num_files].time_modified = fat12_time_to_time_t(entry->modified_date, entry->modified_time);

				dest->file_list[num_files].disk = locicaldrivenum;
				dest->file_list[num_files].location_on_disk = entry->first_cluster;

				uint32_t flags = 0;

				if (entry->attributes & DIRECTORY)
				{
					flags |= IS_DIR;
				}

				dest->file_list[num_files].flags = flags;

				num_files++;
			}
		}
	}

	dest->num_files = num_files;

	free(raw_directory);
}

void fat12_read_dir(directory_handle* dest, fs_index location, const filesystem_drive* fd)
{
	fat12_do_read_dir(dest, location, fd->index, (fat12_drive*)fd->impl_data);
}

void fat12_mount_directory(directory_handle* root, const fat12_drive* selected_drive, size_t logicalDriveNumber)
{
	fat12_do_read_dir(root, 0, logicalDriveNumber, selected_drive);
}

size_t fat12_get_next_cluster(size_t cluster, const filesystem_drive* fd)
{
	fat12_drive* d = (fat12_drive*)fd->impl_data;
	
	size_t fat_offset = cluster + (cluster / 2);
	size_t fat_sector = d->reserved_sectors + (fat_offset / d->bytes_per_sector);
	size_t ent_offset = fat_offset % d->bytes_per_sector;
	
	if(d->cached_fat_sector != fat_sector)
	{
		floppy_read_sectors(d->index, fat_sector, d->fat, 1);
		d->cached_fat_sector = fat_sector;
	}
	
	uint16_t table_value = *(uint16_t*)&d->fat[ent_offset];

	table_value = (cluster & 0x0001) ? table_value >> 4 : table_value & 0x0FFF;
	
	return table_value;
}

size_t fat12_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_drive* fd)
{
	fat12_drive* f = (fat12_drive*)fd->impl_data;

	size_t num_clusters = byte_offset / f->cluster_size;

	while (num_clusters--)
	{
		cluster = fat12_get_next_cluster(cluster, fd);

		if (cluster >= FAT12_EOF) { break; }
	}

	return cluster;
}

void fat12_mount_disk(filesystem_drive *d)
{
	fat12_drive* f = (fat12_drive*)d->impl_data;
	
	fat12_read_bios_block(f);
	
	//allocate fat sector
	f->fat = (uint8_t*)malloc(f->bytes_per_sector);
	f->cached_fat_sector = 0;
	
	fat12_mount_directory(&d->root, f, d->index);
}

size_t fat12_read_clusters(uint8_t* dest, size_t cluster, size_t bufferSize, const filesystem_drive *d)
{
	if (cluster >= FAT12_EOF)
	{
		//puts("read attempted, but eof reached");
		return cluster;
	}

	fat12_drive* f = (fat12_drive*)d->impl_data;
	
	size_t num_clusters = bufferSize / f->cluster_size;

	while(num_clusters--)
	{
		floppy_read_sectors(f->index, fat12_cluster_to_lba(f, cluster), dest, f->sectors_per_cluster);

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

