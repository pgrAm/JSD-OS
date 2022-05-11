#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include <algorithm>
#include <bit>
#include <optional>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/filesystem/util.h>
#include <kernel/kassert.h>
#include <kernel/util/unicode.h>

struct superblock
{
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block;
	uint32_t log_block_size;
	uint32_t log_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t wtime;
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;
	uint16_t def_resuid;
	uint16_t def_resgid;
	uint32_t first_ino;
	uint16_t inode_size;
};
struct blkgrp
{
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks_count;
	uint16_t free_inodes_count;
	uint16_t used_dirs_count;
	uint16_t pad;
	uint8_t reserved[12];
};

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)

struct inode
{
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t dtime;
	uint16_t gid;
	uint16_t links_count;
	uint32_t blocks;
	uint32_t flags;
	uint32_t osd1;
	uint32_t block[15];
	uint32_t generation;
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t faddr;
	uint8_t osd2[12];
};
struct dirent
{
	uint32_t inode;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	uint8_t name[];
};

int find_free_and_mark(uint8_t* arr, int bits)
{
	for(int i = 0; i < bits; i++)
	{
		auto byte	 = &arr[i / 8];
		auto bitmask = 1 << (i % 8);
		if((*byte & bitmask) == 0)
		{
			*byte |= bitmask;
			return i;
		}
	}
	return -1;
}

struct ext2fs
{
	std::unique_ptr<superblock> sb;
	std::unique_ptr<blkgrp[]> blkgrps;
	size_t blks, num_blkgrps;
	filesystem_virtual_drive* d;

	void read(size_t block, size_t offset, uint8_t* data, size_t count)
	{
		auto byte = uint64_t(block) * blks + offset;
		filesystem_read(d, byte / d->block_size, byte % d->block_size, data,
						count);
	}

	void write(size_t block, size_t offset, const uint8_t* data, size_t count)
	{
		auto byte = uint64_t(block) * blks + offset;
		filesystem_write(d, byte / d->block_size, byte % d->block_size, data,
						 count);
	}

	void locate_inode(uint32_t ino, inode* i)
	{
		auto blkgrp = (ino - 1) / sb->inodes_per_group;
		auto idx	= (ino - 1) % sb->inodes_per_group;
		read(blkgrps[blkgrp].inode_table, idx * sb->inode_size, (uint8_t*)i,
			 sizeof(inode));
	}

	void update_inode(uint32_t ino, const inode* i)
	{
		auto blkgrp = (ino - 1) / sb->inodes_per_group;
		auto idx	= (ino - 1) % sb->inodes_per_group;
		write(blkgrps[blkgrp].inode_table, idx * sb->inode_size, (uint8_t*)i,
			  sizeof(inode));
	}

	uint32_t get_block_n(inode& i, uint32_t n)
	{
		auto s		 = blks / 4;
		uint32_t blk = 0;
		if(n < 12)
		{
			blk = i.block[n];
		}
		else if((n -= 12) < s)
		{
			read(i.block[EXT2_IND_BLOCK], n * 4, (uint8_t*)&blk, 4);
		}
		else if((n -= s) < (s * s))
		{
			read(i.block[EXT2_DIND_BLOCK], (n / s) * 4, (uint8_t*)&blk, 4);
			read(blk, (n % s) * 4, (uint8_t*)&blk, 4);
		}
		else
		{ // not tested
			n -= s * s;
			read(i.block[EXT2_TIND_BLOCK], (n / s / s) * 4, (uint8_t*)&blk, 4);
			read(blk, (n / s % s) * 4, (uint8_t*)&blk, 4);
			read(blk, (n % s) * 4, (uint8_t*)&blk, 4);
		}
		return blk;
	}
};

struct __attribute__((packed)) ext2_format_data
{
	uint32_t curr_inode;
	uint32_t parent_inode;
};

static mount_status ext2_mount_disk(filesystem_virtual_drive* d)
{
	auto fs = new ext2fs;
	fs->sb	= std::make_unique<superblock>();
	fs->d	= d;
	filesystem_read(d, 1024 / d->block_size, 1024 % d->block_size,
					(uint8_t*)fs->sb.get(), sizeof(superblock));
	if(fs->sb->magic != 0xEF53)
	{
		delete fs;
		return MOUNT_FAILURE;
	}
	fs->blks		= 1024 << fs->sb->log_block_size;
	fs->num_blkgrps = (fs->sb->blocks_count + fs->sb->blocks_per_group - 1) /
					  fs->sb->blocks_per_group;
	fs->blkgrps = std::make_unique<blkgrp[]>(fs->num_blkgrps);
	fs->read((fs->blks == 1024 ? 2 : 1), 0, (uint8_t*)fs->blkgrps.get(),
			 sizeof(blkgrp) * fs->num_blkgrps);
	d->root_dir = {
		.name		   = {},
		.data		   = {0, d->id, 0, IS_DIR},
		.time_created  = 0,
		.time_modified = 0,
	};
	ext2_format_data child_fdata = {.curr_inode = 2, .parent_inode = 2};
	memcpy(&d->root_dir.data.format_data[0], &child_fdata,
		   sizeof(ext2_format_data));

	d->fs_impl_data = fs;
	return MOUNT_SUCCESS;
}

