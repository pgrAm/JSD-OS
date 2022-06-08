#include <stdio.h>

#include <kernel/locks.h>
#include <kernel/filesystem/fs_driver.h>
#include <kernel/interrupt.h>
#include <kernel/sysclock.h>
#include <kernel/kassert.h>

#include <drivers/portio.h>

#include <drivers/pci.h>
#include <drivers/ata_cmd.h>

#include <vector>
#include <bit>

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
#define ATA_REG_CONTROL    0
#define ATA_REG_ALTSTATUS  0
#define ATA_REG_DEVADDRESS 0x0D

// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01

#define WORDS_PER_SECTOR 256

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

struct ata_channel {
	uint16_t	base;  // I/O Base.
	uint16_t	ctrl;  // Control Base
	uint16_t	bus_master; // Bus Master IDE
	uint8_t		no_interrupt;  // nIEN (No Interrupt);
	pci_device	pci_device;
	size_t		irq;
};

struct ata_drive
{
	bool	 exists;
	uint8_t  channel;		// 0 (Primary Channel) or 1 (Secondary Channel).
	uint8_t  drive;			// 0 (First Drive) or 1 (Second Drive). aka M/S
	ata_drive_type	 type;
	uint16_t	signature;		// drive signature
	uint16_t	capabilities;	// features supported
	uint32_t	command_sets;	// command sets supported.
	size_t		size;			// size in sectors.
	char		model[41];		// model string.
};

static ata_channel channels[2];
static ata_drive ide_drives[4];
static constinit sync::mutex irq_mtx[2];
static constinit sync::atomic_flag irq_flags[2];

template<size_t N, typename Func, typename Buf>
ata_error foreach_n_sectors(ata_drive& drive, size_t lba, size_t num_sectors,
							Buf* buffer, size_t sector_size,
							Func f) requires(std::has_single_bit(N + 1))
{
	while(num_sectors)
	{
		size_t sectors = num_sectors & N;
		if(auto err = f(drive, lba, (uint8_t)sectors, buffer);
		   err != ata_error::NONE)
			return err;
		lba += sectors;
		num_sectors -= sectors;
		buffer += sectors * sector_size;
	}
	return ata_error::NONE;
}

static void ata_wait_irq(size_t index)
{
	irq_flags[index].wait(false);
	irq_flags[index].clear();
}

static void ata_delay400(uint8_t channel)
{
	//Delay 400 nanoseconds by reading ALTSTATUS (100ns each)
	for(size_t i = 0; i < 4; i++)
	{
		inb(channels[channel].ctrl + ATA_REG_ALTSTATUS);
	}
}

static ata_error ata_poll(uint8_t channel, bool check_status = false)
{
	//wait for BSY to be set:
	ata_delay400(channel);

	uint16_t ctrl_port = channels[channel].ctrl;

	// Wait for BSY to be zero.
	while(inb(ctrl_port + ATA_REG_ALTSTATUS) & ATA_SR_BSY);

	if(check_status)
	{
		uint8_t state = inb(ctrl_port + ATA_REG_ALTSTATUS);

		if(state & ATA_SR_ERR) { return ata_error::GENERAL_ERROR; }
		if(state & ATA_SR_DF) { return ata_error::DEVICE_FAULT; }
		if((state & ATA_SR_DRQ) == 0) { return ata_error::DRQ_ERROR; }
	}

	return ata_error::NONE;
}

