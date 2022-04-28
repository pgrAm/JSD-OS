#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <bit>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/util.h>
#include <kernel/kassert.h>
#include <kernel/util/unicode.h>

#define FAT12_EOF 0xFF8
#define FAT16_EOF 0xFFF8
#define FAT32_EOF 0x0FFFFFF8
#define exFAT_EOF 0xFFFFFFF8

#define FREE_CLUSTER 0x00

#define DEFAULT_SECTOR_SIZE 512

#define FAT_ROOT_DIR_FLAG 0x80000000

static size_t fat_allocate_clusters(size_t start_cluster, size_t size_in_bytes, const file_data_block* file, const filesystem_virtual_drive* d);
static void fat_update_file(const file_data_block* file, const filesystem_virtual_drive* fd);

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

struct __attribute__((packed)) fat_short_filename
{
	char root[8];
	char ext[3];
};

struct __attribute__((packed)) fat_directory_entry
{
	fat_short_filename name;
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
};

struct __attribute__((packed)) fat_lfn_entry
{
	constexpr static size_t num_chars = 13;

	uint8_t sequence;
	uint16_t chars0[5];
	uint8_t attributes;
	uint8_t type;
	uint8_t short_checksum;
	uint16_t chars1[6];
	uint16_t zero;
	uint16_t chars2[2];
};

fs_index fat_file_location(const fat_directory_entry& entry)
{
	return (fs_index)entry.first_cluster | ((fs_index)entry.first_cluster_hi << 16);
}

struct __attribute__((packed)) bpb
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
};

struct __attribute__((packed)) fat32_ext_bpb
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

};

struct __attribute__((packed)) fat16_ext_bpb
{
	uint8_t		bios_drive_num;
	uint8_t		reserved1;
	uint8_t		boot_signature;
	uint32_t	volume_id;
	uint8_t		volume_label[11];
	uint8_t		fat_type_label[8];
};

struct __attribute__((packed)) fat_format_data 
{
	fs_index parent_dir;
	uint32_t dir_flags;
};
static_assert(sizeof(file_data_block::format_data) == sizeof(fat_format_data));

static inline size_t fat_cluster_to_block(const fat_drive* d, size_t cluster)
{
	return (((cluster - 2) << d->sectors_per_cluster_log2) + d->datasector) << d->blocks_per_sector_log2;
}

static mount_status fat_read_bios_block(const uint8_t* buffer, fat_drive* d,
										size_t block_size)
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

	auto root_size 			= (sizeof(fat_directory_entry) * bios_block->root_entries) 
							/ bios_block->bytes_per_sector; //number of sectors in root dir

	d->fats_size 			= bios_block->number_of_FATs * bios_block->sectors_per_FAT;
	d->root_location 		= d->fats_size + bios_block->reserved_sectors;
	d->datasector 			= d->root_location + root_size;
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

struct fat_time { uint16_t date; uint16_t time; };

static time_t fat_time_to_time_t(fat_time t)
{
	tm file_time = {
		.tm_sec		= (t.time & 0x001F) << 1,
		.tm_min		= (t.time & 0x07E0) >> 5,
		.tm_hour	= (t.time & 0xF800) >> 11,

		.tm_mday	= (t.date & 0x001F),
		.tm_mon		= ((t.date & 0x01E0) >> 5) - 1,
		.tm_year	= ((t.date & 0xFE00) >> 9) + 80,

		.tm_wday	= 0,
		.tm_yday	= 0,
		.tm_isdst	= 0
	};

	return mktime(&file_time);
}

static fat_time time_t_time_to_fat_time(time_t time)
{
	auto ftime = gmtime(&time);

	return {
		.date = (uint16_t)((ftime->tm_mday & 0x001Fu)
						| (((ftime->tm_mon + 1) << 5) & 0x01E0)
						| (((ftime->tm_year + 80) << 9) & 0xFE00)),
		.time = (uint16_t)(((ftime->tm_sec >> 1) & 0x001F)
						| ((ftime->tm_sec << 5) & 0x07E0)
						| ((ftime->tm_hour << 11) & 0x07E0))
	};
}

