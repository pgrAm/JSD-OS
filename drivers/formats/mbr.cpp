#include <kernel/filesystem/fs_driver.h>
#include <kernel/kassert.h>
#include <stdint.h>
#include <stdio.h>

#define MBR_SIZE 512

struct __attribute__((packed)) mbr_partition
{
	uint8_t  status;
	uint8_t  chs_first_sector[3];
	uint8_t  type;
	uint8_t  chs_last_sector[3];
	uint32_t lba_first_sector;
	uint32_t sector_count;
};

struct __attribute__((packed)) mbr_info
{
	mbr_partition	entries[4];
	uint8_t			signature[2];
};

struct __attribute__((packed)) mbr 
{
	uint8_t			boot_code[446];
	mbr_info		data;
};

static int mbr_read_partitions(filesystem_drive* d, filesystem_virtual_drive* base, size_t block_size)
{
	mbr_info boot;
	filesystem_read_from_disk(d, 0, offsetof(mbr, data), (uint8_t*)&boot, sizeof(mbr_info));

	int return_val = -1;

	if((boot.signature[0] == 0x55 || boot.signature[1] == 0xAA))
	{
		//kinda a hack to ignore fat boot sectors
		uint8_t boot_code[3];
		filesystem_read_from_disk(d, 0, 0, (uint8_t*)&boot_code, sizeof(boot_code));
		if(boot_code[0] == 0xEB || boot_code[2] == 0x90)
		{
			return 0;
		}

		return_val = 0;

		for(size_t i = 0; i < 4; i++)
		{
			mbr_partition* p = &boot.entries[i];
			if(p->sector_count != 0)
			{
				if(base)
				{
					base->first_block = p->lba_first_sector;
					base->num_blocks = p->sector_count;
					base = nullptr;
				}
				else
				{
					filesystem_add_virtual_drive(d, p->lba_first_sector, p->sector_count);
				}
			}
		}
	}

	return return_val;
}

extern "C" void mbr_init(void)
{
	filesystem_add_partitioner(mbr_read_partitions);
}