static ata_error ata_print_error(const ata_drive& drive, ata_error err)
{
	if(err == ata_error::NONE)
		return err;

	printf("ATA: ");
	if(err == ata_error::DEVICE_FAULT)
	{
		printf("Device Fault\n");
		err = ata_error::DEVICE_FAULT;
	}
	else if(err == ata_error::GENERAL_ERROR)
	{
		uint8_t st = inb(channels[drive.channel].base + ATA_REG_ERROR);
		if(st & ATA_ER_AMNF)
		{
			printf("No Address Mark Found\n");
			err = ata_error::NO_ADDRESS_MARK;
		}
		if(st & ATA_ER_TK0NF)
		{
			printf("No Media or Media Error\n");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_ABRT)
		{
			printf("Command Aborted\n");
			err = ata_error::COMMAND_ABORTED;
		}
		if(st & ATA_ER_MCR)
		{
			printf("No Media or Media Error\n");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_IDNF)
		{
			printf("ID mark not Found\n");
			err = ata_error::NO_ID_MARK;
		}
		if(st & ATA_ER_MC) {
			printf("No Media or Media Error\n");
			err = ata_error::NO_MEDIA;
		}
		if(st & ATA_ER_UNC)
		{
			printf("Uncorrectable Data Error\n");
			err = ata_error::DATA_ERROR;
		}
		if(st & ATA_ER_BBK)
		{
			printf("Bad Sectors\n");
			err = ata_error::BAD_SECTORS;
		}
	}
	else if(err == ata_error::DRQ_ERROR)
	{
		printf("Read Nothing\n");
		err = ata_error::NOTHING_READ;
	}
	else if(err == ata_error::WRITE_PROTECTED)
	{
		printf("Write Protected\n");
		err = ata_error::WRITE_PROTECTED;
	}

	printf("\t[%d, %d] %s\n", drive.channel, drive.drive, drive.model);

	return err;
}
static ata_addressing_mode ata_setup_transfer(ata_drive& drive, size_t lba, uint8_t numsects)
{
	uint8_t		lba_io[6];

	auto channel	   = drive.channel;
	uint16_t base_port = channels[channel].base;
	uint16_t crtl_port = channels[channel].ctrl;

	outb(crtl_port, channels[channel].no_interrupt = 0x00);

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
		uint8_t sect = uint8_t((lba % 63) + 1);
		uint16_t cyl = uint16_t((lba + 1 - sect) / (16 * 63));
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect) % (16 * 63) / (63);
	}

	// wait for the drive to be ready
	while(inb(base_port + ATA_REG_STATUS) & ATA_SR_BSY);

	uint8_t mode = (adress_mode == ata_addressing_mode::CHS) ? 0xA0u : 0xE0u;

	outb(base_port + ATA_REG_HDDEVSEL, mode | (uint8_t)(drive.drive << 4) | head);
	ata_delay400(channel);

	if(adress_mode == ata_addressing_mode::LBA48)
	{
		outb(base_port + ATA_REG_SECCOUNT1 - 6, 0);
		outb(base_port + ATA_REG_LBA3 - 6, lba_io[3]);
		outb(base_port + ATA_REG_LBA4 - 6, lba_io[4]);
		outb(base_port + ATA_REG_LBA5 - 6, lba_io[5]);
	}
	outb(base_port + ATA_REG_SECCOUNT0, numsects);
	outb(base_port + ATA_REG_LBA0, lba_io[0]);
	outb(base_port + ATA_REG_LBA1, lba_io[1]);
	outb(base_port + ATA_REG_LBA2, lba_io[2]);

	//TODO implement DMA transfers

	return adress_mode;
}

static ata_error ata_do_read(ata_drive& drive, size_t lba, uint8_t num_sectors,
							 uint8_t* buffer)
{
	k_assert(buffer);

	auto adress_mode = ata_setup_transfer(drive, lba, num_sectors);

	uint8_t read_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
		ATA_CMD_READ_PIO_EXT : ATA_CMD_READ_PIO;

	auto channel = drive.channel;
	uint16_t base_port = channels[channel].base;

	outb(base_port + ATA_REG_COMMAND, read_cmd);
	ata_delay400(channel);

	for(size_t i = 0; i < num_sectors; i++)
	{
		ata_wait_irq(channel);
		if(auto err = ata_poll(channel, true); err != ata_error::NONE)
		{
			return err;
		}
		insw(base_port, (uint16_t*)buffer, WORDS_PER_SECTOR);
		buffer += (WORDS_PER_SECTOR * sizeof(uint16_t));
	}

	return ata_error::NONE;
}