static constexpr std::string_view read_field(const char* field, size_t size)
{
	const std::string_view f(field, size);
	if(auto pos = f.find(' '); pos != f.npos)
	{
		return f.substr(0, pos);
	}
	return f;
}

static void write_field(char* field, std::string_view src, size_t size)
{
	memcpy(field, src.data(), src.size());
	if(src.size() < size)
		memset(field + src.size(), ' ', size - src.size());
}

static constexpr uint32_t fat_convert_flags(uint8_t fat_flags)
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

static constexpr uint8_t to_fat_flags(uint32_t flags)
{
	uint8_t fat_flags = 0;
	if(flags & file_flags::IS_DIR)
	{
		fat_flags |= DIRECTORY;
	}

	if(flags & file_flags::IS_READONLY)
	{
		fat_flags |= READ_ONLY;
	}

	return fat_flags;
}

static fat_directory_entry fat_create_dir_entry(const file_handle& src)
{
	using namespace std::literals;

	auto create = time_t_time_to_fat_time(src.time_created);
	auto modify = time_t_time_to_fat_time(src.time_modified);

	fat_directory_entry e{
		.attributes		  = to_fat_flags(src.data.flags),
		.reserved		  = 0,
		.created_time_ms  = 0,
		.created_time	  = create.time,
		.created_date	  = create.date,
		.accessed_date	  = 0,
		.first_cluster_hi = (uint16_t)(src.data.location_on_disk & 0xFFFF0000),
		.modified_time	  = modify.time,
		.modified_date	  = modify.date,
		.first_cluster	  = (uint16_t)(src.data.location_on_disk & 0xFFFF),
		.file_size		  = src.data.size,
	};

	if(src.name == "."sv)
	{
		write_field(&e.name.root[0], "\x2e", 8);
		write_field(&e.name.ext[0], "", 3);
	}
	else if(src.name == ".."sv)
	{
		write_field(&e.name.root[0], "\x2e\x2e", 8);
		write_field(&e.name.ext[0], "", 3);
	}
	else
	{
		auto dot  = src.name.find_last_of('.');
		auto name = dot == src.name.npos
						? src.name
						: std::string_view{src.name}.substr(0, dot);

		auto ext = dot == src.name.npos
					   ? ""
					   : std::string_view{src.name}.substr(dot + 1);

		write_field(&e.name.root[0], name, 8);
		write_field(&e.name.ext[0], ext, 3);
	}

	return e;
}

static bool fat_read_dir_entry(file_handle& dest, const fat_directory_entry& entry, size_t disk_id)
{
	if(entry.name.root[0] == 0) {
		return false;
	} //end of directory

	if(entry.name.root[0] == '\x2e') 
	{
		dest.name += '.';
		if(entry.name.root[1] == '\x2e')
			dest.name += '.';
	}
	else
	{
		dest.name = read_field(entry.name.root, 8);
		if(auto ext = read_field(entry.name.ext, 3); !ext.empty())
		{
			dest.name += '.';
			dest.name += ext;
		}
	}

	dest.time_created = fat_time_to_time_t({entry.created_date, 
											entry.created_time});

	dest.time_modified = fat_time_to_time_t({entry.modified_date, 
											 entry.modified_time});

	dest.data = {
		.location_on_disk = fat_file_location(entry),
		.disk_id = disk_id,
		.size = entry.file_size,
		.flags = fat_convert_flags(entry.attributes)
	};

	if((dest.data.flags & IS_DIR) && 
	   dest.data.location_on_disk == 0)
	{
		dest.data.flags |= FAT_ROOT_DIR_FLAG;
	}

	return true;
}