static size_t ext2_read(uint8_t* buf, size_t start_cluster, size_t offset,
						size_t size, const file_data_block* file,
						const filesystem_virtual_drive* d)
{
	auto fs = (ext2fs*)d->fs_impl_data;
	ext2_format_data fdata;
	memcpy(&fdata, &file->format_data[0], sizeof(ext2_format_data));
	inode inod;
	fs->locate_inode(fdata.curr_inode, &inod);
	size_t br = 0;
	if(offset)
	{
		// handle first unaligned section
		size_t count = std::min(size, fs->blks - offset);
		fs->read(fs->get_block_n(inod, start_cluster), offset, buf, count);
		buf += count;
		size -= count;
		start_cluster++;
		br += count;
	}
	if(!size) return br;

	auto blocks = size / fs->blks;
	for(size_t i = 0; i < blocks; i++)
	{
		// read aligned blocks
		fs->read(fs->get_block_n(inod, start_cluster), 0, buf, fs->blks);
		buf += fs->blks;
		size -= fs->blks;
		start_cluster++;
		br += fs->blks;
	}
	if(!size) return br;

	fs->read(fs->get_block_n(inod, start_cluster), 0, buf,
			 size); // read the last part that's smaller than a block
	return br;
}

static size_t ext2_write(const uint8_t* buf, size_t start_cluster,
						 size_t offset, size_t size,
						 const file_data_block* file,
						 const filesystem_virtual_drive* d)
{
	return 0;
}


static size_t ext2_allocate_blocks(size_t start_cluster, size_t size_in_bytes,
								   const file_data_block* file,
								   const filesystem_virtual_drive* d)
{
	return 0;
}

enum filetypes
{
	EXT2_FT_UNKNOWN = 0,
	EXT2_FT_REG_FILE,
	EXT2_FT_DIR,
	EXT2_FT_CHRDEV,
	EXT2_FT_BLKDEV,
	EXT2_FT_FIFO,
	EXT2_FT_SOCK,
	EXT2_FT_SYMLINK
};

static void ext2_read_dir(directory_stream* dest, const file_data_block* dir,
						  const filesystem_virtual_drive* fd)
{
	auto fs = (ext2fs*)fd->fs_impl_data;
	ext2_format_data fdata;
	memcpy(&fdata, &dir->format_data[0], sizeof(ext2_format_data));
	inode inod;
	fs->locate_inode(fdata.curr_inode, &inod);

	auto blocks_to_read = inod.size / fs->blks;
	auto temp			= std::make_unique<uint8_t[]>(fs->blks);

	for(size_t i = 0; i < blocks_to_read; i++)
	{
		auto blk = fs->get_block_n(inod, i);
		fs->read(blk, 0, temp.get(), fs->blks);
		for(size_t j = 0; j < fs->blks;)
		{
			dirent* d = (dirent*)(temp.get() + j);
			if(!d->inode)
			{
				j += d->rec_len;
				continue;
			}
			inode child;
			fs->locate_inode(d->inode, &child);
			std::string name((char*)d->name, d->name_len);
			file_handle f = {
				.name		   = name,
				.data		   = {0, fs->d->id, child.size,
						  (d->file_type == EXT2_FT_DIR) ? IS_DIR : 0},
				.time_created  = child.ctime,
				.time_modified = child.mtime,
			};
			ext2_format_data child_fdata = {.curr_inode	  = d->inode,
											.parent_inode = fdata.curr_inode};
			memcpy(&f.data.format_data[0], &child_fdata,
				   sizeof(ext2_format_data));
			dest->file_list.push_back(f);
			j += d->rec_len;
		}
	}
}

static void ext2_update_file(const file_data_block* file,
							 const filesystem_virtual_drive* fd)
{
}

static const filesystem_driver ext2_driver = {
	ext2_mount_disk,
	ext2_read,
	ext2_write,
	ext2_allocate_blocks,
	ext2_read_dir,
	ext2_update_file,
	//NULL,
	//NULL
};

extern "C" void ext2_init()
{
	filesystem_add_driver(&ext2_driver);
}