static ata_error ata_do_write(ata_drive& drive, size_t lba, uint8_t num_sectors,
							  const uint8_t* buffer)
{
	k_assert(buffer);

	auto adress_mode = ata_setup_transfer(drive, lba, num_sectors);

	uint8_t write_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
		ATA_CMD_WRITE_PIO_EXT : ATA_CMD_WRITE_PIO;

	auto channel = drive.channel;
	uint16_t base_port = channels[channel].base;

	outb(base_port + ATA_REG_COMMAND, write_cmd);
	ata_delay400(channel);

	for(size_t i = 0; i < num_sectors; i++)
	{
		//ata_wait_irq(channel);
		ata_poll(channel);
		outsw(base_port, (uint16_t*)buffer, WORDS_PER_SECTOR);
		buffer += (WORDS_PER_SECTOR * sizeof(uint16_t));
	}

	uint8_t flush_cmd = (adress_mode == ata_addressing_mode::LBA48) ?
		ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH;

	outb(base_port + ATA_REG_COMMAND, flush_cmd);
	ata_delay400(channel);
	ata_poll(channel);

	return ata_error::NONE;
}

static INTERRUPT_HANDLER void ata_irq_handler0(interrupt_frame* r)
{
	inb(channels[0].base + ATA_REG_STATUS);
	acknowledge_irq(14);
	irq_flags[0].test_and_set();
	irq_flags[0].notify_one();
}

static INTERRUPT_HANDLER void ata_irq_handler1(interrupt_frame* r)
{
	outb(0xA0, 0x0B);
	if(inb(0xA0) & (1 << 7))
	{
		inb(channels[1].base + ATA_REG_STATUS);
		acknowledge_irq(15);
		irq_flags[1].test_and_set();
		irq_flags[1].notify_one();
	}
	else
	{
		acknowledge_irq(7);
	}
}

static ata_error ata_atapi_read(ata_drive& drive, uint32_t lba, uint8_t num_sectors, uint8_t* buffer)
{
	auto channel = drive.channel;
	uint32_t num_words = ATAPI_SECTOR_SIZE / sizeof(uint16_t);

	irq_flags[channel].clear();

	uint16_t base_port = channels[channel].base;
	uint16_t ctrl_port = channels[channel].ctrl;

	// select the drive:
	outb(base_port + ATA_REG_HDDEVSEL, (uint8_t)(drive.drive << 4));
	// wait 400ns for select to complete:
	ata_delay400(channel);

	// enable IRQs
	outb(ctrl_port, channels[channel].no_interrupt = 0x0);

	// use pio mode
	outb(base_port + ATA_REG_FEATURES, 0);

	// send the size of buffer:
	outb(base_port + ATA_REG_LBA1, (uint8_t)((num_words * sizeof(uint16_t)) & 0xFF));
	outb(base_port + ATA_REG_LBA2, (uint8_t)((num_words * sizeof(uint16_t)) >> 8));

	// send the packet command:
	outb(base_port + ATA_REG_COMMAND, ATA_CMD_PACKET);

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

	irq_flags[channel].clear();
	outsw(base_port, (uint16_t*)atapi_packet,
		  sizeof(atapi_packet) / sizeof(uint16_t));

	// receive data:
	for(size_t i = 0; i < num_sectors; i++)
	{
		ata_wait_irq(channel);
		if(auto err = ata_poll(channel, true); err != ata_error::NONE)
		{
			return err;
		}
		insw(base_port, (uint16_t*)buffer, num_words);
		buffer += (num_words * sizeof(uint16_t));
	}

	ata_wait_irq(channel);

	// wait for BSY & DRQ to clear:
	while(inb(base_port + ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));

	return ata_error::NONE;
}

