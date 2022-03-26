#include <kernel/filesystem.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/util.h>
#include <kernel/kassert.h>
#include <stdlib.h>
#include <bit>
#include "drives.h"

constexpr size_t default_cache_size = 1024;

size_t calc_block_ratio(size_t block_size, size_t cache_size)
{
	k_assert(std::has_single_bit(block_size));
	k_assert(std::has_single_bit(cache_size));
	return (block_size > cache_size || cache_size % block_size != 0) ?
		1 : (cache_size / block_size);
}

struct filesystem_drive
{
	filesystem_drive(const disk_driver& disk_drv, 
					 void* driver_data, 
					 size_t block_size, 
					 size_t num_blocks, 
					 size_t index) 
		: m_drv_impl_data(driver_data)
		, m_driver(disk_drv)
		, m_index(index)
		, m_minimum_block_size(block_size)
		, m_blocksz_log2(std::countr_zero(block_size))
		, m_num_blocks(num_blocks)
		, m_num_blocks_per_cache(calc_block_ratio(block_size, default_cache_size))
		, m_max_cached_blocks(8)
	{
		//must have allocate & free or neither
		k_assert(!!disk_drv.allocate_buffer == !!disk_drv.free_buffer);
	};

	~filesystem_drive()
	{
		for(auto&& block : block_cache)
		{
			free_buffer(block.data, block_size());
		}
	}

	void write_blocks(size_t lba, const uint8_t* buf, size_t num_sectors) const
	{
		k_assert(m_driver.write_blocks);
		m_driver.write_blocks(m_drv_impl_data, lba, buf, num_sectors);
	}

	void read_blocks(size_t lba, uint8_t* buf, size_t num_sectors) const
	{
		k_assert(m_driver.read_blocks);
		m_driver.read_blocks(m_drv_impl_data, lba, buf, num_sectors);
	}

	uint8_t* allocate_buffer(size_t size) const
	{
		if(needs_buffer())
		{
			k_assert(m_driver.allocate_buffer);
			return m_driver.allocate_buffer(size);
		}
		return (uint8_t*)malloc(size);
	}

	int free_buffer(uint8_t* buffer, size_t size) const
	{
		if(needs_buffer())
		{
			k_assert(m_driver.free_buffer);
			return m_driver.free_buffer(buffer, size);
		}
		free(buffer);
		return 0;
	}

	bool read_only() const
	{
		return !(m_driver.write_blocks);
	}

	bool needs_buffer() const
	{
		return !!(m_driver.allocate_buffer);
	}

	size_t index() const
	{
		return m_index;
	}

	size_t num_blocks() const
	{
		return m_num_blocks;
	}

	size_t block_size() const
	{
		return m_minimum_block_size;
	}

	size_t block_size_log2() const
	{
		return m_blocksz_log2;
	}

	size_t blocks_to_bytes(size_t num_blocks) const
	{
		return num_blocks << m_blocksz_log2;
	}

	void write_to_block(size_t block,
						size_t offset,
						const uint8_t* buf,
						size_t num_bytes) const;

	void read_from_block(size_t block,
						 size_t offset,
						 uint8_t* buf,
						 size_t num_bytes) const;

private:

	void* m_drv_impl_data;
	const disk_driver& m_driver;
	size_t m_index;
	size_t m_minimum_block_size;
	size_t m_blocksz_log2;
	size_t m_num_blocks;
	size_t m_num_blocks_per_cache;
	size_t m_max_cached_blocks;

	struct cached_block {
		size_t index;
		uint8_t* data;
		bool dirty;
	};

	cached_block& block_rw(size_t block, bool write_all) const;

	mutable std::vector<cached_block> block_cache;
};

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
							|| drive->disk->read_only();
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
		if(partitioners[i](drive, base, drive->block_size()) == 0)
			return;
	}

	if(!base)
	{
		//no known partitioning on drive
		filesystem_add_virtual_drive(drive, 0, drive->num_blocks());
	}
}

