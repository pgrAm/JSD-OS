#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <bit>
#include <string>
#include <vector>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/util.h>
#include <kernel/kassert.h>

#define FAT12_EOF 0xFF8
#define FAT16_EOF 0xFFF8
#define FAT32_EOF 0x0FFFFFF8
#define exFAT_EOF 0xFFFFFFF8

#define DEFAULT_SECTOR_SIZE 512

#define FAT_ROOT_DIR_FLAG 0x80000000

static int fat_mount_disk(filesystem_virtual_drive* d);
static size_t fat_read(uint8_t* dest, size_t cluster, size_t offset, size_t num_bytes, const filesystem_virtual_drive* d);
static size_t fat_write(const uint8_t* dest, size_t cluster, size_t offset, size_t num_bytes, const filesystem_virtual_drive* d);
static size_t fat_get_relative_cluster(size_t cluster, size_t byte_offset, const filesystem_virtual_drive* fd);
static void fat_read_dir(directory_stream* dest, const file_data_block* location, const filesystem_virtual_drive* fd);
static size_t fat_allocate_clusters(size_t start_cluster, size_t size_in_bytes, const filesystem_virtual_drive* d);

enum fat_type
{
	FAT_UNKNOWN = 0,
	FAT_12,
	FAT_16,
	FAT_32,
	exFAT
};

template<fat_type T> 
static constexpr size_t fat_entry_mask = 0;

template<> 
static constexpr size_t fat_entry_mask<FAT_12>	= 0xFFF;

template<> 
static constexpr size_t fat_entry_mask<FAT_16> = 0xFFFF;

template<> 
static constexpr size_t fat_entry_mask<FAT_32> = 0xFFFFFFF;

template<> 
static constexpr size_t fat_entry_mask<exFAT> = 0xFFFFFFFF;

template<fat_type T> struct fat_entry_type { typedef void type; };

template<> struct fat_entry_type<FAT_12> { typedef uint16_t type; };
template<> struct fat_entry_type<FAT_16> { typedef uint16_t type; };
template<> struct fat_entry_type<FAT_32> { typedef uint32_t type; };
template<> struct fat_entry_type<exFAT> { typedef uint32_t type; };

template<fat_type T>
using fat_entry_type_t = typename fat_entry_type<T>::type;

struct cluster_span
{
	size_t first_cluster;
	size_t num_clusters;
};

struct fat_drive
{
	size_t root_size;
	size_t fats_size;
	size_t root_location;
	size_t datasector;
	size_t cluster_size;
	size_t cluster_size_log2;
	size_t sectors_per_cluster;
	size_t sectors_per_cluster_log2;
	size_t bytes_per_sector;
	size_t root_entries;
	size_t reserved_sectors;
	size_t num_clusters;

	size_t blocks_per_sector;
	size_t blocks_per_sector_log2;
	size_t fat_block;

	size_t cached_fat_sector; //the sector index of the cached fat data

	bool fat_dirty = false;

	size_t eof_value;

	fat_type type;

