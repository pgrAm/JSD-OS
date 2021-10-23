#include <stdio.h>

#include <kernel/locks.h>
#include <kernel/fs_driver.h>
extern "C" {
#include <drivers/portio.h>
#include <drivers/sysclock.h>
#include <kernel/interrupt.h>
};

#define ATA_SECTOR_SIZE 0x200
#define ATAPI_SECTOR_SIZE 0x800

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRDY    0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

#define ATA_ER_BBK      0x80    // Bad block
#define ATA_ER_UNC      0x40    // Uncorrectable data
#define ATA_ER_MC       0x20    // Media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // Media change request
#define ATA_ER_ABRT     0x04    // Command aborted
#define ATA_ER_TK0NF    0x02    // Track 0 not found
#define ATA_ER_AMNF     0x01    // No address mark

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_FIRST_DRIVE     0x00
#define ATA_SECOND_SRIVE   0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

struct ata_capability {
	enum {
		LBA = 0x200
	};
};

enum class ata_access_type {
	READ = 0,
	WRITE
};

enum class ata_error {
	NONE = 0,
	INVALID_DRIVE,
	INVALID_SEEK_POS,
	DEVICE_FAULT,
	GENERAL_ERROR,
	NO_ADDRESS_MARK,
	NO_MEDIA,
	COMMAND_ABORTED,
	NO_ID_MARK,
	DATA_ERROR,
	BAD_SECTORS,
	WRITE_PROTECTED,
	NOTHING_READ,
	DRQ_ERROR
};

enum class ata_drive_type {
	ATA,
	ATAPI
};

enum class ata_addressing_mode {
	LBA28,
	LBA48,
	CHS
};

struct IDEChannelRegisters {
	uint16_t	base;  // I/O Base.
	uint16_t	ctrl;  // Control Base
	uint16_t	bus_master; // Bus Master IDE
	uint8_t		no_interrupt;  // nIEN (No Interrupt);
} channels[2];

static kernel_cv irq_condition[2] = {{-1, 1}, {-1, 1}};

struct ata_drive
{
	bool	 exists;
	uint8_t  channel;		// 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t  drive;			// 0 (First Drive) or 1 (Second Drive). aka M/S
	ata_drive_type	 type;
	uint16_t	signature;		// drive signature
	uint16_t	capabilities;	// features supported
	uint32_t	command_sets;	// command sets supported.
	uint32_t	size;			// size in sectors.
	char		model[41];		// model string.
};

ata_drive ide_drives[4];

void ata_write(uint8_t channel, uint8_t reg, uint8_t data) {
	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
	if(reg < 0x08)
		outb(channels[channel].base + reg - 0x00, data);
	else if(reg < 0x0C)
		outb(channels[channel].base + reg - 0x06, data);
	else if(reg < 0x0E)
		outb(channels[channel].ctrl + reg - 0x0A, data);
	else if(reg < 0x16)
		outb(channels[channel].bus_master + reg - 0x0E, data);
	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}

uint8_t ata_read(uint8_t channel, uint8_t reg)
{
	uint8_t result = 0;
	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);
	if(reg < 0x08)
		result = inb(channels[channel].base + reg - 0x00);
	else if(reg < 0x0C)
		result = inb(channels[channel].base + reg - 0x06);
	else if(reg < 0x0E)
		result = inb(channels[channel].ctrl + reg - 0x0A);
	else if(reg < 0x16)
		result = inb(channels[channel].bus_master + reg - 0x0E);
	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
	return result;
}

void ata_read_buffer(uint8_t channel, uint8_t reg, uint8_t* buffer, size_t size)
{
	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].no_interrupt);

	if(reg < 0x08)
		insd(channels[channel].base + reg - 0x00, buffer, size);
	else if(reg < 0x0C)
		insd(channels[channel].base + reg - 0x06, buffer, size);
	else if(reg < 0x0E)
		insd(channels[channel].ctrl + reg - 0x0A, buffer, size);
	else if(reg < 0x16)
		insd(channels[channel].bus_master + reg - 0x0E, buffer, size);

	if(reg > 0x07 && reg < 0x0C)
		ata_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt);
}

