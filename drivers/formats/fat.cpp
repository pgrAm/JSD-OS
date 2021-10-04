#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <string>

#include <kernel/fs_driver.h>

#include "fat.h"

#define FAT12_EOF 0xFF8
#define FAT16_EOF 0xFFF8
#define FAT32_EOF 0x0FFFFFF8
#define exFAT_EOF 0xFFFFFFF8

#define DEFAULT_SECTOR_SIZE 512

static int fat_mount_disk(filesystem_virtual_drive* d);
static size_t fat_read_clusters(uint8_t* dest, size_t cluster, size_t num_bytes, const filesystem_virtual_drive* d);
static size_t fat_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_virtual_drive* fd);
static void fat_read_dir(directory_handle* dest, const file_handle* location, const filesystem_virtual_drive* fd);

enum fat_type
{
	FAT_UNKNOWN = 0,
	FAT_12,
	FAT_16,
	FAT_32,
	exFAT
};

typedef enum fat_type fat_type;

struct fat_drive
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

	size_t eof_value;

	fat_type type;
};

typedef struct fat_drive fat_drive;

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
	uint32_t	hidden_sectors;
	uint32_t	total_sectors_big;
} __attribute__((packed)) bpb;

typedef struct
{
	uint32_t	table_size_32;
	uint16_t	extended_flags;
	uint16_t	fat_version;
	uint32_t	root_cluster;
	uint16_t	fat_info;
	uint16_t	backup_BS_sector;
	uint8_t 	reserved_0[12];
	uint8_t		drive_number;
	uint8_t 	reserved_1;
	uint8_t		boot_signature;
	uint32_t 	volume_id;
	uint8_t		volume_label[11];
	uint8_t		fat_type_label[8];

}__attribute__((packed)) fat32_ext_bpb;

typedef struct
{
	uint8_t		bios_drive_num;
	uint8_t		reserved1;
	uint8_t		boot_signature;
	uint32_t	volume_id;
	uint8_t		volume_label[11];
	uint8_t		fat_type_label[8];

}__attribute__((packed)) fat16_ext_bpb;

filesystem_driver fat_driver = {
	fat_mount_disk,
	fat_get_relative_cluster,
	fat_read_clusters,
	fat_read_dir
};

extern "C" void fat_init()
{
	filesystem_add_driver(&fat_driver);
}

static inline size_t fat_cluster_to_lba(const fat_drive* d, size_t cluster)
{
	return (cluster - 2) * d->sectors_per_cluster + d->datasector;
}

