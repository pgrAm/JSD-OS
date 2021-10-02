#include <kernel/fs_driver.h>
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

struct __attribute__((packed)) mbr 
{
	uint8_t			boot_code[446];
	mbr_partition	entries[4];
	uint8_t			signature[2];
};

int mbr_read_partitions(filesystem_drive* d)
{
	size_t num_sectors = (MBR_SIZE + (d->minimum_block_size - 1)) / d->minimum_block_size;
	size_t buffer_size = num_sectors * d->minimum_block_size;

	uint8_t* buffer = filesystem_allocate_buffer(d, buffer_size);

	d->driver->read_blocks(d, 0, buffer, num_sectors);

	mbr* master_boot_record = (mbr*)buffer;

	int return_val = -1;

	//the last 2 are kinda a hack to ignore fat boot sectors
	if((master_boot_record->signature[0] == 0x55 ||
		master_boot_record->signature[1] == 0xAA) &&

	   (master_boot_record->boot_code[0] != 0xEB &&
		master_boot_record->boot_code[2] != 0x90))
	{
		return_val = 0;

		for(size_t i = 0; i < 4; i++)
		{
			mbr_partition* p = &master_boot_record->entries[i];
			if(p->sector_count != 0)
			{
				printf("partition 1 at %d\n", p->lba_first_sector);
				filesystem_add_virtual_drive(d, p->lba_first_sector, p->sector_count);
			}
		}
	}

	filesystem_free_buffer(d, buffer, buffer_size);

	return return_val;
}

extern "C" void mbr_init(void)
{
	filesystem_add_partitioner(mbr_read_partitions);
}