	std::vector<cluster_span> free_clusters;
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

static const filesystem_driver fat_driver = {
	fat_mount_disk,
	//fat_get_relative_cluster,
	fat_read,
	fat_write,
	fat_allocate_clusters,
	fat_read_dir
};

extern "C" void fat_init()
{
	filesystem_add_driver(&fat_driver);
}

static inline size_t fat_cluster_to_block(const fat_drive* d, size_t cluster)
{
	return (((cluster - 2) << d->sectors_per_cluster_log2) + d->datasector) << d->blocks_per_sector_log2;
}

static int fat_read_bios_block(const uint8_t* buffer, fat_drive* d, size_t block_size)
{
	bpb* bios_block = (bpb*)buffer;

	if(bios_block->boot_code[0] != 0xEB || bios_block->boot_code[2] != 0x90)
	{
		return UNKNOWN_FILESYSTEM;
	}

	if(!std::has_single_bit(bios_block->bytes_per_sector))
	{
		return MOUNT_FAILURE;
	}

	if(bios_block->bytes_per_sector < block_size)
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
	d->blocks_per_sector	= d->bytes_per_sector / block_size;

	d->blocks_per_sector_log2 = std::countr_zero(d->blocks_per_sector);
	d->sectors_per_cluster_log2 = std::countr_zero(d->sectors_per_cluster);
	d->cluster_size_log2 = std::countr_zero(d->cluster_size);

	d->fat_block = d->reserved_sectors * d->blocks_per_sector;

	size_t total_sectors = (bios_block->total_sectors == 0) ?
		bios_block->total_sectors_big : bios_block->total_sectors;

	size_t data_sectors = total_sectors - d->datasector;
	size_t total_clusters = data_sectors / bios_block->sectors_per_cluster;

	d->num_clusters = total_clusters;

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

static std::string_view read_field(const char* field, size_t size)
{
	const std::string_view f(field, size);
	if(auto pos = f.find(' '); pos != std::string_view::npos)
	{
		return f.substr(0, pos);
	}
	return f;
}

static uint32_t fat_convert_flags(uint8_t fat_flags)
{
	uint32_t flags = 0;
	if(fat_flags & DIRECTORY)
	{
		flags |= file_flags::IS_DIR;
	}

	if(fat_flags & READ_ONLY)
	{
		flags |= file_flags::IS_READONLY;
	}

	return flags;
}

static bool fat_read_dir_entry(file_handle& dest, const fat_directory_entry& entry, size_t disk_id)
{
	if(entry.name[0] == 0) {
		return false;
	} //end of directory

	dest.name = read_field(entry.name, 8);
	if(auto ext = read_field(entry.extension, 3); !ext.empty())
	{
		dest.name += '.';
		dest.name += ext;
	}

	dest.time_created = fat_time_to_time_t(entry.created_date, entry.created_time);
	dest.time_modified = fat_time_to_time_t(entry.modified_date, entry.modified_time);

	dest.data = {
		.location_on_disk = entry.first_cluster,
		.disk_id = disk_id,
		.size = entry.file_size,
		.flags = fat_convert_flags(entry.attributes)
	};

	return true;
}

static void fat_read_root_dir(directory_stream* dest, const filesystem_virtual_drive* fd)
{
	const fat_drive* d = (fat_drive*)fd->fs_impl_data;

	const size_t max_num_files = d->root_entries;

	std::vector<file_handle> files;
	for(size_t i = 0; i < max_num_files; i++)
	{
		fat_directory_entry entry;
		filesystem_read(fd, d->root_location, i * sizeof(fat_directory_entry),
						(uint8_t*)&entry, sizeof(fat_directory_entry));

		if(entry.attributes & LFN)
		{
			continue;
		}

		file_handle out{};
		if(!fat_read_dir_entry(out, entry, fd->id))
		{
			break;
		} //end of directory

		dest->file_list.push_back(out);
	}
}

static void fat_read_dir(directory_stream* dest, const file_data_block* dir, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;
	if((d->type == FAT_12 || d->type == FAT_16) && (dir->flags & FAT_ROOT_DIR_FLAG))
	{
		fat_read_root_dir(dest, fd);
		return;
	}

	file_stream* dir_stream = filesystem_create_stream(dir);
	k_assert(dir_stream);

	while(true)
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

		file_handle out;
		if(!fat_read_dir_entry(out, entry, fd->id))
		{
			break;
		}

		dest->file_list.push_back(out);
	}

	filesystem_close_file(dir_stream);
}

template<fat_type T>
static size_t fat_get_next_cluster(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t offset = cluster * sizeof(fat_entry_type_t<T>);

	fat_entry_type_t<T> value;
	filesystem_read(fd, d->fat_block, offset, (uint8_t*)&value,
					sizeof(fat_entry_type_t<T>));
	return value & fat_entry_mask<T>;
}

template<>
size_t fat_get_next_cluster<FAT_12>(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t offset = cluster + (cluster / 2);

	uint16_t value;
	filesystem_read(fd, d->fat_block, offset, (uint8_t*)&value, sizeof(uint16_t));

	return (cluster & 1) ? value >> 4 : value & fat_entry_mask<FAT_12>;
}

template<fat_type T>
static void fat_set_next_cluster(size_t cluster, size_t next, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t offset = cluster * sizeof(fat_entry_type_t<T>);

	fat_entry_type_t<T> value = next & fat_entry_mask<T>;
	filesystem_write(fd, d->fat_block,
					 offset, (uint8_t*)&value,
					 sizeof(fat_entry_type_t<T>));
}

template<>
void fat_set_next_cluster<FAT_12>(size_t cluster, size_t next, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	size_t offset = cluster + (cluster / 2);
	size_t fat_sector = d->reserved_sectors;

	uint16_t value;
	filesystem_read(fd, fat_sector, offset, (uint8_t*)&value, sizeof(uint16_t));

	if(cluster & 0x01)
	{
		value = (value & 0x000F) | (next << 4);
	}
	else
	{
		value = (value & 0xF000) | (next & 0x0FFF);
	}

	filesystem_write(fd, fat_sector, offset, (uint8_t*)&value, sizeof(uint16_t));
}

static size_t fat_get_next_cluster(size_t cluster, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	switch(d->type)
	{
	case FAT_12:
		return fat_get_next_cluster<FAT_12>(cluster, fd);
	case FAT_16:
		return fat_get_next_cluster<FAT_16>(cluster, fd);
	case FAT_32:
		return fat_get_next_cluster<FAT_32>(cluster, fd);
	case exFAT:
		return fat_get_next_cluster<exFAT>(cluster, fd);
	case FAT_UNKNOWN:
		k_assert(false);
	}
	return d->eof_value;
}

static size_t fat_get_relative_cluster(size_t cluster, size_t num_clusters, const filesystem_virtual_drive* fd)
{
	fat_drive* f = (fat_drive*)fd->fs_impl_data;

	while(num_clusters--)
	{
		cluster = fat_get_next_cluster(cluster, fd);

		if(cluster >= f->eof_value) { break; }
	}

	return cluster;
}

static int fat_mount_disk(filesystem_virtual_drive* d)
{
	uint8_t* boot_sector = new uint8_t[DEFAULT_SECTOR_SIZE];

	filesystem_read(d, 0, 0, boot_sector, DEFAULT_SECTOR_SIZE);

	fat_drive* f = new fat_drive{};

	int err = fat_read_bios_block(boot_sector, f, d->block_size);
	delete[] boot_sector;

	if(err != MOUNT_SUCCESS)
	{
		delete f;
		return err;
	}

	d->fs_impl_data = f;

	f->cached_fat_sector = 0;

	d->root_dir = {
		.name = {},
		.data = {f->root_location, d->id, 0, IS_DIR | FAT_ROOT_DIR_FLAG},
		.time_created = 0,
		.time_modified = 0,
	};

	return MOUNT_SUCCESS;
}

static size_t fat_write(const uint8_t* buf, size_t start_cluster, size_t offset, size_t size, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if(start_cluster >= f->eof_value)
	{
		return start_cluster;
	}

	auto chunks = filesystem_chunkify(offset, size, f->cluster_size-1, f->cluster_size_log2);
	auto cluster = fat_get_relative_cluster(start_cluster, chunks.start_chunk, d);

	if(chunks.start_size != 0)
	{
		auto lba = fat_cluster_to_block(f, cluster);
		filesystem_write(d, lba, chunks.start_offset, buf, chunks.start_size);

		cluster = fat_get_next_cluster(cluster, d);
		buf += chunks.start_size;
	}

	if(chunks.num_full_chunks)
	{
		size_t num_clusters = chunks.num_full_chunks;
		while(num_clusters-- && cluster < f->eof_value)
		{
			auto lba = fat_cluster_to_block(f, cluster);
			filesystem_write(d, lba, 0, buf, f->cluster_size);

			cluster = fat_get_next_cluster(cluster, d);
			buf += f->cluster_size;
		}
	}

	if(chunks.end_size != 0 && cluster < f->eof_value)
	{
		auto lba = fat_cluster_to_block(f, cluster);
		filesystem_write(d, lba, 0, buf, chunks.end_size);
	}

	return cluster;
}

static size_t fat_read(uint8_t* buf, size_t start_cluster, size_t offset, size_t size, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if(start_cluster >= f->eof_value)
	{
		return start_cluster;
	}

	auto chunks = filesystem_chunkify(offset, size, f->cluster_size - 1, f->cluster_size_log2);
	auto cluster = fat_get_relative_cluster(start_cluster, chunks.start_chunk, d);

	if(chunks.start_size != 0)
	{
		auto lba = fat_cluster_to_block(f, cluster);
		filesystem_read(d, lba, chunks.start_offset, buf, chunks.start_size);

		cluster = fat_get_next_cluster(cluster, d);
		buf += chunks.start_size;
	}

	if(chunks.num_full_chunks)
	{
		size_t num_clusters = chunks.num_full_chunks;
		while(num_clusters-- && cluster < f->eof_value)
		{
			auto lba = fat_cluster_to_block(f, cluster);
			filesystem_read(d, lba, 0, buf, f->cluster_size);

			cluster = fat_get_next_cluster(cluster, d);
			buf += f->cluster_size;
		}
	}

	if(chunks.end_size != 0 && cluster < f->eof_value)
	{
		auto lba = fat_cluster_to_block(f, cluster);
		filesystem_read(d, lba, 0, buf, chunks.end_size);
	}

	return cluster;
}

template<fat_type T> static size_t do_write_to_fat(size_t previous, size_t first_cluster, size_t num_clusters, const filesystem_virtual_drive* fd)
{
	fat_set_next_cluster<T>(previous, first_cluster, fd);
	for(size_t i = 0; i < (num_clusters - 1); i++)
	{
		fat_set_next_cluster<T>(first_cluster + i, first_cluster + i + 1, fd);
	}

	return first_cluster + num_clusters - 1;
}

static size_t write_to_fat(size_t previous, size_t first_cluster, size_t num_clusters, const filesystem_virtual_drive* fd)
{
	fat_drive* d = (fat_drive*)fd->fs_impl_data;

	switch(d->type)
	{
	case FAT_12:
		return do_write_to_fat<FAT_12>(previous, first_cluster, num_clusters, fd);
	case FAT_16:
		return do_write_to_fat<FAT_16>(previous, first_cluster, num_clusters, fd);
	case FAT_32:
		return do_write_to_fat<FAT_32>(previous, first_cluster, num_clusters, fd);
	case exFAT:
		return do_write_to_fat<exFAT>(previous, first_cluster, num_clusters, fd);
	case FAT_UNKNOWN:
		k_assert(false);
	}

	k_assert(false);
	return 0;
}

static bool fat_cluster_free(size_t cluster, const filesystem_virtual_drive* f)
{
	return fat_get_next_cluster(cluster, f) == 0;
}

static size_t fat_allocate_clusters(size_t start_cluster, size_t size_in_bytes, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	const size_t needed_clusters = (size_in_bytes + (f->cluster_size - 1)) / f->cluster_size;
	size_t num_clusters = needed_clusters;
	size_t last_cluster = start_cluster;

	//skip any cluster which may already be allocated
	while(num_clusters)
	{
		size_t cluster = fat_get_next_cluster(last_cluster, d);
		if(cluster == f->eof_value)
			break;

		last_cluster = cluster;
		num_clusters--;
	}

	if(num_clusters == 0)
	{
		return size_in_bytes;
	}

	size_t free_cluster = f->free_clusters.size();
	while(free_cluster--)
	{
		auto span = f->free_clusters.back();
		f->free_clusters.pop_back();

		last_cluster = write_to_fat(last_cluster, span.first_cluster, span.num_clusters, d);
		num_clusters -= span.num_clusters;

		if(num_clusters == 0)
		{
			write_to_fat(last_cluster, f->eof_value, 1, d);
			return size_in_bytes;
		}
	}

	for(size_t cluster = 0; cluster < f->num_clusters; cluster++)
	{
		if(!fat_cluster_free(cluster, d))
			continue;

		cluster_span span{cluster, 1};

		while(span.num_clusters < num_clusters)
		{
			if(!fat_cluster_free(++cluster, d))
			{
				break;
			}
			span.num_clusters++;
		}

		last_cluster = write_to_fat(last_cluster, span.first_cluster, span.num_clusters, d);
		num_clusters -= span.num_clusters;

		if(num_clusters == 0)
		{
			write_to_fat(last_cluster, f->eof_value, 1, d);
			return size_in_bytes;
		}
	}

	write_to_fat(last_cluster, f->eof_value, 1, d);
	return (needed_clusters - num_clusters) * f->cluster_size;
}
