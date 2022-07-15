#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <kernel/filesystem/fs_driver.h>
#include <kernel/interrupt.h>
#include <kernel/locks.h>
#include <kernel/sysclock.h>
#include <kernel/kassert.h>

#include <drivers/portio.h>
#include <drivers/isa_dma.h>
#include <drivers/cmos.h>

struct floppy_disk_info {
	uint8_t sectors_per_track;
	uint8_t heads_per_cylinder;
	uint8_t num_tracks;
	uint8_t gap_len;
};

floppy_disk_info floppy_types[] =
{
	{9,	 2, 40, 0x2A}, //360K
	{15, 2, 80, 0x1B}, //1.2M
	{9,  2, 80, 0x2A}, //720k
	{18, 2, 80, 0x1B}, //1.44M
	{36, 2, 80, 0x1B}, //2.88M
};

#define FLOPPY_BYTES_PER_SECTOR			512

static void floppy_sendbyte(uint8_t byte);
static uint8_t floppy_getbyte();
static void floppy_reset(uint8_t driveNum);
static bool floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head);

enum floppy_registers : uint16_t
{
   DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   DATA_FIFO                        = 0x3F5,
   CONFIGURATION_CONTROL_REGISTER   = 0x3F7, // write-only
};

enum floppy_commands : uint8_t
{
   READ_TRACK 			= 2,	// generates IRQ6
   SPECIFY 				= 3,	// * set drive parameters
   SENSE_DRIVE_STATUS 	= 4,
   WRITE_DATA 			= 5,	// * write to the disk
   READ_DATA 			= 6,	// * read from the disk
   RECALIBRATE 			= 7,	// * seek to cylinder 0
   SENSE_INTERRUPT 		= 8,	// * ack IRQ6, get status of last command
   WRITE_DELETED_DATA 	= 9,
   READ_ID 				= 10,	// generates IRQ6
   READ_DELETED_DATA 	= 12,
   FORMAT_TRACK 		= 13,	// *
   SEEK 				= 15,	// * seek both heads to cylinder X
   VERSION				= 16,	// * used during initialization, once
   SCAN_EQUAL 			= 17,
   PERPENDICULAR_MODE 	= 18,	// * used during initialization, once, maybe
   CONFIGURE 			= 19,	// * set controller parameters
   LOCK					= 20,	// * protect controller params from a reset
   VERIFY				= 22,
   SCAN_LOW_OR_EQUAL	= 25,
   SCAN_HIGH_OR_EQUAL	= 29
};

#define MFM_BIT 0x40
#define MT_BIT 0x80

static bool motor_is_ready[2] = {false, false};
static constinit sync::mutex irq6_mtx{};
static constinit sync::condition_variable irq6_condition{};

static void floppy_read_blocks(void* drv_data, size_t block_number, uint8_t* buf, size_t num_bytes);
static void floppy_write_blocks(void* drv_data, size_t block_number, const uint8_t* buf, size_t num_bytes);
//static uint8_t* floppy_allocate_buffer(size_t size);
//static int floppy_free_buffer(uint8_t* buffer, size_t size);

static disk_driver floppy_driver = {
	floppy_read_blocks,
	floppy_write_blocks,
	isa_dma_allocate_buffer,
	isa_dma_free_buffer
};

static INTERRUPT_HANDLER void floppy_irq_handler(interrupt_frame* r)
{
	setup_segs();

	acknowledge_irq(6);
	irq6_condition.notify_one();
}

static void wait_for_irq6(void)
{
	sync::unique_lock l{irq6_mtx};
	irq6_condition.wait(l);
}

struct floppy_drive
{
	sync::mutex mutex;
	size_t num_sectors = 0;
	uint8_t type = 0;
	uint8_t sectors_per_track = 0;
	uint8_t heads_per_cylinder = 0;
	uint8_t num_tracks = 0;
	uint8_t gap_len = 0;
	uint8_t drive_index = 0;
};

static constinit floppy_drive floppy_drives[2];
static size_t num_floppy_drives = 0;

static void floppy_set_drive_params(uint8_t type, floppy_drive* f)
{
	f->type = type;

	if(type > 1 && type <= 5)
	{
		f->sectors_per_track = floppy_types[type - 1].sectors_per_track;
		f->heads_per_cylinder = floppy_types[type - 1].heads_per_cylinder;
		f->num_tracks = floppy_types[type - 1].num_tracks;
		f->gap_len = floppy_types[type - 1].gap_len;
	}
	else
	{
		return;
	}

	f->num_sectors = f->num_tracks * f->sectors_per_track * f->heads_per_cylinder;

	num_floppy_drives++;
}

