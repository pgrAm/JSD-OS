#include <kernel/filesystem/fs_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <bit>
#include <string_view>
#include <memory>

struct iso9660_drive
{
	size_t sector_size; //almost always 2KiB
	size_t sector_size_log2; //almost always 11

	size_t blocks_per_sector;
	size_t blocks_per_sector_log2;
};

struct flags {
	enum {
		HIDDEN = 0x01,
		DIRECTORY = 0x02,
		ASSOCIATED = 0x04,
		EXTENDED = 0x08,
		PERMISSIONS = 0x10,
		CONTINUES = 0x80,
	};
};

template<typename T> struct __attribute__((packed)) both_endian {
	T lsb;
	T msb;

	T get() const
	{
#ifdef MACHINE_IS_BIG_ENDIAN
		return msb;
#else
		return lsb;
#endif
	}
};

struct __attribute__((packed)) iso9660_dir_time
{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	int8_t timezone;
};

struct __attribute__((packed)) iso9660_directory_entry
{
	uint8_t length;
	uint8_t ext_length;

	both_endian<uint32_t> extent_start;
	both_endian<uint32_t> extent_length;

	iso9660_dir_time record_date;

	uint8_t flags;
	uint8_t interleave_units;
	uint8_t interleave_gap;

	both_endian<uint16_t> volume_seq;

	uint8_t name_len;
	char name[1];
};

struct __attribute__((packed)) iso9660_time
{
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char hundredths[2];
	int8_t timezone;
};

struct __attribute__((packed)) iso9660_volume_descriptor 
{
	uint8_t type;	// 0x01
	char id[5];		// CD001

	uint8_t version;
	uint8_t _unused0;

	char system_id[32];
	char volume_id[32];

	uint8_t _unused1[8];

	both_endian<uint32_t> volume_space;

	uint8_t _unused2[32];

	both_endian<uint16_t> volume_set;
	both_endian<uint16_t> volume_seq;
	both_endian<uint16_t> logical_block_size;
	both_endian<uint32_t> path_table_size;

	uint32_t path_table_LSB;
	uint32_t optional_path_table_LSB;
	uint32_t path_table_MSB;
	uint32_t optional_path_table_MSB;

	iso9660_directory_entry root;

	char volume_set_id[128];
	char volume_publisher[128];
	char data_preparer[128];
	char application_id[128];

	char copyright_file[38];
	char abstract_file[36];
	char bibliographic_file[37];

	iso9660_time creation;
	iso9660_time modification;
	iso9660_time expiration;
	iso9660_time effective;

	uint8_t file_structure_version;
	uint8_t _unused_3;

	char application_use[];
};

/*static time_t iso9660_time_to_time_t(const iso9660_time& t)
{
	struct tm file_time = {
		.tm_sec = atoi(std::string(t.second, 2).c_str()),
		.tm_min = atoi(std::string(t.minute, 2).c_str()),
		.tm_hour = atoi(std::string(t.hour, 2).c_str()),

		.tm_mday = atoi(std::string(t.day, 2).c_str()),
		.tm_mon = atoi(std::string(t.month, 2).c_str()) - 1,
		.tm_year = atoi(std::string(t.year, 4).c_str()),

		.tm_wday = 0,
		.tm_yday = 0,
		.tm_isdst = 0
	};

	return mktime(&file_time);
}*/

static time_t iso9660_time_to_time_t(const iso9660_dir_time& t)
{
	struct tm file_time = {
		.tm_sec = t.second,
		.tm_min = t.minute,
		.tm_hour = t.hour,

		.tm_mday = t.day,
		.tm_mon = t.month - 1,
		.tm_year = t.year,

		.tm_wday = 0,
		.tm_yday = 0,
		.tm_isdst = 0
	};

	return mktime(&file_time);
}

static std::string_view iso9660_read_entry_name(const char* data,
												size_t name_len)
{
	using namespace std::literals;

	std::string_view full_name = {data, name_len};

	if(auto pos = full_name.find(';'); pos != std::string_view::npos)
	{
		full_name = full_name.substr(0, pos);
	}

	if(full_name.empty() || full_name[0] == '\0')
	{
		return "."sv;
	}
	else if(full_name == "\1")
	{
		return ".."sv;
	}
	else
	{
		return full_name;
	}
}