extern "C" void filesystem_add_partitioner(partition_func p)
{
	partitioners.push_back(p);

	for(auto drive : drives)
	{
		auto& partitions = partition_map[drive->index()];
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

	k_assert(disk->index() < partition_map.size());
	partition_map[disk->index()].push_back(drive);
}

extern "C" void filesystem_add_driver(const filesystem_driver * fs_drv)
{
	fs_drivers.push_back(fs_drv);
}

filesystem_drive* filesystem_add_drive(const disk_driver* disk_drv, void* driver_data, size_t block_size, size_t num_blocks)
{
	k_assert(disk_drv);

	auto drive = new filesystem_drive(*disk_drv, driver_data,
									  block_size, num_blocks,
									  drives.size());

	drives.push_back(drive);
	partition_map.push_back(fs_drive_list{});

	filesystem_read_drive_partitions(drive, nullptr);

	return drives.back();
}

filesystem_drive::cached_block& 
filesystem_drive::block_rw(size_t index, bool write_all) const
{
	auto block = fs::align_power_2(index, m_num_blocks_per_cache);

	auto it = std::find_if(block_cache.begin(), block_cache.end(), [block](auto&& c) {
		return block == c.index;
	});

	if(it == block_cache.end())
	{
		uint8_t* disk_buf = nullptr;
		if(block_cache.size() >= m_max_cached_blocks)
		{
			disk_buf = block_cache.front().data;
			if(block_cache.front().dirty)
			{
				write_blocks(block_cache.front().index, disk_buf, m_num_blocks_per_cache);
			}
			block_cache.erase(block_cache.begin());
		}
		else
		{
			disk_buf = allocate_buffer(blocks_to_bytes(m_num_blocks_per_cache));
		}

		block_cache.push_back({block, disk_buf, false});

		k_assert(disk_buf);
		if(!write_all)
		{
			read_blocks(block, disk_buf, m_num_blocks_per_cache);
		}

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
	, block_size(disk->block_size())
	, root_dir{}
	, mounted(false)
	, read_only(false)
{}

void filesystem_drive::write_to_block(size_t block,
									  size_t offset,
									  const uint8_t* buf,
									  size_t num_bytes) const
{
	auto& blk = block_rw(block, num_bytes == blocks_to_bytes(m_num_blocks_per_cache));
	auto block_offset = blocks_to_bytes(block - blk.index);
	memcpy(blk.data + block_offset + offset, buf, num_bytes);
	blk.dirty = true;
}

void filesystem_drive::read_from_block(size_t block,
									   size_t offset,
									   uint8_t* buf,
									   size_t num_bytes) const
{
	auto& blk = block_rw(block, false);
	k_assert(blk.index <= block);
	auto block_offset = blocks_to_bytes(block - blk.index);
	memcpy(buf, blk.data + block_offset + offset, num_bytes);

	/*if(blk.index != block)
		printf("%d %d\n", blk.index, block);*/
}

void filesystem_write_to_disk(const filesystem_drive* disk,
							  size_t block_num,
							  size_t offset,
							  const uint8_t* buf,
							  size_t num_bytes)
{
	k_assert(disk);
	k_assert(!disk->read_only());

	auto blocks = filesystem_chunkify(offset, num_bytes, disk->block_size() - 1, disk->block_size_log2());
	auto block = block_num + blocks.start_chunk;

	if(blocks.start_size != 0)
	{
		disk->write_to_block(block++, blocks.start_offset, buf, blocks.start_size);
		buf += blocks.start_size;
	}

	if(blocks.num_full_chunks)
	{
		if(!disk->needs_buffer())
		{
			disk->write_blocks(block, buf, blocks.num_full_chunks);
			buf += disk->blocks_to_bytes(blocks.num_full_chunks);
			block += blocks.num_full_chunks;
		}
		else
		{
			auto num_chunks = blocks.num_full_chunks;
			while(num_chunks--)
			{
				disk->write_to_block(block++, 0, buf, disk->block_size());
				buf += disk->block_size();
			}
		}
	}

	if(blocks.end_size != 0)
	{
		disk->write_to_block(block, 0, buf, blocks.end_size);
	}
}

void filesystem_read_from_disk(const filesystem_drive* disk,
							   size_t block_num,
							   size_t offset,
							   uint8_t* buf,
							   size_t num_bytes)
{
	k_assert(disk);

	auto blocks = filesystem_chunkify(offset, num_bytes, disk->block_size()-1, disk->block_size_log2());
	auto block = block_num + blocks.start_chunk;

	if(blocks.start_size != 0)
	{
		disk->read_from_block(block++, blocks.start_offset, buf, blocks.start_size);
		buf += blocks.start_size;
	}

	if(blocks.num_full_chunks)
	{
		if(!disk->needs_buffer())
		{
			disk->read_blocks(block, buf, blocks.num_full_chunks);
			buf += disk->blocks_to_bytes(blocks.num_full_chunks);
			block += blocks.num_full_chunks;
		}
		else
		{
			auto num_chunks = blocks.num_full_chunks;
			while(num_chunks--)
			{
				disk->read_from_block(block, 0, buf, disk->block_size());
				buf += disk->block_size();
			}
		}
	}

	if(blocks.end_size != 0)
	{
		disk->read_from_block(block, 0, buf, blocks.end_size);
	}
}

void filesystem_raw_block_read(const filesystem_drive* d, size_t block, uint8_t* buf, size_t num_blocks)
{
	d->read_blocks(block, buf, num_blocks);
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