static void fat_do_read_dir(std::vector<file_handle>& file_list,
							const file_data_block* dir,
							const filesystem_virtual_drive* fd)
{
	fs::stream dir_stream{filesystem_create_stream(dir)};
	k_assert(dir_stream);

	std::string lfn{};

	while(true)
	{
		fat_directory_entry entry;
		if(dir_stream.read(&entry, sizeof(fat_directory_entry))
		   != sizeof(fat_directory_entry))
		{
			break;
		}

		if(entry.name.root[0] == 0) { break; }

		if(entry.name.root[0] == '\xe5') 
		{
			lfn.clear();
			break;
		}

		if((entry.attributes & LFN) == LFN)
		{
			const auto e = std::bit_cast<fat_lfn_entry>(entry);

			if(e.sequence == 0xE5) continue;

			using sv_type = std::basic_string_view<uint16_t>;

			std::string lfn_buf = utf8_encode(utf16_decode(sv_type{&e.chars0[0], 5}));
			lfn_buf += utf8_encode(utf16_decode(sv_type{&e.chars1[0], 6}));
			lfn_buf += utf8_encode(utf16_decode(sv_type{&e.chars2[0], 2}));

			lfn = lfn_buf + lfn;
			continue;
		}

		file_handle out;
		if(!fat_read_dir_entry(out, entry, fd->id))
		{
			break;
		}

		fat_format_data fdata{dir->location_on_disk, dir->flags};
		memcpy(&out.data.format_data[0], &fdata, sizeof(fat_format_data));

		if(!lfn.empty())
		{
			if(auto nul = lfn.find('\0'); nul != lfn.npos)
			{
				lfn.resize(nul);
			}
			out.name = std::move(lfn);

			lfn.clear();
		}

		file_list.push_back(out);
	}
}