void ata_delay400(uint8_t channel)
{
	//Delay 400 nanosecond for BSY to be set:
	for(size_t i = 0; i < 4; i++)
	{
		// Reading the ALTSTATUS port takes 100ns
		ata_read(channel, ATA_REG_ALTSTATUS);
	}
}

ata_error ata_poll(uint8_t channel, bool check_status = false)
{
	//wait for BSY to be set:
	ata_delay400(channel);

	// Wait for BSY to be zero.
	while(ata_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); 

	if(check_status)
	{
		uint8_t state = ata_read(channel, ATA_REG_STATUS); 

		if(state & ATA_SR_ERR) { return ata_error::GENERAL_ERROR; }
		if(state & ATA_SR_DF) { return ata_error::DEVICE_FAULT; }
		if((state & ATA_SR_DRQ) == 0) { return ata_error::DRQ_ERROR; }
	}

	return ata_error::NONE;
}

ata_error ata_print_error(ata_drive& drive, ata_error err)
{
	if(err == ata_error::NONE)
		return err;

	printf("IDE:");
	if(err == ata_error::DEVICE_FAULT)
	{
		printf("- Device Fault\n     ");
		err = ata_error::DEVICE_FAULT;
	}
	else if(err == ata_error::GENERAL_ERROR)
	{
		uint8_t st = ata_read(drive.channel, ATA_REG_ERROR);
		if(st & ATA_ER_AMNF)
		{
			printf("- No Address Mark Found\n     ");
			err = ata_error::NO_ADDRESS_MARK;
		}
		if(st & ATA_ER_TK0NF)
		{
			printf("- No Media or Media Error\n     ");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_ABRT)
		{
			printf("- Command Aborted\n     ");
			err = ata_error::COMMAND_ABORTED;
		}
		if(st & ATA_ER_MCR)
		{
			printf("- No Media or Media Error\n     ");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_IDNF)
		{
			printf("- ID mark not Found\n     ");
			err = ata_error::NO_ID_MARK;
		}
		if(st & ATA_ER_MC) {
			printf("- No Media or Media Error\n     ");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_UNC)
		{
			printf("- Uncorrectable Data Error\n     ");
			err = ata_error::DATA_ERROR;
		}
		if(st & ATA_ER_BBK)
		{
			printf("- Bad Sectors\n     ");
			err = ata_error::BAD_SECTORS;
		}
	}
	else if(err == ata_error::DRQ_ERROR)
	{
		printf("- Reads Nothing\n     ");
		err = ata_error::NOTHING_READ;
	}
	else if(err == ata_error::WRITE_PROTECTED)
	{
		printf("- Write Protected\n     ");
		err = ata_error::WRITE_PROTECTED;
	}

	auto p_s = drive.channel == 0 ? "Primary" : "Secondary";
	auto m_s = drive.channel == 0 ? "First": "Second";

	printf("- [%s %s] %s\n", p_s, m_s, drive.model);

	return err;
}

