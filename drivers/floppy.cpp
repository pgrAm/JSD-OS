#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <kernel/fs_driver.h>
#include <kernel/interrupt.h>
#include <kernel/locks.h>
#include <kernel/sysclock.h>

#include <drivers/portio.h>
#include <drivers/isa_dma.h>
#include <drivers/cmos.h>

#define FLOPPY_360_SECTORS_PER_TRACK	9
#define FLOPPY_360_HEADS_PER_CYLINDER	2
#define FLOPPY_360_NUMBER_OF_TRACKS		40

#define FLOPPY_1_2_SECTORS_PER_TRACK	15
#define FLOPPY_1_2_HEADS_PER_CYLINDER	2
#define FLOPPY_1_2_NUMBER_OF_TRACKS		80

#define FLOPPY_720_SECTORS_PER_TRACK	9
#define FLOPPY_720_HEADS_PER_CYLINDER	2
#define FLOPPY_720_NUMBER_OF_TRACKS		80

#define FLOPPY_144_SECTORS_PER_TRACK	18
#define FLOPPY_144_HEADS_PER_CYLINDER	2
#define FLOPPY_144_NUMBER_OF_TRACKS		80

#define FLOPPY_288_SECTORS_PER_TRACK	36
#define FLOPPY_288_HEADS_PER_CYLINDER	2
#define FLOPPY_288_NUMBER_OF_TRACKS		80

#define FLOPPY_BYTES_PER_SECTOR			512

static void floppy_sendbyte(uint8_t byte);
static uint8_t floppy_getbyte();
static void floppy_reset(uint8_t driveNum);
static bool floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head);

enum floppy_registers
{
   DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   DATA_FIFO                        = 0x3F5,
   CONFIGURATION_CONTROL_REGISTER   = 0x3F7, // write-only
};

enum floppy_commands
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
static kernel_cv irq6_condition = {-1, 1};

static void floppy_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_bytes);
//static uint8_t* floppy_allocate_buffer(size_t size);
//static int floppy_free_buffer(uint8_t* buffer, size_t size);

static disk_driver floppy_driver = {
	floppy_read_blocks,
	isa_dma_allocate_buffer,
	isa_dma_free_buffer
};

static INTERRUPT_HANDLER void floppy_irq_handler(interrupt_frame* r)
{
	acknowledge_irq(6);
	kernel_signal_cv(&irq6_condition);
}

static void wait_for_irq6(void)
{
	kernel_wait_cv(&irq6_condition);
}

typedef struct 
{
	uint8_t type;
	uint8_t sectors_per_track;
	uint8_t heads_per_cylinder;
	uint8_t num_tracks;
	size_t num_sectors;

	uint8_t drive_index;
	kernel_mutex mutex;
} floppy_drive;

static floppy_drive floppy_drives[2];
static size_t num_floppy_drives = 0;

static void floppy_set_drive_params(uint8_t type, floppy_drive* f)
{
	f->type = type;
	f->mutex.tas_lock = 0;
	f->mutex.ownerPID = -1;

	switch(type)
	{
		case 1:
			f->sectors_per_track = FLOPPY_360_SECTORS_PER_TRACK;
			f->heads_per_cylinder = FLOPPY_360_HEADS_PER_CYLINDER;
			f->num_tracks = FLOPPY_360_NUMBER_OF_TRACKS;
		break;
		case 2:
			f->sectors_per_track = FLOPPY_1_2_SECTORS_PER_TRACK;
			f->heads_per_cylinder = FLOPPY_1_2_HEADS_PER_CYLINDER;
			f->num_tracks = FLOPPY_1_2_NUMBER_OF_TRACKS;
		break;
		case 3:
			f->sectors_per_track = FLOPPY_720_SECTORS_PER_TRACK;
			f->heads_per_cylinder = FLOPPY_720_HEADS_PER_CYLINDER;
			f->num_tracks = FLOPPY_720_NUMBER_OF_TRACKS;
		break;
		case 4:
			f->sectors_per_track = FLOPPY_144_SECTORS_PER_TRACK;
			f->heads_per_cylinder = FLOPPY_144_HEADS_PER_CYLINDER;
			f->num_tracks = FLOPPY_144_NUMBER_OF_TRACKS;
		break;
		case 5:
			f->sectors_per_track = FLOPPY_288_SECTORS_PER_TRACK;
			f->heads_per_cylinder = FLOPPY_288_HEADS_PER_CYLINDER;
			f->num_tracks = FLOPPY_288_NUMBER_OF_TRACKS;
		break;
		default:
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
	
	for(int i = 0; i < num_floppy_drives; i++)
	{
		floppy_reset(i);
		floppy_drives[i].drive_index = i;

		filesystem_add_drive(&floppy_driver, &floppy_drives[i], 
							 FLOPPY_BYTES_PER_SECTOR, 
							 floppy_drives[i].num_sectors);
	}
}

static void lba_to_chs(floppy_drive* d, size_t lba, size_t *cylinder, size_t *head, size_t *sector)
{
	*sector = (lba % d->sectors_per_track) + 1;
	
	size_t track = lba / d->sectors_per_track;

	*cylinder = track / d->heads_per_cylinder;

	*head = track % d->heads_per_cylinder;
}

static void floppy_read_sectors(floppy_drive* d, size_t lba, uint8_t* buf, size_t num_sectors)
{
	if (lba > d->num_sectors)
	{
		printf("lba#%d is out of bounds for floppy read\n", lba);
		printf("lba max = %d\n", d->num_sectors);
		while (true);
	}

	size_t cylinder, head, sector;
	
	lba_to_chs(d, lba, &cylinder, &head, &sector);

	for(uint8_t i = 0; i < 5; i++)
	{		
		//printf("seeking to c=%d h=%d s=%d\n", cylinder, head, sector);
		if(!floppy_seek(d->drive_index, cylinder, head))
		{
			continue;
		}

		isa_dma_begin_transfer(0x02, 0x44, buf, FLOPPY_BYTES_PER_SECTOR * num_sectors);
		
		floppy_sendbyte(READ_DATA | MFM_BIT | MT_BIT);
		floppy_sendbyte(head << 2 | d->drive_index);
		floppy_sendbyte(cylinder);
		floppy_sendbyte(head);
		floppy_sendbyte(sector);
		floppy_sendbyte(2);  		// sector size = 128*2^size
		floppy_sendbyte(d->sectors_per_track); 		// number of sectors
		floppy_sendbyte(0x1b);      // 27 default gap3 value
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
			return; //read success!
		} 

		printf("read failure #%d\n", i);
		
		floppy_reset(d->drive_index);
	}
	
	printf("Catastrophic read failure at sector %d\n", lba);
}

static void floppy_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_blocks)
{
	floppy_drive* disk = (floppy_drive*)d->drv_impl_data;

	scoped_lock l{&disk->mutex};

	floppy_read_sectors(disk, block_number, buf, num_blocks);
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

	return -1; //read timeout 
}

static void floppy_motor_on(uint8_t driveNum)
{
	if(!motor_is_ready[driveNum]) 
	{
		outb(DIGITAL_OUTPUT_REGISTER, (0x10 << driveNum) | 0x0c | (driveNum & 0x03));
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
		floppy_sendbyte(head << 2 | driveNum);
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