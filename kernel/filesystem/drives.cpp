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
				drive->fs_impl_data = nullptr;
			}
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
	auto drive = new filesystem_virtual_drive{disk, begin, size};

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

struct fs_chunks
{
	size_t start_chunk;

	size_t start_offset;	//offset to start in the first chunk
	size_t start_size;		//size of the first chunk

	size_t num_full_chunks;	//number of complete chunks

	size_t end_size;		//size of the last chunk
};

static fs_chunks filesystem_chunkify(size_t offset, size_t length, size_t chunk_size)
{
	size_t first_chunk = offset / chunk_size;
	size_t start_offset = offset % chunk_size;

	size_t last_chunk = (offset + length) / chunk_size;
	size_t end_size = (offset + length) % chunk_size;

	if(first_chunk == last_chunk)
	{
		end_size = 0; //start and end are in the same chunk
	}

	size_t start_size = ((length - end_size) % chunk_size);
	size_t num_chunks = (length - (start_size + end_size)) / chunk_size;

	return {first_chunk, start_offset, start_size, num_chunks, end_size};
}


filesystem_virtual_drive::cached_block& 
filesystem_virtual_drive::block_rw(size_t block) const
{
	auto it = std::find_if(block_cache.begin(), block_cache.end(), [block](auto&& c) {
		return c.index == block;
	});

	if(it == block_cache.cend())
	{
		uint8_t* disk_buf = nullptr;
		if(block_cache.size() >= max_cached_blocks)
		{
			disk_buf = block_cache.front().data;
			if(block_cache.front().dirty)
			{
				disk->driver->write_blocks(disk, first_block + block, disk_buf, 1);
			}
			block_cache.erase(block_cache.begin());
		}
		else
		{
			disk_buf = filesystem_allocate_buffer(disk, disk->minimum_block_size);
		}
		block_cache.push_back({block, disk_buf, false});

		k_assert(disk_buf);

		disk->driver->read_blocks(disk, first_block + block, disk_buf, 1);

		return block_cache.back();
	}
	return *it;
}

filesystem_virtual_drive::filesystem_virtual_drive(filesystem_drive* disk_, 
												   fs_index begin, 
												   size_t size)
	: disk(disk_)
	, fs_impl_data(nullptr)
	, fs_driver(nullptr)
	, id(virtual_drives.size())
	, first_block(begin)
	, num_blocks(size)
	, chunk_read_size(disk->minimum_block_size > DEFAULT_CHUNK_READ_SIZE ?
					  disk->minimum_block_size : DEFAULT_CHUNK_READ_SIZE)
	, root_dir{}
	, mounted(false)
	, read_only(false)
	, max_cached_blocks(8)
{}

void filesystem_virtual_drive::write_block(size_t block,
										   size_t offset,
										   const uint8_t* buf,
										   size_t num_bytes) const
{
	auto blk = block_rw(block);
	memcpy(blk.data + offset, buf, num_bytes);
	blk.dirty = true;
}

void filesystem_virtual_drive::read_block(size_t block,
										  size_t offset,
										  uint8_t* buf,
										  size_t num_bytes) const
{
	auto blk = block_rw(block);
	memcpy(buf, blk.data + offset, num_bytes);
}

void filesystem_write_to_disk(const filesystem_virtual_drive* d,
							  size_t block_num,
							  size_t offset,
							  const uint8_t* buf,
							  size_t num_bytes)
{
	k_assert(d);
	k_assert(d->disk);
	k_assert(d->disk->driver);
	k_assert(d->disk->driver->write_blocks);

	auto* disk = d->disk;
	auto block_size = disk->minimum_block_size;
	auto blocks = filesystem_chunkify(offset, num_bytes, block_size);
	auto block = block_num + blocks.start_chunk;

	if(blocks.start_size != 0)
	{
		d->write_block(block++, blocks.start_offset, buf, blocks.start_size);
		buf += blocks.start_size;
	}

	if(blocks.num_full_chunks)
	{
		if(disk->driver->allocate_buffer == nullptr)
		{
			disk->driver->write_blocks(disk, d->first_block + block, buf, blocks.num_full_chunks);
			buf += blocks.num_full_chunks * block_size;
			block += blocks.num_full_chunks;
		}
		else
		{
			auto num_chunks = blocks.num_full_chunks;
			while(num_chunks--)
			{
				d->write_block(block++, 0, buf, block_size);
				buf += block_size;
			}
		}
	}

	if(blocks.end_size != 0)
	{
		d->write_block(block, 0, buf, blocks.end_size);
	}
}

void filesystem_read_from_disk(const filesystem_virtual_drive* d,
							   size_t block_num,
							   size_t offset,
							   uint8_t* buf,
							   size_t num_bytes)
{
	k_assert(d);
	k_assert(d->disk);
	k_assert(d->disk->driver);
	k_assert(d->disk->driver->read_blocks);

	auto* disk = d->disk;
	auto block_size = disk->minimum_block_size;
	auto blocks = filesystem_chunkify(offset, num_bytes, block_size);
	auto block = block_num + blocks.start_chunk;

	if(blocks.start_size != 0)
	{
		d->read_block(block++, blocks.start_offset, buf, blocks.start_size);
		buf += blocks.start_size;
	}

	if(blocks.num_full_chunks)
	{
		if(disk->driver->allocate_buffer == nullptr)
		{
			disk->driver->read_blocks(disk, d->first_block + block, buf, blocks.num_full_chunks);
			buf += blocks.num_full_chunks * block_size;
			block += blocks.num_full_chunks;
		}
		else
		{
			auto num_chunks = blocks.num_full_chunks;
			while(num_chunks--)
			{
				d->read_block(block, 0, buf, block_size);
				buf += block_size;
			}
		}
	}

	if(blocks.end_size != 0)
	{
		d->read_block( block, 0, buf, blocks.end_size);
	}
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