ata_error ata_access(ata_access_type access_type, ata_drive& drive, size_t lba,
					 uint8_t numsects, uint8_t* buffer)
{
	uint8_t		lba_io[6];
	uint32_t	channel = drive.channel;
	uint32_t	bus = channels[channel].base;
	uint32_t	words = 256; // Almost every ATA drive has a sector-size of 512-byte.

	ata_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt = 0x02);

	uint8_t head = 0;
	auto adress_mode = ata_addressing_mode::LBA28;

	if(lba >= 0x10000000)
	{
		// The drive must support LBA48 or this adress is incorrect
		// 32-bits are enough to address 2TB
		adress_mode = ata_addressing_mode::LBA48;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		// if size_t is 32 bit these will be 0
		lba_io[4] = (lba & 0x00FF00000000) >> 32;
		lba_io[5] = (lba & 0xFF0000000000) >> 40;
		head = 0; // Lower 4-bits of HDDEVSEL are not used here.
	}
	else if(drive.capabilities & ata_capability::LBA)
	{
		adress_mode = ata_addressing_mode::LBA28;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba & 0xF000000) >> 24;
	}
	else 
	{
		adress_mode = ata_addressing_mode::CHS;
		uint8_t sect = (lba % 63) + 1;
		uint16_t cyl = (lba + 1 - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect) % (16 * 63) / (63);
	}

	//TODO implement DMA transfers
	bool dma = false;

	// wait for the drive to be ready
	while(ata_read(channel, ATA_REG_STATUS) & ATA_SR_BSY);

	uint8_t ms_bit = drive.drive;
	if(adress_mode == ata_addressing_mode::CHS)
	{
		ata_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (ms_bit << 4) | head);
	}
	else
	{
		ata_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (ms_bit << 4) | head);
	}

	if(adress_mode == ata_addressing_mode::LBA48)
	{
		ata_write(channel, ATA_REG_SECCOUNT1, 0);
		ata_write(channel, ATA_REG_LBA3, lba_io[3]);
		ata_write(channel, ATA_REG_LBA4, lba_io[4]);
		ata_write(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ata_write(channel, ATA_REG_SECCOUNT0, numsects);
	ata_write(channel, ATA_REG_LBA0, lba_io[0]);
	ata_write(channel, ATA_REG_LBA1, lba_io[1]);
	ata_write(channel, ATA_REG_LBA2, lba_io[2]);

	if(dma)
	{
		//TODO implement DMA transfers
	}
	else
	{
		if(access_type == ata_access_type::READ)
		{
			uint8_t read_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
							ATA_CMD_READ_PIO_EXT : ATA_CMD_READ_PIO;

			ata_write(channel, ATA_REG_COMMAND, read_cmd);

			for(size_t i = 0; i < numsects; i++)
			{
				if(auto err = ata_poll(channel, true); err != ata_error::NONE)
				{
					return err; // Polling, set error and exit if there is.
				}
				insw(bus, buffer, words);
				buffer += (words * sizeof(uint16_t));
			}
		}
		else
		{
			uint8_t write_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
							 ATA_CMD_WRITE_PIO_EXT : ATA_CMD_WRITE_PIO;

			ata_write(channel, ATA_REG_COMMAND, write_cmd);

			for(size_t i = 0; i < numsects; i++)
			{
				ata_poll(channel);
				outsw(bus, buffer, words);
				buffer += (words * sizeof(uint16_t));
			}

			uint8_t flush_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
								ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH;

			ata_write(channel, ATA_REG_COMMAND, flush_cmd);
			ata_poll(channel);
		}
	}

	return ata_error::NONE; // Easy, isn't it?
}

void ide_wait_irq(size_t index)
{
	kernel_wait_cv(&irq_condition[index]);
}

static INTERRUPT_HANDLER void ata_irq_handler0(interrupt_frame* r)
{
	send_eoi(14 + 32);
	kernel_signal_cv(&irq_condition[0]);
}

static INTERRUPT_HANDLER void ata_irq_handler1(interrupt_frame* r)
{
	send_eoi(15 + 32);
	kernel_signal_cv(&irq_condition[1]);
}

ata_error ata_atapi_read(ata_drive& drive, uint32_t lba, uint8_t num_sectors, uint8_t* buffer)
{
	uint32_t channel = drive.channel;
	uint32_t ms_bit = drive.drive;
	uint32_t bus = channels[channel].base;
	uint32_t words = ATAPI_SECTOR_SIZE / sizeof(uint16_t);

	// enable IRQs
	ata_write(channel, ATA_REG_CONTROL, channels[channel].no_interrupt = 0x0);

	// select the drive:
	ata_write(channel, ATA_REG_HDDEVSEL, ms_bit << 4);

	// wait 400ns for select to complete:
	ata_delay400(channel);

	// use pio mode
	ata_write(channel, ATA_REG_FEATURES, 0); 

	// send the size of buffer:
	ata_write(channel, ATA_REG_LBA1, (words * sizeof(uint16_t)) & 0xFF);
	ata_write(channel, ATA_REG_LBA2, (words * sizeof(uint16_t)) >> 8);

	// send the packet command:
	ata_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);

	// Wait for the driver to finish or return an error code
	if(auto err = ata_poll(channel, true); err != ata_error::NONE)
	{
		return err;
	}

	// send the atapi packet
	uint8_t atapi_packet[12] = {
		ATAPI_CMD_READ,
		0x0,
		(uint8_t)((lba >> 24) & 0xFF),
		(uint8_t)((lba >> 16) & 0xFF),
		(uint8_t)((lba >> 8) & 0xFF),
		(uint8_t)((lba >> 0) & 0xFF),
		0x0, 0x0, 0x0,
		num_sectors,
		0x0, 0x0
	};
	outsw(bus, atapi_packet, sizeof(atapi_packet) / sizeof(uint16_t));

	// receive data:
	for(size_t i = 0; i < num_sectors; i++)
	{
		ide_wait_irq(channel);
		if(auto err = ata_poll(channel, 1); err != ata_error::NONE)
		{
			return err;
		}
		insw(bus, buffer, words);
		buffer += (words * sizeof(uint16_t));
	}

	ide_wait_irq(channel);

	// wait for BSY & DRQ to clear:
	while(ata_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));

	return ata_error::NONE;
}