static ata_error ata_read_sectors(ata_drive& drive, size_t num_sectors,
								  uint32_t lba, uint8_t* buffer)
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
			foreach_n_sectors<255>(drive, lba, num_sectors, buffer,
								   ATA_SECTOR_SIZE, ata_do_read);
		}
		else if(drive.type == ata_drive_type::ATAPI)
		{
			foreach_n_sectors<255>(drive, lba, num_sectors, buffer,
								   ATAPI_SECTOR_SIZE, ata_atapi_read);
		}
		return ata_print_error(drive, err);
	}
}

static ata_error ata_write_sectors(ata_drive& drive, size_t num_sectors,
								   uint32_t lba, const uint8_t* buffer)
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
			err = foreach_n_sectors<255>(drive, lba, num_sectors, buffer,
										 ATA_SECTOR_SIZE, ata_do_write);
		}
		else if(drive.type == ata_drive_type::ATAPI)
		{
			err = ata_error::WRITE_PROTECTED;
		}
		return ata_print_error(drive, err);
	}
}

static void ata_write_blocks(void* drv_data, size_t block_number,
							 const uint8_t* buffer, size_t num_blocks)
{
	ata_drive* drive = (ata_drive*)drv_data;
	if(auto err = ata_write_sectors(*drive, num_blocks, block_number, buffer);
	   err != ata_error::NONE)
	{
		printf("error code %d\n", err);
	}
}

static void ata_read_blocks(void* drv_data, size_t block_number,
							uint8_t* buffer, size_t num_blocks)
{
	ata_drive* drive = (ata_drive*)drv_data;
	if(auto err = ata_read_sectors(*drive, num_blocks, block_number, buffer);
	   err != ata_error::NONE)
	{
		printf("error code %d\n", err);
	}
}

static disk_driver ata_driver = {
	ata_read_blocks,
	ata_write_blocks,
	nullptr,
	nullptr
};

static bool ata_check_status(uint8_t channel)
{
	while(true)
	{
		uint8_t status = inb(channels[channel].ctrl + ATA_REG_ALTSTATUS);
		if(status & ATA_SR_ERR)
		{
			return false;
		}
		if(!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ) && (status & ATA_SR_DRDY))
		{
			return true;
		}
	}
}

static void ata_wait_ready(uint8_t channel)
{
	uint16_t ctrl_port = channels[channel].ctrl;

	if(inb(ctrl_port + ATA_REG_ALTSTATUS) == 0) return;

	while(true)
	{
		uint8_t status = inb(ctrl_port + ATA_REG_ALTSTATUS);
		if(status & ATA_SR_ERR)
			break;
		if(status & (ATA_SR_BSY | ATA_SR_DRQ))
			continue;
		if(status & ATA_SR_DRDY)
			break;
	}
}

