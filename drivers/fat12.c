#include "../drivers/floppy.h"
#include "../drivers/fat12.h"
#include "../kernel/filesystem.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

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

void fat12_read_bios_block(fat12_drive* d)
{
	uint8_t boot_sector[512];
	
	floppy_read_sectors(d->index, 0, boot_sector, 1);
	
	bpb* bios_block = (bpb*)(boot_sector + 0x00B);
	
	d->root_size 			= (32 * bios_block->root_entries) / 512; //number of sectors in root dir
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

void fat12_mount_directory(directory* root, const fat12_drive* selected_drive, size_t logicalDriveNumber)
{
	uint8_t *root_directory = (uint8_t*)malloc(selected_drive->root_size * selected_drive->bytes_per_sector);
	
	//root_directory[0] = 0xfe;
	//
	//if(root_directory[0] == 0xfe)
	//{
	//	printf("sanity test successful\n");
	//}
	
	//uint8_t root_directory[512 * 14];
	
	//printf("mounting root directory at %X\n", (uint32_t)root_directory);
	
	root->name = "";
	//printf("m\n");
	root->file_list = (file_handle*)malloc(selected_drive->root_entries * sizeof(file_handle));
	//printf("m\n");
	root->drive = logicalDriveNumber;
	
	//printf("mounting root directory at %X\n", (uint32_t)root->file_list);
	//printf("root_size=%d\n", selected_drive->root_size);
	size_t num_files = 0;
	
	//printf("sizeof(fat_directory_entry) = %d\n", sizeof(fat_directory_entry));
	
	for(size_t sector = 0; sector < selected_drive->root_size; sector++)
	{
		floppy_read_sectors(selected_drive->index, selected_drive->root_location + sector, root_directory, 1);
		
		for(size_t entry_number = 0; entry_number < 16; entry_number++)
		{
			fat_directory_entry* entry = (fat_directory_entry*)(root_directory + entry_number * sizeof(fat_directory_entry));
			
			if(entry->name[0] != 0)
			{
				char* full_name = (char*)malloc(13*sizeof(char));
			
				root->file_list[num_files].name = (char*)malloc(9*sizeof(char));
				memcpy(root->file_list[num_files].name, entry->name, 8);
				root->file_list[num_files].name[8] = '\0';
				
				root->file_list[num_files].type = (char*)malloc(4*sizeof(char));
				memcpy(root->file_list[num_files].type, entry->extension, 3);
				root->file_list[num_files].type[3] = '\0';
				
				strcpy(full_name, strtok(root->file_list[num_files].name, " "));
				strcat(full_name, ".");
				strcat(full_name, strtok(root->file_list[num_files].type, " "));
				
				root->file_list[num_files].full_name = full_name;
				
				root->file_list[num_files].size = entry->file_size;
				
				root->file_list[num_files].time_created = fat12_time_to_time_t(entry->created_date, entry->created_time);				
				root->file_list[num_files].time_modified = fat12_time_to_time_t(entry->modified_date, entry->modified_time);
				
				root->file_list[num_files].disk = logicalDriveNumber;
				root->file_list[num_files].location_on_disk = entry->first_cluster;
				
				num_files++;
			}
		}
	}
	
	root->num_files = num_files;
	
	//printf("Found %d files\n", (uint32_t)num_files);
	
	free(root_directory);
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

size_t fat12_get_cluster(size_t current_cluster, size_t byte_offset, const filesystem_drive *d)
{
	fat12_drive* f = (fat12_drive*)d->impl_data;
	
	size_t clusters_to_traverse = byte_offset / (f->bytes_per_sector * f->sectors_per_cluster);
	
	while(clusters_to_traverse--)
	{
		current_cluster = fat12_get_next_cluster(current_cluster, d) ;
	}
	
	return current_cluster;
}

void fat12_mount_disk(filesystem_drive *d, size_t logicalDriveNumber)
{
	fat12_drive* f = (fat12_drive*)d->impl_data;
	
	fat12_read_bios_block(f);
	
	//allocate fat sector
	f->fat = (uint8_t*)malloc(f->bytes_per_sector);
	f->cached_fat_sector = 0;
	
	fat12_mount_directory(&d->root, f, logicalDriveNumber);
}

size_t fat12_cluster_to_lba(fat12_drive* d, size_t cluster)
{
	return (cluster - 2) * d->sectors_per_cluster + d->datasector;
}

size_t fat12_read_clusters(uint8_t* dest, size_t bufferSize, size_t cluster, const filesystem_drive *d)
{
	fat12_drive* f = (fat12_drive*)d->impl_data;
	
	size_t num_sectors = bufferSize / (f->bytes_per_sector * f->sectors_per_cluster);
	
	while(num_sectors--)
	{
		floppy_read_sectors(f->index, fat12_cluster_to_lba(f, cluster), dest, f->sectors_per_cluster);
		
		cluster = fat12_get_next_cluster(cluster, d);
		
		if(cluster >= 0xFF8)
		{
			break;
		}			
	}
	
	return cluster;
}

