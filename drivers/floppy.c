#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"
#include "sysclock.h"
#include "floppy.h"
#include "isa_dma.h"

#include "cmos.h"

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

void floppy_sendbyte(uint8_t byte);
uint8_t floppy_getbyte();
void floppy_reset(uint8_t driveNum);
void floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head);

enum floppy_registers
{
   DIGITAL_OUTPUT_REGISTER          = 0x3F2,
   MAIN_STATUS_REGISTER             = 0x3F4, // read-only
   DATA_FIFO                        = 0x3F5,
   CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
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

bool motor_is_ready[2] = {false, false};
volatile bool operation_complete = false;

INTERRUPT_HANDLER void floppy_irq_handler(interrupt_frame* r)
{
	operation_complete = true;
	send_eoi(6 + 32);
}

void wait_for_irq6(void)
{
	while(!operation_complete); // wait for irq6 to signal operation complete
	operation_complete = false;
}

struct floppy_drive
{
	uint8_t type;
	uint8_t sectorsPerTrack;
	uint8_t headsPerCylinder;
	uint8_t numTracks;

	uint8_t drive_index;
};

floppy_drive floppy_drives[2];

size_t num_floppy_drives = 0;

void floppy_set_drive_params(uint8_t type, floppy_drive* f)
{
	f->type = type;
	
	switch(type)
	{
		case 1:
			f->sectorsPerTrack = FLOPPY_360_SECTORS_PER_TRACK;
			f->headsPerCylinder = FLOPPY_360_HEADS_PER_CYLINDER;
			f->numTracks = FLOPPY_360_NUMBER_OF_TRACKS;
		break;
		case 2:
			f->sectorsPerTrack = FLOPPY_1_2_SECTORS_PER_TRACK;
			f->headsPerCylinder = FLOPPY_1_2_HEADS_PER_CYLINDER;
			f->numTracks = FLOPPY_1_2_NUMBER_OF_TRACKS;
		break;
		case 3:
			f->sectorsPerTrack = FLOPPY_720_SECTORS_PER_TRACK;
			f->headsPerCylinder = FLOPPY_720_HEADS_PER_CYLINDER;
			f->numTracks = FLOPPY_720_NUMBER_OF_TRACKS;
		break;
		case 4:
			f->sectorsPerTrack = FLOPPY_144_SECTORS_PER_TRACK;
			f->headsPerCylinder = FLOPPY_144_HEADS_PER_CYLINDER;
			f->numTracks = FLOPPY_144_NUMBER_OF_TRACKS;
		break;
		case 5:
			f->sectorsPerTrack = FLOPPY_288_SECTORS_PER_TRACK;
			f->headsPerCylinder = FLOPPY_288_HEADS_PER_CYLINDER;
			f->numTracks = FLOPPY_288_NUMBER_OF_TRACKS;
		break;
		default:
		return;
	}
	
	num_floppy_drives++;
}

void floppy_configure()
{
	floppy_sendbyte(CONFIGURE);
	floppy_sendbyte(0);
	floppy_sendbyte(1 << 6 | 0 << 5 | 1 << 4 | (8 - 1));
	floppy_sendbyte(8);
}

disk_driver floppy_driver = {
	floppy_read_blocks
};

void floppy_init()
{
	uint8_t floppyTypes = cmos_get_register(0x10);
	
	floppy_set_drive_params(floppyTypes & 0x0f, &floppy_drives[1]);
	floppy_set_drive_params(floppyTypes >> 4, &floppy_drives[0]);

	printf("%d floppy drives detected\n", num_floppy_drives);
	
	irq_install_handler(6, floppy_irq_handler);
	
	outb(DIGITAL_OUTPUT_REGISTER, 0x0C); //make sure interrupt is enabled
	
	floppy_configure();
	
	for(int i = 0; i < num_floppy_drives; i++)
	{
		floppy_reset(i);
		filesystem_add_drive(&floppy_driver, floppy_get_drive(i), 512);
	}
}

void lba_to_chs(floppy_drive* d, size_t lba, size_t *cylinder, size_t *head, size_t *sector)
{
	*sector = (lba % d->sectorsPerTrack) + 1;
	
	size_t track = lba / d->sectorsPerTrack;

	*cylinder = track / d->headsPerCylinder;

	*head = track % d->headsPerCylinder;
}

void floppy_read_sectors(floppy_drive* d, size_t lba, uint8_t* buf, size_t num_sectors)
{
	if (lba > d->numTracks * d->sectorsPerTrack* d->headsPerCylinder)
	{
		printf("lba#%d is out of bounds for floppy read\n", lba);
		while (true);
	}

	size_t cylinder, head, sector;
	
	lba_to_chs(d, lba, &cylinder, &head, &sector);

	for(uint8_t i = 0; i < 5; i++)
	{		
		//printf("seeking to c=%d h=%d s=%d\n", cylinder, head, sector);
		floppy_seek(d->drive_index, cylinder, head);
		
		isa_dma_begin_transfer(0x02, 0x44, buf, FLOPPY_BYTES_PER_SECTOR * num_sectors);
		
		floppy_sendbyte(READ_DATA | MFM_BIT | MT_BIT);
		floppy_sendbyte(head << 2 | d->drive_index);
		floppy_sendbyte(cylinder);
		floppy_sendbyte(head);
		floppy_sendbyte(sector);
		floppy_sendbyte(2);  		// sector size = 128*2^size
		floppy_sendbyte(18); 		// number of sectors
		floppy_sendbyte(0x1b);      // 27 default gap3 value
		floppy_sendbyte(0xff);      // default value for data length
		
		wait_for_irq6(); // wait for irq6 to signal operation complete
		
		//printf("irq6\n");
		
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

void floppy_read_blocks(const filesystem_drive* d, size_t block_number, uint8_t* buf, size_t num_blocks)
{
	floppy_read_sectors((floppy_drive*)d->dsk_impl_data, block_number, buf, num_blocks);
}

floppy_drive* floppy_get_drive(size_t index)
{
	return &floppy_drives[index];
}

//sendbyte() routine from intel manual
void floppy_sendbyte(uint8_t byte)
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
uint8_t floppy_getbyte()
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

void floppy_motor_on(uint8_t driveNum)
{
	if(!motor_is_ready[driveNum]) 
	{
		outb(DIGITAL_OUTPUT_REGISTER, (0x10 << driveNum) | 0x0c | (driveNum & 0x03));
		sysclock_sleep(500);			// wait 500ms for motor to spin up
		motor_is_ready[driveNum] = true;
	}
}

void floppy_motor_off(uint8_t driveNum)
{
	if(motor_is_ready[driveNum]) 
	{
		outb(DIGITAL_OUTPUT_REGISTER, 0x0c | (driveNum & 0x03)); 
		sysclock_sleep(2000);   // start motor kill countdown: 2s
		motor_is_ready[driveNum] = false;
	}
}

int floppy_calibrate(uint8_t driveNum) 
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


void floppy_reset(uint8_t driveNum)
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

void floppy_seek(uint8_t driveNum, uint8_t cylinder, uint8_t head)
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
		
		if(fdc_cylinder == cylinder) break;
	}
	
	//floppy_motor_off(driveNum);
}