static void floppy_configure()
{
	floppy_sendbyte(CONFIGURE);
	floppy_sendbyte(0);
	floppy_sendbyte(1 << 6 | 0 << 5 | 1 << 4 | (8 - 1));
	floppy_sendbyte(8);
}

/*static uint8_t* floppy_allocate_buffer(size_t size)
{
	return isa_dma_allocate_buffer(size);
}

static int floppy_free_buffer(uint8_t* buffer, size_t size)
{
	return isa_dma_free_buffer(buffer, size);
}*/

extern "C" void floppy_init()
{
	uint8_t floppyTypes = cmos_get_register(0x10);

	floppy_set_drive_params(floppyTypes & 0x0f, &floppy_drives[1]);
	floppy_set_drive_params(floppyTypes >> 4, &floppy_drives[0]);

	printf("\t%d floppy drives detected\n", num_floppy_drives);

	if(num_floppy_drives == 0)
	{
		return;
	}

	irq_install_handler(6, floppy_irq_handler);

	outb(DIGITAL_OUTPUT_REGISTER, 0x0C); //make sure interrupt is enabled

	floppy_configure();

	for(size_t i = 0; i < num_floppy_drives; i++)
	{
		floppy_reset((uint8_t)i);
		floppy_drives[i].drive_index = (uint8_t)i;

		filesystem_add_drive(&floppy_driver, &floppy_drives[i],
							 FLOPPY_BYTES_PER_SECTOR,
							 floppy_drives[i].num_sectors);
	}
}

struct chs_location {
	uint8_t cylinder;
	uint8_t head;
	uint8_t sector;
};

static chs_location lba_to_chs(const floppy_drive* d, size_t lba)
{
	size_t track = lba / d->sectors_per_track;
	
	return {
		.cylinder = (uint8_t)(track / (size_t)d->heads_per_cylinder),
		.head = (uint8_t)(track % (size_t)d->heads_per_cylinder),
		.sector = (uint8_t)((lba % (size_t)d->sectors_per_track) + 1)
	};
}

static bool floppy_do_rw(const floppy_drive* d, floppy_commands cmd, chs_location loc)
{
	k_assert(loc.cylinder < d->num_tracks);
	k_assert(loc.head < d->heads_per_cylinder);
	k_assert(loc.sector <= d->sectors_per_track);
	k_assert(loc.sector > 0);

	floppy_sendbyte(MFM_BIT | MT_BIT | cmd);
	floppy_sendbyte((uint8_t)(loc.head << 2) | d->drive_index);
	floppy_sendbyte(loc.cylinder);
	floppy_sendbyte(loc.head);
	floppy_sendbyte(loc.sector);
	floppy_sendbyte(2);  		// sector size = 128*2^size
	floppy_sendbyte(d->sectors_per_track); 		// number of sectors
	floppy_sendbyte(d->gap_len);
	floppy_sendbyte(0xff);      // default value for data length

	//this will wait forever if no media is present
	wait_for_irq6(); // wait for irq6 to signal operation complete

	uint8_t st0 = floppy_getbyte();

	for(int b = 0; b < 6; b++)
	{
		floppy_getbyte();
	}

	floppy_sendbyte(SENSE_INTERRUPT); //now acknowledge the interrupt
	floppy_getbyte();
	floppy_getbyte();

	if((st0 & 0xc0) == 0)
	{
		return true; //r/w success!
	}

	floppy_reset(d->drive_index);

	return false;
}

static void floppy_read_blocks(void* drv_data, size_t lba, uint8_t* buf, size_t num_sectors)
{
	floppy_drive* d = (floppy_drive*)drv_data;

	sync::lock_guard l{d->mutex};

	if(lba > d->num_sectors)
	{
		printf("lba#%d is out of bounds for floppy read\n", lba);
		printf("lba max = %d\n", d->num_sectors);
		while(true);
	}

	const auto loc = lba_to_chs(d, lba);

	//printf("lba = %d, chs = %d %d %d, drv = %d\n", lba, loc.cylinder, loc.head, loc.sector, d->drive_index);

	for(uint8_t i = 0; i < 5; i++)
	{
		if(!floppy_seek(d->drive_index, loc.cylinder, loc.head))
			continue;

		isa_dma_begin_transfer(0x02, ISA_DMA_READ, buf, FLOPPY_BYTES_PER_SECTOR * num_sectors);

		if(floppy_do_rw(d, READ_DATA, loc))
			return;

		printf("read failure #%d\n", i);
	}

	printf("Catastrophic read failure at sector %d\n", lba);
}