ata_error ata_read_sectors(ata_drive& drive, uint8_t num_sectors, uint32_t lba, uint8_t* buffer)
{
	if(!drive.exists)
	{
		return ata_error::INVALID_DRIVE;
	}
	else if(((lba + num_sectors) > drive.size) && (drive.type == ata_drive_type::ATA))
	{
		return ata_error::INVALID_SEEK_POS;
	}
	else
	{
		auto err = ata_error::NONE;
		if(drive.type == ata_drive_type::ATA)
		{
			err = ata_access(ata_access_type::READ, drive, lba, num_sectors, buffer);
		}
		else if(drive.type == ata_drive_type::ATAPI)
		{
			for(size_t i = 0; i < num_sectors; i++)
			{
				err = ata_atapi_read(drive, lba + i, 1, buffer + (i * ATAPI_SECTOR_SIZE));
			}
		}
		return ata_print_error(drive, err);
	}
}

ata_error ata_write_sectors(ata_drive& drive, uint8_t num_sectors, uint32_t lba, uint8_t* buffer)
{
	if(!drive.exists)
	{
		return ata_error::INVALID_DRIVE;
	}
	else if(((lba + num_sectors) > drive.size) && (drive.type == ata_drive_type::ATA))
	{
		return ata_error::INVALID_SEEK_POS;
	}
	else
	{
		auto err = ata_error::NONE;
		if(drive.type == ata_drive_type::ATA)
		{
			err = ata_access(ata_access_type::WRITE, drive, lba, num_sectors, buffer);
		}
		else if(drive.type == ata_drive_type::ATAPI)
		{
			err = ata_error::WRITE_PROTECTED;
		}
		return ata_print_error(drive, err);
	}
}

static void ata_read_blocks(const filesystem_drive* fd,
							size_t block_number,
							uint8_t* buf,
							size_t num_blocks)
{
	ata_drive* d = (ata_drive*)fd->drv_impl_data;
	if(auto err = ata_read_sectors(*d, num_blocks, block_number, buf);
	   err != ata_error::NONE)
	{
		printf("error code %d\n", err);
	}
}

static disk_driver ata_driver = {
	ata_read_blocks,
	nullptr,
	nullptr
};