static int fat_read_bios_block(uint8_t* buffer, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	bpb* bios_block = (bpb*)buffer;

	if(bios_block->boot_code[0] != 0xEB || bios_block->boot_code[2] != 0x90)
	{
		return UNKNOWN_FILESYSTEM;
	}

	if(bios_block->bytes_per_sector < fd->disk->minimum_block_size)
	{
		return DRIVE_NOT_SUPPORTED;
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
	d->blocks_per_sector	= d->bytes_per_sector / fd->disk->minimum_block_size;

	size_t total_sectors = (bios_block->total_sectors == 0) ?
		bios_block->total_sectors_big : bios_block->total_sectors;

	size_t data_sectors = total_sectors - d->datasector;
	size_t total_clusters = data_sectors / bios_block->sectors_per_cluster;

	if(d->bytes_per_sector == 0)
	{
		d->type = exFAT;
		d->eof_value = exFAT_EOF;
	}
	else if(total_clusters < 0xFF5)
	{
		d->type = FAT_12;
		d->eof_value = FAT12_EOF;
	}
	else if(total_clusters < 0xFFF5)
	{
		d->type = FAT_16;
		d->eof_value = FAT16_EOF;
	}
	else
	{
		d->type = FAT_32;
		d->eof_value = FAT32_EOF;
	}

	if(d->type == exFAT || d->type == FAT_32)
	{
		fat32_ext_bpb* ext_bpb = (fat32_ext_bpb*)(buffer + sizeof(bpb));
		d->root_location = ext_bpb->root_cluster;
	}

	return MOUNT_SUCCESS;
}

static time_t fat_time_to_time_t(uint16_t date, uint16_t time)
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

static bool fat_read_dir_entry(file_handle& dest, const fat_directory_entry* entry, size_t disk_num)
{
	if(entry->name[0] == 0) {
		return false;
	} //end of directory

	dest.name.assign(entry->name, 8);
	if(auto pos = dest.name.find(' '); pos != std::string::npos)
	{
		dest.name.resize(pos);
	}

	dest.type.assign(entry->extension, 3);
	if(auto pos = dest.type.find(' '); pos != std::string::npos)
	{
		dest.type.resize(pos);
	}

	dest.full_name = dest.name;
	if(!dest.type.empty())
	{
		dest.full_name += '.';
		dest.full_name += dest.type;
	}

	dest.size = entry->file_size;

	dest.time_created = fat_time_to_time_t(entry->created_date, entry->created_time);
	dest.time_modified = fat_time_to_time_t(entry->modified_date, entry->modified_time);

	dest.disk = disk_num;
	dest.location_on_disk = entry->first_cluster;

	uint32_t flags = 0;
	if(entry->attributes & DIRECTORY)
	{
		flags |= IS_DIR;
	}

	dest.flags = flags;

	return true;
}

static void fat_read_root_dir(directory_handle* dest, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	if(d->type == exFAT || d->type == FAT_32)
	{
		file_handle f = {{}, {}, {}, IS_DIR, fd->index, d->root_location, 0, 0, 0};
		fat_read_dir(dest, &f, fd);
		return;
	}

	size_t max_num_files = d->root_entries;

	dest->name = "";
	dest->file_list = new file_handle[max_num_files];
	dest->drive = fd->index;

	size_t buffer_size = d->root_size * d->bytes_per_sector;

	fat_directory_entry* root_directory = 
		(fat_directory_entry*)filesystem_allocate_buffer(fd->disk, buffer_size);

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

		if(!fat_read_dir_entry(dest->file_list[num_files], 
								 &root_directory[i], fd->index)) 
		{
			break;
		} //end of directory

		num_files++;
	}

	dest->num_files = num_files;

	filesystem_free_buffer(fd->disk, (uint8_t*)root_directory, buffer_size);
}

static void fat_read_dir(directory_handle* dest, const file_handle* dir, const filesystem_virtual_drive* fd)
{
	file_stream* dir_stream = filesystem_create_stream(dir);

	size_t max_num_files = 100;// dir->size / sizeof(fat_directory_entry);

	//printf("\ndir %u\n", dir->size);

	dest->name = "";// strdup(dir->full_name);
	dest->file_list = new file_handle[max_num_files];
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

		if (!fat_read_dir_entry(dest->file_list[num_files], &entry, fd->index))
		{
			break; 
		}

		num_files++;
	}

	dest->num_files = num_files;

	filesystem_close_file(dir_stream);
}

static void read_from_fat(size_t fat_sector, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	if(d->cached_fat_sector != fat_sector)
	{
		filesystem_read_blocks_from_disk(fd, fat_sector, d->fat, d->blocks_per_sector);

		d->cached_fat_sector = fat_sector;
	}
}

static size_t fat_get_next_cluster_32(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t fat_offset = cluster * 2;
	size_t fat_sector = d->reserved_sectors + (fat_offset / d->bytes_per_sector);
	size_t ent_offset = fat_offset % d->bytes_per_sector;

	read_from_fat(fat_sector, fd);

	uint32_t table_value = *(uint32_t*)&d->fat[ent_offset];

	if(d->type == exFAT)
		return table_value;

	return table_value & 0x0FFFFFFF;
}