static void floppy_write_blocks(void* drv_data, size_t lba, const uint8_t* buf, size_t num_sectors)
{
	//k_assert(false);

	floppy_drive* d = (floppy_drive*)drv_data;

	sync::lock_guard l{d->mutex};

	if(lba + num_sectors > d->num_sectors)
	{
		printf("lba#%d is out of bounds for floppy write\n", lba);
		printf("lba max = %d\n", d->num_sectors);
		while(true);
	}

	const auto loc = lba_to_chs(d, lba);

	for(uint8_t i = 0; i < 5; i++)
	{
		if(!floppy_seek(d->drive_index, loc.cylinder, loc.head))
			continue;

		isa_dma_begin_transfer(0x02, ISA_DMA_WRITE, buf, FLOPPY_BYTES_PER_SECTOR * num_sectors);

		if(floppy_do_rw(d, WRITE_DATA, loc))
			return;

		printf("write failure #%d\n", i);
	}

	printf("Catastrophic write failure at sector %d\n", lba);
}

//sendbyte() routine from intel manual
static void floppy_sendbyte(uint8_t byte)
{
	for(uint8_t tmo = 0; tmo < 128; tmo++)
	{
		if((inb(MAIN_STATUS_REGISTER) & 0xc0) == 0x80)
		{
			outb(DATA_FIFO, byte);
			return;
		}

		inb(0x80); //delay 
	}
}

//getbyte() routine from intel manual
static uint8_t floppy_getbyte()
{
	for(uint8_t tmo = 0; tmo < 128; tmo++)
	{
		if((inb(MAIN_STATUS_REGISTER) & 0xd0) == 0xd0)
		{
			return inb(DATA_FIFO);
		}

		inb(0x80); //delay
	}

	return 0xFF; //read timeout 
}

static void floppy_motor_on(uint8_t driveNum)
{
	if(!motor_is_ready[driveNum])
	{
		outb(DIGITAL_OUTPUT_REGISTER, (uint8_t)(0x10 << driveNum) | 0x0c | (driveNum & 0x03));
		sysclock_sleep(500, MILLISECONDS);			// wait 500ms for motor to spin up
		motor_is_ready[driveNum] = true;
	}
}

static void floppy_motor_off(uint8_t driveNum)
{
	if(motor_is_ready[driveNum])
	{
		outb(DIGITAL_OUTPUT_REGISTER, 0x0c | (driveNum & 0x03));
		sysclock_sleep(2000, MILLISECONDS);   // start motor kill countdown: 2s
		motor_is_ready[driveNum] = false;
	}
}

static int floppy_calibrate(uint8_t driveNum)
{
	floppy_motor_on(driveNum);

	for(size_t i = 0; i < 10; i++)
	{
		// Attempt to positions head to cylinder 0
		floppy_sendbyte(RECALIBRATE);
		floppy_sendbyte(driveNum); // argument is drive

		wait_for_irq6(); // wait for irq6 to signal operation complete

		floppy_sendbyte(SENSE_INTERRUPT);
		uint8_t st0 = floppy_getbyte();
		uint8_t cyl = floppy_getbyte();

		if(!(st0 & 0xC0))
		{
			if(cyl == 0) //reached cylinder 0
			{
				floppy_motor_off(driveNum);
				return 0;
			}

			printf("Cylinder 0 not reached\n");
		}

		printf("floppy calibrate failed %X\n", st0);
	}

	floppy_motor_off(driveNum);
	return -1;
}

static void floppy_reset(uint8_t driveNum)
{
	outb(DIGITAL_OUTPUT_REGISTER, 0x00);
	outb(DIGITAL_OUTPUT_REGISTER, 0x0C);

	wait_for_irq6(); 			// wait for irq6 to signal operation complete

	floppy_sendbyte(SENSE_INTERRUPT);
	floppy_getbyte();
	floppy_getbyte();

	outb(CONFIGURATION_CONTROL_REGISTER, 0x00);

	floppy_sendbyte(SPECIFY);
	floppy_sendbyte(8 << 4 | 15);	// SRT = 3ms (8), HUT = 240ms (15)
	floppy_sendbyte(5 << 1 | 0);	// HLT = 16ms (5), NoDma = 0, use DMA

	floppy_configure();

	floppy_calibrate(driveNum);
}

static bool floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head)
{
	for(int i = 0; i < 3; i++)
	{
		// if motor isn't on then turn it on
		floppy_motor_on(driveNum);

		floppy_sendbyte(SEEK);
		floppy_sendbyte((uint8_t)(head << 2) | driveNum);
		floppy_sendbyte(cylinder);

		wait_for_irq6();// wait for operation to complete

		floppy_sendbyte(SENSE_INTERRUPT);
		uint8_t st0 = floppy_getbyte();
		uint8_t fdc_cylinder = floppy_getbyte();

		if(st0 & 0xC0)
		{
			continue;
		}

		if(fdc_cylinder == cylinder) return true;
	}

	return false;
}