static size_t ata_initialize_drives(uint16_t base_port0, uint16_t base_port1,
									uint16_t base_port2, uint16_t base_port3,
									uint16_t base_port4, size_t irq = 0, 
									pci_device pci_device = {})
{
	channels[0].base = base_port0;
	channels[0].ctrl = base_port1;
	channels[0].bus_master = base_port4;
	channels[0].pci_device = pci_device;
	channels[0].irq = irq;
	channels[1].base = base_port2;
	channels[1].ctrl = base_port3;
	channels[1].bus_master = base_port4 + 8;
	channels[1].pci_device = pci_device;
	channels[1].irq = irq;

	for(int i = 0; i < 4; i++)
	{
		ide_drives[i].exists = false;
	}

	size_t num_drives = 0;
	for(uint8_t channel = 0; channel < 2; channel++)
	{
		uint16_t base_port = channels[channel].base;
		uint16_t ctrl_port = channels[channel].ctrl;

		outb(ctrl_port, 0x00);
		ata_delay400(channel);

		for(uint8_t index = 0; index < 2; index++)
		{
			auto& drive = ide_drives[num_drives];

			//select the drive
			outb(base_port + ATA_REG_HDDEVSEL, 0xA0 | (uint8_t)(index << 4));
			ata_delay400(channel);

			ata_wait_ready(channel);

			//clear these regs before IDENTIFY according to spec
			outb(base_port + ATA_REG_SECCOUNT0, 0x00);
			outb(base_port + ATA_REG_LBA0, 0x00);
			outb(base_port + ATA_REG_LBA1, 0x00);
			outb(base_port + ATA_REG_LBA2, 0x00);

			//send the IDENTIFY command
			outb(base_port + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			ata_delay400(channel);

			if(inb(base_port + ATA_REG_STATUS) == 0) { continue; }

			auto status = ata_check_status(channel);

			uint8_t cl = inb(base_port + ATA_REG_LBA1);
			uint8_t ch = inb(base_port + ATA_REG_LBA2);

			auto type = ata_drive_type::ATA;

			//check for ATAPI device 
			if((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96))
			{
				ata_wait_ready(channel);

				type = ata_drive_type::ATAPI;
				outb(base_port + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);

				if(auto err = ata_poll(channel, true); err != ata_error::NONE)
				{
					ata_print_error(drive, ata_error::GENERAL_ERROR);
				}
			}
			else if(!status || !((cl == 0 && ch == 0) || (cl == 0x3c && ch == 0xc3)))
			{
				//no usable drive here
				continue;
			}

			//read ident result
			uint8_t ident_buf[256*sizeof(uint16_t)];
			insw(base_port + ATA_REG_DATA, (uint16_t*)ident_buf, 256);

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
			else if(drive.capabilities & ata_capability::LBA)
			{
				drive.size = *((uint32_t*)(ident_buf + ATA_IDENT_MAX_LBA));
			}
			else //must use CHS
			{
				auto cylinders	= *((uint16_t*)(ident_buf + ATA_IDENT_CYLINDERS));
				auto heads		= *((uint16_t*)(ident_buf + ATA_IDENT_HEADS));
				auto sectors	= *((uint16_t*)(ident_buf + ATA_IDENT_SECTORS));

				drive.size = cylinders * heads * sectors;
			}

			// read the model string
			for(size_t k = 0; k < 40; k += 2)
			{
				drive.model[k + 0] = (char)ident_buf[ATA_IDENT_MODEL + k + 1];
				drive.model[k + 1] = (char)ident_buf[ATA_IDENT_MODEL + k + 0];
			}
			drive.model[40] = '\0'; // terminate string.

			num_drives++;
		}
	}


	for(int i = 0; i < 4; i++)
	{
		if(ide_drives[i].exists)
		{
			auto type = ide_drives[i].type == ata_drive_type::ATA
				? "ATA" : "ATAPI";

			auto size_in_GB = ide_drives[i].size / 1024 / 1024 / 2;

			printf("\t[%d, %d] Found %s Drive %dGB - %s\n",
				   ide_drives[i].channel, ide_drives[i].drive, type, size_in_GB, ide_drives[i].model);

			filesystem_add_drive(&ata_driver, &ide_drives[i],
								 ide_drives[i].type == ata_drive_type::ATA
								 ? ATA_SECTOR_SIZE
								 : ATAPI_SECTOR_SIZE, ide_drives[i].size);
		}
	}

	inb(channels[0].base + ATA_REG_STATUS);
	inb(channels[1].base + ATA_REG_STATUS);

	return num_drives;
}

static INTERRUPT_HANDLER void ata_irq_handler(interrupt_frame* r)
{

	/*auto status = pci_read<uint16_t>(channels[0].pci_device, PCI_STATUS);

	if(!(status & (1 << 3)))
	{
		//printf(".");
		return;
	}

	pci_write<uint16_t>(channels[0].pci_device, PCI_STATUS, status & ~(1 << 3));*/

	//printf("x-");

	/*
	auto bm_status = inb(channels[0].bus_master + 0x2);
	if(bm_status & 0x04)
	{
		outb(channels[0].bus_master + 0x2, 0x04);
	}*/

	inb(channels[0].base + ATA_REG_STATUS);
	inb(channels[1].base + ATA_REG_STATUS);
	irq_flags[0].test_and_set();
	irq_flags[1].test_and_set();
	irq_flags[0].notify_one();
	irq_flags[1].notify_one();

	//outb(channels[0].bus_master + 0x2, inb(channels[0].bus_master + 0x2) | 4);

	acknowledge_irq((uint8_t)channels[0].irq);
}

extern "C" int ata_init()
{
	std::vector<pci_device> devices;

	sysclock_sleep(1, MILLISECONDS);

	pci_device_by_class(
		[](pci_device device, uint16_t vendorid, uint16_t deviceid,
		   void* extra)
		{
			((std::vector<pci_device>*)extra)->push_back(device);
		}, 0x01, 0x01, &devices);

	//install irq handlers
	irq_install_handler(14, ata_irq_handler0);
	irq_install_handler(15, ata_irq_handler1);
	irq_enable(14, true);
	irq_enable(15, true);

	for(auto device : devices)
	{
		//look for a troublesome ICH7 chipset and set it to legacy mode
		if(pci_read<uint16_t>(device, PCI_VENDOR_ID) == 0x8086 &&
		   pci_read<uint16_t>(device, PCI_DEVICE_ID) == 0x27C0)
		{
			if(auto pif = pci_read<uint8_t>(device, PCI_PROG_IF); (pif & 0x0A) == 0x0A)
			{
				pci_write<uint8_t>(device, PCI_PROG_IF, 0x8A);
				pci_write<uint32_t>(device, PCI_BAR0, 1);
				pci_write<uint32_t>(device, PCI_BAR1, 1);
				pci_write<uint32_t>(device, PCI_BAR2, 1);
				pci_write<uint32_t>(device, PCI_BAR3, 1);
				pci_write<uint8_t>(device, PCI_INTERRUPT_LINE, 0);
			}
		}

		sysclock_sleep(1, MILLISECONDS);

		auto bar0 = pci_read<uint32_t>(device, PCI_BAR0);
		auto bar1 = pci_read<uint32_t>(device, PCI_BAR1);
		auto bar2 = pci_read<uint32_t>(device, PCI_BAR2);
		auto bar3 = pci_read<uint32_t>(device, PCI_BAR3);
		auto bar4 = pci_read<uint32_t>(device, PCI_BAR4);

		if(bar0 == 0 || bar0 == 1)
			bar0 = 0x1F0;
		else
			bar0 &= ~1u;

		if(bar1 == 0 || bar1 == 1)
			bar1 = 0x3F6u;
		else
			bar1 &= ~1u;

		if(bar2 == 0 || bar2 == 1)
			bar2 = 0x170u;
		else
			bar2 &= ~1u;

		if(bar3 == 0 || bar3 == 1)
			bar3 = 0x376u;
		else
			bar3 &= ~1u;

		auto interrupt_line = pci_read<uint8_t>(device, PCI_INTERRUPT_LINE);

		if(interrupt_line != 0)
		{
			irq_install_handler(interrupt_line, ata_irq_handler);
		}
		if(!ata_initialize_drives((uint16_t)bar0, (uint16_t)bar1,
								  (uint16_t)bar2, (uint16_t)bar3,
								  (uint16_t)bar4, interrupt_line, device))
		{
			//disable the PCI device if no drives found
			pci_write<uint16_t>(device, PCI_COMMAND,
								(pci_read<uint16_t>(device, PCI_COMMAND) & ~0x03) | (1 << 10));
		}
	}
	
	if(devices.empty())
	{
		ata_initialize_drives(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
	}

	return 0;
}