static bool ata_check_status(uint8_t channel)
{
	while(1)
	{
		uint8_t status = ata_read(channel, ATA_REG_STATUS);
		if((status & ATA_SR_ERR))
		{
			return false;
		}
		if(!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
			return true;
	}
}

void ata_initialize_drives(uint16_t base_port0, uint16_t base_port1, uint16_t base_port2,
						   uint16_t base_port3, uint16_t base_port4)
{
	//install irq handlers
	irq_install_handler(14, ata_irq_handler0);
	irq_install_handler(15, ata_irq_handler1);

	channels[0].base = base_port0;
	channels[0].ctrl = base_port1;
	channels[0].bus_master = base_port4;
	channels[1].base = base_port2;
	channels[1].ctrl = base_port3;
	channels[1].bus_master = base_port4 + 8;

	// disable IRQs
	ata_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ata_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	size_t count = 0;
	for(size_t channel = 0; channel < 2; channel++)
	{
		for(size_t index = 0; index < 2; index++)
		{
			auto& drive = ide_drives[count];
			drive.exists = false;

			// select drive.
			ata_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (index << 4));
			sysclock_sleep(1); // wait 1ms for selection

			// send ATA identify command:
			ata_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			sysclock_sleep(1);

			// poll drive, will return nonzero if it exists
			if(ata_read(channel, ATA_REG_STATUS) == 0) continue;

			// check for ATAPI device
			auto type = ata_drive_type::ATA;
			if(!ata_check_status(channel))
			{
				uint8_t cl = ata_read(channel, ATA_REG_LBA1);
				uint8_t ch = ata_read(channel, ATA_REG_LBA2);

				if((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96))
					type = ata_drive_type::ATAPI;
				else
					continue; // Unknown Type (may not be a device).

				ata_write(channel, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				sysclock_sleep(1);
			}

			// read ident space of the drive
			uint8_t ident_buf[ATAPI_SECTOR_SIZE];
			ata_read_buffer(channel, ATA_REG_DATA, ident_buf, 128);

			// read revice params
			drive.exists = true;
			drive.type = type;
			drive.channel = channel;
			drive.drive = index;
			drive.signature		= *((uint16_t*)(ident_buf + ATA_IDENT_DEVICETYPE));
			drive.capabilities	= *((uint16_t*)(ident_buf + ATA_IDENT_CAPABILITIES));
			drive.command_sets	= *((uint32_t*)(ident_buf + ATA_IDENT_COMMANDSETS));

			// get disk size
			if(drive.command_sets & (1 << 26)) // LBA48
			{
				drive.size = *((uint32_t*)(ident_buf + ATA_IDENT_MAX_LBA_EXT));
			}
			else //drive uses CHS or 28-bit Addressing:
			{
				drive.size = *((uint32_t*)(ident_buf + ATA_IDENT_MAX_LBA));
			}

			// read the model string
			for(size_t k = 0; k < 40; k += 2)
			{
				drive.model[k] = ident_buf[ATA_IDENT_MODEL + k + 1];
				drive.model[k + 1] = ident_buf[ATA_IDENT_MODEL + k];
			}
			drive.model[40] = '\0'; // terminate string.

			count++;
		}
	}

	for(int i = 0; i < 4; i++)
	{
		if(ide_drives[i].exists)
		{
			auto type = ide_drives[i].type == ata_drive_type::ATA 
						? "ATA" : "ATAPI";

			auto size_in_GB = ide_drives[i].size / 1024 / 1024 / 2;

			printf("\tFound %s Drive %dGB - %s\n", 
				   type, size_in_GB, ide_drives[i].model);

			filesystem_add_drive(&ata_driver, &ide_drives[i], 
								 ide_drives[i].type == ata_drive_type::ATA 
								 ? ATA_SECTOR_SIZE
								 : ATAPI_SECTOR_SIZE, ide_drives[i].size);
		}
	}
}

extern "C" int ata_init()
{
	ata_initialize_drives(0x1F0, 0x3F6, 0x170, 0x376, 0x000);

	return 0;
}