static void fat_read_dir(directory_stream* dest, const file_data_block* dir,
						 const filesystem_virtual_drive* fd)
{
	k_assert(dest);
	fat_do_read_dir(dest->file_list, dir, fd);
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

static mount_status fat_mount_disk(filesystem_virtual_drive* d)
{
	auto boot_sector = std::make_unique<uint8_t[]>(DEFAULT_SECTOR_SIZE);

	filesystem_read(d, 0, 0, boot_sector.get(), DEFAULT_SECTOR_SIZE);

	fat_drive* f = new fat_drive{};

	auto err = fat_read_bios_block(boot_sector.get(), f, d->block_size);

	if(err != MOUNT_SUCCESS)
	{
		delete f;
		return err;
	}

	d->fs_impl_data = f;

	f->cached_fat_sector = 0;

	auto loc = (f->type == FAT_12 || f->type == FAT_16) ? 0 : f->root_location;

	d->root_dir = {
		.name = {},
		.data = {loc, d->id, 0, IS_DIR | FAT_ROOT_DIR_FLAG},
		.time_created = 0,
		.time_modified = 0,
	};

	return MOUNT_SUCCESS;
}

static size_t fat_write(const uint8_t* buf, size_t start_cluster, size_t offset, size_t size, const file_data_block* file, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if((f->type == FAT_12 || f->type == FAT_16) && (file->flags & FAT_ROOT_DIR_FLAG))
	{
		filesystem_write(d, f->root_location + start_cluster, offset, buf, size);
		return offset & (f->bytes_per_sector - 1);
	}

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

static size_t fat_read(uint8_t* buf, size_t start_cluster, size_t offset, size_t size, const file_data_block* file, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if(start_cluster >= f->eof_value)
	{
		return start_cluster;
	}

	if((f->type == FAT_12 || f->type == FAT_16) && (file->flags & FAT_ROOT_DIR_FLAG))
	{
		filesystem_read(d, f->root_location + start_cluster, offset, buf, size);
		return offset & (f->bytes_per_sector - 1);
	}

	auto chunks = filesystem_chunkify(offset, size, f->cluster_size - 1, f->cluster_size_log2);
	auto cluster = fat_get_relative_cluster(start_cluster, chunks.start_chunk, d);
	if(cluster >= f->eof_value)
	{
		return cluster;
	}

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
	if(previous != 0)
	{
		fat_set_next_cluster<T>(previous, first_cluster, fd);
	}
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

static size_t write_to_fat(size_t cluster, size_t value,
						   const filesystem_virtual_drive* fd)
{
	return write_to_fat(cluster, value, 1, fd);
}

[[nodiscard]]
static bool fat_cluster_is_free(size_t cluster, const filesystem_virtual_drive* f)
{
	return fat_get_next_cluster(cluster, f) == FREE_CLUSTER;
}

static void fat_mark_cluster_free(size_t cluster,
								  const filesystem_virtual_drive* fd)
{
	fat_drive* f = (fat_drive*)fd->fs_impl_data;
	write_to_fat(cluster, FREE_CLUSTER, fd);
	f->free_clusters.emplace_back(cluster, 1u);
}

static size_t fat_claim_clusters(size_t last_cluster, size_t num_clusters, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;
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
			return last_cluster;
		}
	}

	for(size_t cluster = 0; cluster < f->num_clusters; cluster++)
	{
		if(!fat_cluster_is_free(cluster, d))
			continue;

		cluster_span span{cluster, 1};

		while(span.num_clusters < num_clusters)
		{
			if(!fat_cluster_is_free(++cluster, d))
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
			return last_cluster;
		}
	}

	write_to_fat(last_cluster, f->eof_value, 1, d);
	return 0;
}

static size_t fat_allocate_clusters(size_t start_cluster, size_t size_in_bytes, const file_data_block* file, const filesystem_virtual_drive* d)
{
	fat_drive* f = (fat_drive*)d->fs_impl_data;

	if((f->type == FAT_12 || f->type == FAT_16) && (file->flags & FAT_ROOT_DIR_FLAG))
	{
		return f->root_entries * sizeof(fat_directory_entry);
	}

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

	if(auto n = fat_claim_clusters(last_cluster, num_clusters, d); n == 0)
	{
		return (needed_clusters - n) * f->cluster_size;
	}

	return size_in_bytes;
}

static int fat_delete_file(const file_data_block* file,
						   const filesystem_virtual_drive* fd)
{
	auto fdata = std::bit_cast<fat_format_data>(file->format_data);

	file_data_block dir_block{fdata.parent_dir, fd->id, 0, fdata.dir_flags};

	fs::stream dir_stream{filesystem_create_stream(&dir_block)};
	k_assert(dir_stream);

	while(true)
	{
		fat_directory_entry entry;
		if(dir_stream.read(&entry, sizeof(fat_directory_entry)) !=
		   sizeof(fat_directory_entry))
		{
			break;
		}

		if(entry.name.root[0] == 0) break;

		if(entry.attributes & LFN) continue;

		if(fat_file_location(entry) == file->location_on_disk)
		{
			dir_stream.seek(dir_stream.get_pos() - sizeof(fat_directory_entry));

			entry.name.root[0] = 0xE5; //set the file as deleted

			dir_stream.write((uint8_t*)&entry, sizeof(fat_directory_entry));
			break;
		}
	}

	fat_drive* f = (fat_drive*)fd->fs_impl_data;

	//make the file's clusters available
	size_t cluster		= file->location_on_disk;
	size_t num_clusters = f->num_clusters; //upper bound on num clusters
	while(num_clusters)
	{
		size_t next_cluster = fat_get_next_cluster(cluster, fd);
		if(next_cluster == f->eof_value) break;

		fat_mark_cluster_free(cluster, fd);
		cluster = next_cluster;
		num_clusters--;
	}

	return 0;
}


//updates the directory entry after modifying a file
static void fat_update_file(const file_data_block* file, const filesystem_virtual_drive* fd)
{
	auto fdata = std::bit_cast<fat_format_data>(file->format_data);

	file_data_block dir_block{fdata.parent_dir, fd->id, 0, fdata.dir_flags};

	fs::stream dir_stream{filesystem_create_stream(&dir_block)};
	k_assert(dir_stream);

	while(true)
	{
		fat_directory_entry entry;
		if(dir_stream.read(&entry, sizeof(fat_directory_entry))
		   != sizeof(fat_directory_entry))
		{
			break;
		}

		if(entry.name.root[0] == 0)
			break;

		if(entry.attributes & LFN)
			continue;

		if(fat_file_location(entry) == file->location_on_disk)
		{
			dir_stream.seek(dir_stream.get_pos() - sizeof(fat_directory_entry));

			auto mod = time_t_time_to_fat_time(time(nullptr));

			entry.file_size = file->size;
			entry.modified_date = mod.date;
			entry.modified_time = mod.time;

			dir_stream.write((uint8_t*)&entry, sizeof(fat_directory_entry));
			break;
		}
	}
}

uint8_t fat_lfn_checksum(std::array<char, 11>&& name)
{
	uint8_t sum = 0;

	for(auto c : name)
	{
		sum = ((sum & 1) << 7) + (sum >> 1) + c;
	}

	return sum;
}

static void fat_write_lfn_entries(fat_directory_entry entry,
								  std::string_view long_name,
								  fs::stream_ref dir_stream)
{
	auto checksum =
		fat_lfn_checksum(std::bit_cast<std::array<char, 11>>(entry.name));

	auto lfn = utf16_encode<uint16_t>(utf8_decode(long_name)) + '\0';
	auto num_entries = (lfn.size() + (fat_lfn_entry::num_chars - 1)) /
					   fat_lfn_entry::num_chars;

	size_t num_chars_needed = num_entries * fat_lfn_entry::num_chars;

	while(lfn.size() < num_chars_needed)
	{
		lfn.push_back(0xFFFF);
	}
		
	auto lfn_it = lfn.end() - fat_lfn_entry::num_chars;
	uint8_t sequence_number = 0x40 | (num_entries + 1);

	for(size_t i = 0; i < num_entries; i++)
	{
		fat_lfn_entry e{
			.sequence		= sequence_number,
			.attributes		= LFN,
			.type			= 0,
			.short_checksum = checksum,
			.zero			= 0,
		};

		sequence_number = num_entries - i;

		memcpy(&e.chars0[0], lfn_it + 0, 5 * sizeof(uint16_t));
		memcpy(&e.chars1[0], lfn_it + 5, 6 * sizeof(uint16_t));
		memcpy(&e.chars2[0], lfn_it + 11, 2 * sizeof(uint16_t));

		lfn_it -= fat_lfn_entry::num_chars;

		dir_stream.write((uint8_t*)&e, sizeof(fat_lfn_entry));
	}
}

static void fat_create_file(const char* name, size_t name_len, uint32_t flags, directory_stream* dir, const filesystem_virtual_drive* fd)
{
	fs::stream dir_stream{filesystem_create_stream(&dir->data)};
	k_assert(dir_stream);

	while(true)
	{
		fat_directory_entry entry;
		if(dir_stream.read(&entry, sizeof(fat_directory_entry))
		   != sizeof(fat_directory_entry))
		{
			break;
		}

		if(entry.name.root[0] == 0)
		{
			dir_stream.seek(dir_stream.get_pos() - sizeof(fat_directory_entry));
			
			auto ts = time(nullptr);

			file_handle new_file{
				{name, name_len},
				{
					.location_on_disk = fat_claim_clusters(0, 1, fd),
					.disk_id = fd->id,
					.size = 0,
					.flags = flags,
				},
				ts, ts
			};

			fat_format_data fdata{dir->data.location_on_disk, dir->data.flags};
			memcpy(&new_file.data.format_data[0], &fdata, sizeof(fat_format_data));

			auto new_file_entry = fat_create_dir_entry(new_file);

			fat_write_lfn_entries(new_file_entry, new_file.name, dir_stream);

			dir_stream.write((uint8_t*)&new_file_entry, sizeof(fat_directory_entry));
			dir_stream.write(0);

			dir->file_list.push_back(new_file);

			if(flags & IS_DIR)
			{
				fs::stream f{filesystem_create_stream(&new_file.data)};
				k_assert(f);
			
				file_handle up_dir{"..", dir->data, ts, ts};
			
				auto up = fat_create_dir_entry(up_dir);
				f.write((uint8_t*)&up, sizeof(fat_directory_entry));
				f.write(0);
			}
			break;
		}
	}
}

static const filesystem_driver fat_driver = {
	fat_mount_disk,
	fat_read,
	fat_write,
	fat_allocate_clusters,
	fat_read_dir,
	fat_update_file,
	fat_create_file,
	fat_delete_file
};

extern "C" void fat_init()
{
	filesystem_add_driver(&fat_driver);
}