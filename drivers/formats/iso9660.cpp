#include <kernel/fs_driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string_view>

struct iso9660_drive
{
	size_t sector_size; //almost always 2KiB
	size_t blocks_per_sector;
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

static size_t iso9660_read_dir_entry(file_handle& dest, const uint8_t* entry_ptr, size_t disk_id)
{
	auto* entry = (const iso9660_directory_entry*)entry_ptr;

	if(entry->length == 0)
		return entry->length;

	size_t name_size = entry->name_len;

	std::string_view full_name =
	{
		(const char*)entry_ptr + offsetof(iso9660_directory_entry, name),
		name_size
	};

	if(auto pos = full_name.find(';'); pos != std::string_view::npos)
	{
		full_name = full_name.substr(0, pos);
	}

	if(full_name.empty() || full_name[0] == '\0')
	{
		dest.name = ".";
	}
	else if(full_name == "\1")
	{
		dest.name = "..";
	}
	else
	{
		dest.name = full_name;
	}

	dest.time_created = iso9660_time_to_time_t(entry->record_date);
	dest.time_modified = dest.time_created;

	uint32_t flags = 0;
	if(entry->flags & flags::DIRECTORY)
	{
		flags |= IS_DIR;
	}

	dest.data = {
		.location_on_disk = entry->extent_start.get(),
		.disk_id = disk_id,
		.size = entry->extent_length.get(),
		.flags = flags
	};

	return entry->length;
}

#define ISO_DEFAULT_SECTOR_SIZE 0x800

static fs_index iso9660_get_relative_location(fs_index location, size_t byte_offset, const filesystem_virtual_drive* fd)
{
	iso9660_drive* f = (iso9660_drive*)fd->fs_impl_data;

	size_t num_sectors = byte_offset / f->sector_size;

	return location + num_sectors;
}

static fs_index iso9660_read_chunks(uint8_t* dest, fs_index location, size_t num_bytes, const filesystem_virtual_drive* fd)
{
	iso9660_drive* f = (iso9660_drive*)fd->fs_impl_data;

	size_t num_sectors = num_bytes / f->sector_size;

	filesystem_read_blocks_from_disk(fd, location * f->blocks_per_sector, dest, num_sectors * f->blocks_per_sector);

	return location + num_sectors;
}

static void iso9660_read_dir(directory_stream* dest, const file_data_block* file, const filesystem_virtual_drive* fd)
{
	const iso9660_drive* f = (iso9660_drive*)fd->fs_impl_data;

	const size_t num_sectors = (file->size + (f->sector_size - 1)) / f->sector_size;
	const size_t buffer_size = num_sectors * f->sector_size;

	filesystem_buffer dir_data{fd->disk, buffer_size};

	iso9660_read_chunks(&dir_data[0], file->location_on_disk, buffer_size, fd);

	const uint8_t* dir_ptr = &dir_data[0];

	while((size_t)(dir_ptr - &dir_data[0]) < file->size)
	{
		file_handle out;
		if(auto length = iso9660_read_dir_entry(out, dir_ptr, fd->id); length == 0)
		{
			dir_ptr++;
			continue;
		}
		else
		{
			dest->file_list.push_back(out);
			dir_ptr += length;
		}	
	}
}

static int iso9660_mount_disk(filesystem_virtual_drive* fd)
{
	iso9660_drive* f = new iso9660_drive{};
	fd->fs_impl_data = f;

	size_t blocks_per_sector = (ISO_DEFAULT_SECTOR_SIZE / fd->disk->minimum_block_size);

	//find the primary volume descriptor
	filesystem_buffer buffer{fd->disk, ISO_DEFAULT_SECTOR_SIZE};
	size_t sector = 0x10;
	do
	{
		filesystem_read_blocks_from_disk(fd, sector++ * blocks_per_sector, &buffer[0], blocks_per_sector);
		if(memcmp(&buffer[1], "CD001", 5 * sizeof(char)) != 0)
		{
			return UNKNOWN_FILESYSTEM;
		}
	} while(buffer[0] != 0x01);

	iso9660_volume_descriptor* volume_descriptor = (iso9660_volume_descriptor*)&buffer[0];

	f->sector_size = volume_descriptor->logical_block_size.get();
	f->blocks_per_sector = f->sector_size / fd->disk->minimum_block_size;

	if(f->sector_size < fd->disk->minimum_block_size)
	{
		return DRIVE_NOT_SUPPORTED;
	}

	iso9660_read_dir_entry(fd->root_dir, (uint8_t*)&(volume_descriptor->root), fd->id);

	return MOUNT_SUCCESS;
}

static filesystem_driver iso9660_driver = {
	iso9660_mount_disk,
	iso9660_get_relative_location,
	iso9660_read_chunks,
	iso9660_read_dir
};

extern "C" void iso9660_init(void)
{
	filesystem_add_driver(&iso9660_driver);
}