static size_t fat_get_next_cluster_16(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t fat_offset = cluster * 2;
	size_t fat_sector = d->reserved_sectors + (fat_offset / d->bytes_per_sector);
	size_t ent_offset = fat_offset % d->bytes_per_sector;

	read_from_fat(fat_sector, fd);

	return *(uint16_t*)&d->fat[ent_offset];
}

static size_t fat_get_next_cluster_12(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t fat_offset = cluster + (cluster / 2);
	size_t fat_sector = d->reserved_sectors + (fat_offset / d->bytes_per_sector);
	size_t ent_offset = fat_offset % d->bytes_per_sector;

	read_from_fat(fat_sector, fd);

	uint16_t table_value = *(uint16_t*)&d->fat[ent_offset];

	table_value = (cluster & 0x0001) ? table_value >> 4 : table_value & 0x0FFF;

	return table_value;
}

static size_t fat_get_next_cluster(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	switch(d->type)
	{
	case FAT_12:
		return fat_get_next_cluster_12(cluster, fd);
	case FAT_16:
		return fat_get_next_cluster_16(cluster, fd);
	case exFAT:
	case FAT_32:
		return fat_get_next_cluster_32(cluster, fd);
	}
	return d->eof_value;
}

static size_t fat_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_virtual_drive* fd)
{
	fat_drive* f = (fat_drive*)fd->fs_impl_data;

	size_t num_clusters = byte_offset / f->cluster_size;

	while (num_clusters--)
	{
		cluster = fat_get_next_cluster(cluster, fd);

		if (cluster >= f->eof_value) { break; }
	}

	return cluster;
}

static int fat_mount_disk(filesystem_virtual_drive* d)
{
	fat_drive* f = new fat_drive{};
	d->fs_impl_data = f;

	size_t num_sectors = (DEFAULT_SECTOR_SIZE + (d->disk->minimum_block_size - 1))
		/ d->disk->minimum_block_size;

	size_t buffer_size = num_sectors * d->disk->minimum_block_size;
	uint8_t* boot_sector = filesystem_allocate_buffer(d->disk, buffer_size);

	filesystem_read_blocks_from_disk(d, 0, boot_sector, num_sectors);

	int err = fat_read_bios_block(boot_sector, d);
	if(err != MOUNT_SUCCESS)
	{
		filesystem_free_buffer(d->disk, boot_sector, buffer_size);
		delete f;
		d->fs_impl_data = nullptr;
		return err;
	}

	if(d->chunk_read_size < f->cluster_size)
	{
		d->chunk_read_size = f->cluster_size;
	}

	//allocate fat sector
	if(f->bytes_per_sector > DEFAULT_SECTOR_SIZE)
	{
		filesystem_free_buffer(d->disk, boot_sector, DEFAULT_SECTOR_SIZE);
		f->fat = filesystem_allocate_buffer(d->disk, f->bytes_per_sector);
	}
	else
	{
		f->fat = boot_sector;
	}

	f->cached_fat_sector = 0;
	
	fat_read_root_dir(&d->root, d);

	return MOUNT_SUCCESS;
}

static size_t fat_read_clusters(uint8_t* dest, size_t cluster, size_t buffer_size, const filesystem_virtual_drive*d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if (cluster >= f->eof_value)
	{
		//puts("read attempted, but eof reached");
		return cluster;
	}

	size_t num_clusters = buffer_size / f->cluster_size;
	while(num_clusters--)
	{
		//printf("Reading from cluster %d to %X\n", cluster, dest);
		//printf("lba %d\n", fat_cluster_to_lba(f, cluster));
		filesystem_read_blocks_from_disk(d, fat_cluster_to_lba(f, cluster), dest, f->sectors_per_cluster * f->blocks_per_sector);

		//printf("value = %X\n", *((uint32_t*)dest));

		dest += f->cluster_size;
		
		cluster = fat_get_next_cluster(cluster, d);
		
		if(cluster >= f->eof_value)
		{
			//puts("eof reached");
			break;
		}			
	}
	
	return cluster;
}