static file_handle
iso9660_read_dir_entry(intrusive_ptr<std::string> parent_dir,
					   const uint8_t* entry_ptr, size_t disk_id)
{
	auto* entry = (const iso9660_directory_entry*)entry_ptr;

	auto entry_name =
		iso9660_read_entry_name((const char*)entry_ptr +
									offsetof(iso9660_directory_entry, name),
								size_t{entry->name_len});

	uint32_t flags = 0;
	if(entry->flags & flags::DIRECTORY)
	{
		flags |= IS_DIR;
	}

	auto time = iso9660_time_to_time_t(entry->record_date);

	return file_handle{
		.dir_path = parent_dir,
		.name	  = std::string{entry_name},
		.data =
			{
				.location_on_disk = entry->extent_start.get(),
				.disk_id		  = disk_id,
				.size			  = entry->extent_length.get(),
				.flags			  = flags,
			},

		.time_created  = time,
		.time_modified = time,
	};
}

#define ISO_DEFAULT_SECTOR_SIZE 0x800

static fs_index iso9660_read_chunks(uint8_t* dest, fs_index location, size_t offset, size_t num_bytes, const file_data_block* file, const filesystem_virtual_drive* fd)
{
	iso9660_drive* f = (iso9660_drive*)fd->fs_impl_data;

	filesystem_read(fd, location << f->blocks_per_sector_log2, offset, dest, num_bytes);

	return location + ((offset + num_bytes) >> f->sector_size_log2);
}

static void iso9660_read_dir(directory_stream* dest, const file_data_block* file, const filesystem_virtual_drive* fd)
{
	const iso9660_drive* f = (iso9660_drive*)fd->fs_impl_data;

	const size_t num_sectors = (file->size + (f->sector_size - 1)) >> f->sector_size_log2;
	const size_t buffer_size = num_sectors << f->sector_size_log2;

	auto dir_data = std::make_unique<uint8_t[]>(buffer_size);

	iso9660_read_chunks(&dir_data[0], file->location_on_disk, 0, buffer_size, file, fd);

	const uint8_t* dir_ptr = &dir_data[0];

	while((size_t)(dir_ptr - &dir_data[0]) < file->size)
	{
		if(auto* entry = (const iso9660_directory_entry*)dir_ptr;
		   entry->length == 0)
		{
			dir_ptr++;
			continue;
		}
		else
		{
			dest->file_list.emplace_back(iso9660_read_dir_entry(dest->full_path, dir_ptr, fd->id));
			dir_ptr += entry->length;
		}	
	}
}

static mount_status iso9660_mount_disk(filesystem_virtual_drive* fd)
{
	iso9660_drive* f = new iso9660_drive{};
	fd->fs_impl_data = f;

	size_t blocks_per_sector = (ISO_DEFAULT_SECTOR_SIZE / fd->block_size);

	//find the primary volume descriptor
	iso9660_volume_descriptor descriptor;
	size_t sector = 0x10;
	do
	{
		filesystem_read(fd, sector++ * blocks_per_sector,
						0, (uint8_t*)&descriptor,
						sizeof(iso9660_volume_descriptor));

		if(memcmp(&descriptor.id, "CD001", 5 * sizeof(char)) != 0)
		{
			return UNKNOWN_FILESYSTEM;
		}
	} while(descriptor.type != 0x01);

	if(!std::has_single_bit(descriptor.logical_block_size.get()))
	{
		return MOUNT_FAILURE;
	}

	f->sector_size = descriptor.logical_block_size.get();
	f->sector_size_log2 = (size_t)std::countr_zero(f->sector_size);
	f->blocks_per_sector = f->sector_size / fd->block_size;
	f->blocks_per_sector_log2 = (size_t)std::countr_zero(f->blocks_per_sector);

	if(f->sector_size < fd->block_size)
	{
		return DRIVE_NOT_SUPPORTED;
	}

	fd->root_dir =
		iso9660_read_dir_entry(nullptr, (uint8_t*)&(descriptor.root), fd->id);

	fd->root_dir.name = fd->root_name;

	return MOUNT_SUCCESS;
}

static filesystem_driver iso9660_driver = {
	iso9660_mount_disk,
	iso9660_read_chunks,
	nullptr,
	nullptr,
	iso9660_read_dir,
	nullptr,
	nullptr
};

extern "C" void iso9660_init(void)
{
	filesystem_add_driver(&iso9660_driver);
}