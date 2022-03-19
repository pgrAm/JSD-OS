#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/kassert.h>
#include "drives.h"

#define DEFAULT_CHUNK_READ_SIZE 1024

using fs_drive_list = std::vector<filesystem_virtual_drive*>;
using fs_part_map = std::vector<fs_drive_list>;

static std::vector<filesystem_drive*> drives;
static std::vector<filesystem_virtual_drive*> virtual_drives;
static std::vector<const filesystem_driver*> fs_drivers;
static std::vector<partition_func> partitioners;

static fs_part_map partition_map;

size_t filesystem_get_num_drives()
{
	return virtual_drives.size();
}

filesystem_virtual_drive* filesystem_get_drive(size_t index)
{
	k_assert(index < virtual_drives.size());
	auto drive = virtual_drives[index];

	k_assert(drive->mounted);
	k_assert(drive->fs_driver);
	k_assert(drive->fs_driver->read_chunks);
	k_assert(drive->disk);
	k_assert(drive->disk->driver);
	k_assert(drive->disk->driver->read_blocks);

	return drive;
}

static bool filesystem_mount_drive(filesystem_virtual_drive* drive)
{
	k_assert(drive);
	k_assert(drive->disk);

	if(!drive->mounted)
	{
		if(drive->fs_driver == nullptr)
		{
			for(auto& driver : fs_drivers)
			{
				drive->fs_driver = driver;
				int mount_result = driver->mount_disk(drive);

				if(mount_result != UNKNOWN_FILESYSTEM && 
				   mount_result != DRIVE_NOT_SUPPORTED)
				{
					if(mount_result == MOUNT_SUCCESS)
					{
						drive->mounted = true;
						drive->read_only = !driver->write_chunks
										|| !drive->disk->driver->write_blocks;
					}
					break;
				}
				drive->fs_driver = nullptr;
				drive->fs_impl_data = nullptr;			}
		}
		else
		{
			k_assert(drive->fs_driver);
			drive->mounted = drive->fs_driver->mount_disk(drive);
		}
	}

	return drive->mounted;
}

static void filesystem_read_drive_partitions(filesystem_drive* drive, filesystem_virtual_drive* base)
{
	k_assert(!base || base->disk == drive);
	k_assert(!base || !base->mounted);

	for(size_t i = 0; i < partitioners.size(); i++)
	{
		if(partitioners[i](drive, base) == 0)
			return;
	}

	if(!base)
	{
		//no known partitioning on drive
		filesystem_add_virtual_drive(drive, 0, drive->num_blocks);
	}
}

extern "C" void filesystem_add_partitioner(partition_func p)
{
	partitioners.push_back(p);

	for(auto drive : drives)
	{
		auto& partitions = partition_map[drive->index];
		if(partitions.size() > 1)
		{
			continue;
		}
		if(partitions.size() == 1)
		{
			if(!partitions[0]->mounted)
			{
				filesystem_read_drive_partitions(drive, partitions[0]);
			}
			continue;
		}

		filesystem_read_drive_partitions(drive, nullptr);
	}
}

extern "C" void filesystem_add_virtual_drive(filesystem_drive * disk, fs_index begin, size_t size)
{
	auto drive = new filesystem_virtual_drive{
		.disk = disk,
		.fs_impl_data = nullptr,
		.fs_driver = nullptr,
		.id = virtual_drives.size(),
		.first_block = begin,
		.num_blocks = size,
		.chunk_read_size = disk->minimum_block_size > DEFAULT_CHUNK_READ_SIZE ?
							disk->minimum_block_size : DEFAULT_CHUNK_READ_SIZE,
		.root_dir = {},
		.mounted = false,
		.read_only = false
	};

	virtual_drives.push_back(drive);

	k_assert(disk->index < partition_map.size());
	partition_map[disk->index].push_back(drive);
}

extern "C" void filesystem_add_driver(const filesystem_driver * fs_drv)
{
	fs_drivers.push_back(fs_drv);
}

filesystem_drive* filesystem_add_drive(const disk_driver* disk_drv, void* driver_data, size_t block_size, size_t num_blocks)
{
	auto drive = new filesystem_drive
	{
		.drv_impl_data = driver_data,
		.driver = disk_drv,
		.index = drives.size(),
		.minimum_block_size = block_size,
		.num_blocks = num_blocks
	};

	drives.push_back(drive);
	partition_map.push_back(fs_drive_list{});

	filesystem_read_drive_partitions(drive, nullptr);

	return drives.back();
}

const file_handle* filesystem_get_root_directory(size_t drive_number)
{
	k_assert(drive_number < virtual_drives.size());

	auto drive = virtual_drives[drive_number];

	if(filesystem_mount_drive(drive))
	{
		return &drive->root_dir;
	}

	return nullptr;
}

SYSCALL_HANDLER const file_handle* syscall_get_root_directory(size_t drive_number)
{
	if(drive_number >= virtual_drives.size())
	{
		return nullptr;
	}

	return filesystem_get_root_directory(drive_number);
}
