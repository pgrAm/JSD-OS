#include <vector>
#include <stdint.h>
#include <stdio.h>

#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/kassert.h>
#include <kernel/filesystem/fs_driver.h>
#include <drivers/ata_cmd.h>
#include <drivers/pci.h>

#define HBA_PxIS_TFES   (1 << 30) // TFES - Task File Error Status

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

#define SATA_SIG_ATA    0x00000101  // SATA drive
#define SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define SATA_SIG_PM		0x96690101  // Port multiplier

#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1

enum fis_type : uint8_t
{
	FIS_TYPE_REG_H2D = 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H = 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT = 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP = 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA = 0x46,		// Data FIS - bidirectional
	FIS_TYPE_BIST = 0x58,		// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP = 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS = 0xA1,	// Set device bits FIS - device to host
};

struct __attribute__((packed)) fis_h2d
{
	fis_type type;

	uint8_t  port_mul	: 4; // Port multiplier
	uint8_t  rsv0		: 3; // Reserved
	uint8_t  cmd_bit	: 1; // 1: Command, 0: Control

	uint8_t  command;		// Command register
	uint8_t  feat_lo;		// Feature register, 7:0

	uint8_t  lba0;			// LBA low register, 7:0
	uint8_t  lba1;			// LBA mid register, 15:8
	uint8_t  lba2;			// LBA high register, 23:16
	uint8_t  device;		// Device register

	uint8_t  lba3;			// LBA register, 31:24
	uint8_t  lba4;			// LBA register, 39:32
	uint8_t  lba5;			// LBA register, 47:40
	uint8_t  feat_hi;		// Feature register, 15:8

	uint16_t count;			// Count register
	uint8_t  icc;			// Isochronous command completion
	uint8_t  control;		// Control register

	uint8_t  rsv1[4];	// Reserved
};

struct __attribute__((packed)) fis_d2h
{
	fis_type type;

	uint8_t  port_mul	: 4; // port multiplier
	uint8_t  rsv0		: 2; // Reserved
	uint8_t  int_bit	: 1; // Interrupt bit
	uint8_t  rsv1		: 1; // Reserved

	uint8_t  status;      // Status register
	uint8_t  error;       // Error register

	uint8_t  lba0;        // LBA low register, 7:0
	uint8_t  lba1;        // LBA mid register, 15:8
	uint8_t  lba2;        // LBA high register, 23:16
	uint8_t  device;      // Device register

	uint8_t  lba3;        // LBA register, 31:24
	uint8_t  lba4;        // LBA register, 39:32
	uint8_t  lba5;        // LBA register, 47:40
	uint8_t  rsv2;        // Reserved

	uint16_t count;		  // Count register
	uint8_t  rsv3[2];     // Reserved

	uint8_t  rsv4[4];     // Reserved
};

struct __attribute__((packed)) fis_data
{
	fis_type type;
	uint8_t  port_mul	: 4; // port multiplier & flags
	uint8_t  rsv0		: 4;

	uint8_t  rsv1[2];	// Reserved

	uint32_t data[1];	// Payload
};

struct __attribute__((packed)) fis_pio
{
	fis_type type;

	uint8_t  port_mul	: 4; // port multiplier & flags
	uint8_t  rsv0		: 1; // Reserved
	uint8_t  data_bit	: 1; // Data transfer direction, 1 - device to host
	uint8_t  int_bit	: 1; // Interrupt bit
	uint8_t  rsv1		: 1;

	uint8_t  status;	// Status register
	uint8_t  error;		// Error register

	uint8_t  lba0;		// LBA low register, 7:0
	uint8_t  lba1;		// LBA mid register, 15:8
	uint8_t  lba2;		// LBA high register, 23:16
	uint8_t  device;	// Device register

	uint8_t  lba3;		// LBA register, 31:24
	uint8_t  lba4;		// LBA register, 39:32
	uint8_t  lba5;		// LBA register, 47:40
	uint8_t  rsv2;		// Reserved

	uint16_t count;		  // Count register
	uint8_t  rsv3;		// Reserved
	uint8_t  e_status;	// New value of status register

	uint16_t tc;		// Transfer count
	uint8_t  rsv4[2];	// Reserved
};

struct __attribute__((packed)) fis_dma
{
	fis_type type;
	uint8_t  port_mul;		// port multiplier & flags
	uint8_t  rsv0[2];		// Reserved

	uint64_t DMAbufferID;	//DMA Buffer Identifier
	uint32_t rsv1;			//Reserved
	uint32_t DMAbufOffset;	//Byte offset into buffer. First 2 bits must be 0
	uint32_t TransferCount;	//Number of bytes to transfer. Bit 0 must be 0
	uint32_t rsv2;			//Reserved
};

volatile struct __attribute__((packed)) hba_port
{
	uint64_t clb;		// 0x00, command list base address, 1K-byte aligned
	uint64_t fb;		// 0x08, FIS base address, 256-byte aligned
	uint32_t is;		// 0x10, interrupt status
	uint32_t ie;		// 0x14, interrupt enable
	uint32_t cmd;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t tfd;		// 0x20, task file data
	uint32_t sig;		// 0x24, signature
	uint32_t ssts;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t serr;		// 0x30, SATA error (SCR1:SError)
	uint32_t sact;		// 0x34, SATA active (SCR3:SActive)
	uint32_t ci;		// 0x38, command issue
	uint32_t sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fbs;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
};

volatile struct __attribute__((packed)) hba_mem
{
	// 0x00 - 0x2B, Generic Host Control
	uint32_t cap;		// 0x00, Host capability
	uint32_t ghc;		// 0x04, Global host control
	uint32_t is;		// 0x08, Interrupt status
	uint32_t pi;		// 0x0C, Port implemented
	uint32_t vs;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;	// 0x1C, Enclosure management location
	uint32_t em_ctl;	// 0x20, Enclosure management control
	uint32_t cap2;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t rsv[0xA0 - 0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t vendor[0x100 - 0xA0];

	// 0x100 - 0x10FF, Port control registers
	hba_port ports[1];	// 1 ~ 32
};


struct __attribute__((packed)) hba_cmd_hdr
{
	uint8_t  fis_len	: 5;	// Command FIS length in DWORDS, 2 ~ 16
	uint8_t  atapi		: 1;	// ATAPI
	uint8_t  write_bit	: 1;	// Write, 1: H2D, 0: D2H
	uint8_t  preftc_bit : 1;	// Prefetchable

	uint8_t  reset_bit	: 1;	// Reset
	uint8_t  bist_bit	: 1;	// BIST
	uint8_t  clear_bit	: 1;	// Clear busy upon R_OK
	uint8_t  rsv0		: 1;	// Reserved
	uint8_t  port_mul	: 4;	// Port multiplier port

	uint16_t num_prdts;		// Physical region descriptor table length in entries

	volatile uint32_t prdbc; // Physical region descriptor byte count transferred

	uint64_t cmd_tbl_addr;	// Command table descriptor base address

	uint32_t rsv1[4];	// Reserved
};

struct __attribute__((packed)) prdt_entry
{
	uint64_t data_addr;		// Data base address
	uint32_t rsv0;			// Reserved

	uint32_t data_size : 22;// Byte count, 4M max
	uint32_t rsv1 : 9;		// Reserved
	uint32_t int_bit : 1;	// Interrupt on completion
};

struct __attribute__((packed)) hba_cmd_tbl
{
	uint8_t		cfis[64];	// Command FIS
	uint8_t		acmd[16];	// ATAPI command, 12 or 16 bytes
	uint8_t		rsv0[48];	// Reserved
	prdt_entry	prdt[1];	// Physical region descriptor table entries, 0 ~ 65535
};

enum class ahci_drive_type 
{
	SATA,
	SATAPI,
	SEMB,
	PORT_MUL,
	UNKNOWN
};

struct ahci_controller
{
	size_t num_cmd_slots;
	hba_mem* abar;
	uintptr_t phys_base;
	uintptr_t virt_base;

	template<typename T> T* get_addr(T* addr)
	{
		return (T*)(virt_base + ((uintptr_t)addr) - phys_base);
	}
};

struct ahci_drive
{
	ahci_controller*	controller;
	hba_port*			port;
	ahci_drive_type		type;
	size_t				size;			// size in sectors.
	char				model[41];		// model string.
};

std::vector<ahci_controller*>* ahci_controllers;
std::vector<ahci_drive*>* ahci_drives;

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

// Start command engine
void ahci_start_cmd(hba_port* port)
{
	// Wait until CR (bit15) is cleared
	while(port->cmd & HBA_PxCMD_CR);

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

// Stop command engine
void ahci_stop_cmd(hba_port* port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while(1)
	{
		if(port->cmd & HBA_PxCMD_FR)
			continue;
		if(port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

void port_rebase(ahci_controller& controller, hba_port* port, size_t port_num)
{
	ahci_stop_cmd(port);	// Stop command engine

	//auto ahci_base = physical_memory_allocate(296*1024, PAGE_SIZE);

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	size_t cmd_offset = (32 << 10) + (port_num << 8);

	port->clb = controller.phys_base + cmd_offset;
	memset((void*)(controller.virt_base + cmd_offset), 0, 1024);

	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	size_t fis_offset = (32 << 10) + (port_num << 8);

	port->fb = controller.phys_base + fis_offset;
	memset((void*)(controller.virt_base + fis_offset), 0, 256);

	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	hba_cmd_hdr* hdr = (hba_cmd_hdr*)(port->clb);
	for(int i = 0; i < 32; i++)
	{
		hdr[i].num_prdts = 8;	// 8 prdt entries per command table
								// 256 bytes per command table, 64+16+48+16*8

		// Command table offset: 40K + 8K*portno + cmdheader_index*256

		size_t tbl_offset = (40 << 10) + (port_num << 13) + (i << 8);

		hdr[i].cmd_tbl_addr = controller.phys_base + tbl_offset;
		memset((void*)(controller.virt_base + tbl_offset), 0, 256);
	}

	ahci_start_cmd(port);	// Start command engine
}

// Find a free command list slot
size_t find_cmdslot(const ahci_drive& drive)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots	= (drive.port->sact | drive.port->ci);
	//size_t num_slots = (drive.abar->cap & 0x0f00) >> 8; // Bit 8-12

	for(size_t i = 0; i < drive.controller->num_cmd_slots; i++)
	{
		if((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	printf("Cannot find free command list entry\n");
	return drive.controller->num_cmd_slots;
}

// Check device type
static ahci_drive_type ahci_get_drive_type(hba_port* port)
{
	uint32_t ssts = port->ssts;

	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;

	if(det != HBA_PORT_DET_PRESENT)	// Check drive status
		return ahci_drive_type::UNKNOWN;
	if(ipm != HBA_PORT_IPM_ACTIVE)
		return ahci_drive_type::UNKNOWN;

	switch(port->sig)
	{
	case SATA_SIG_ATAPI:
		return ahci_drive_type::SATAPI;
	case SATA_SIG_SEMB:
		return ahci_drive_type::SEMB;
	case SATA_SIG_PM:
		return ahci_drive_type::PORT_MUL;
	default:
		return ahci_drive_type::SATA;
	}
}

bool ahci_wait_complete(hba_port* port, size_t slot)
{
	// Wait for completion
	while(1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if((port->ci & (1 << slot)) == 0)
			break;
		if(port->is & HBA_PxIS_TFES)	// Task file error
		{
			return false;
		}
	}

	// Check again
	if(port->is & HBA_PxIS_TFES)
	{
		return false;
	}

	return true;
}

bool ahci_wait_busy(hba_port* port)
{
	int spin = 0; // Spin lock timeout counter
	// The below loop waits until the port is no longer busy before issuing a new command
	while((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if(spin == 1000000)
	{
		return false;
	}

	return true;
}

bool ahci_identify(const ahci_drive& drive, uint8_t* buffer)
{
	auto port = drive.port;

	port->is = (uint32_t)-1;		// Clear pending interrupt bits
	
	size_t slot = find_cmdslot(drive);
	if(slot >= drive.controller->num_cmd_slots)
	{
		return false;
	}

	uintptr_t phys_addr = memmanager_get_physical((uintptr_t)buffer);

	hba_cmd_hdr& header = drive.controller->get_addr((hba_cmd_hdr*)port->clb)[slot];

	header.write_bit = 0;
	header.fis_len = sizeof(fis_h2d) / sizeof(uint32_t);
	header.num_prdts = 1;

	//printf("%X\n", drive.controller->virt_base);

	hba_cmd_tbl* cmd_tbl = drive.controller->get_addr((hba_cmd_tbl*)header.cmd_tbl_addr);
	memset(cmd_tbl, 0, sizeof(hba_cmd_tbl));

	cmd_tbl->prdt[0].data_addr = (uint64_t)phys_addr;
	cmd_tbl->prdt[0].data_size = 512; // 512 bytes for IDENTIFY
	cmd_tbl->prdt[0].int_bit = 1;

	// Setup command
	fis_h2d* cmd = (fis_h2d*)(&cmd_tbl->cfis);

	cmd->type = FIS_TYPE_REG_H2D;
	cmd->cmd_bit = 1;	// Command
	cmd->command = ATA_CMD_IDENTIFY;

	cmd->device = 0xA0;	//CHS for identify

	if(!ahci_wait_busy(port))
	{
		printf("Port is hung\n");
		return false;
	}

	port->ci = 1 << slot;	// Issue command

	if(!ahci_wait_complete(port, slot))
	{
		printf("Disk error\n");
		return false;
	}

	k_assert(false);

	return true;
}

bool ahci_access(bool write, const ahci_drive& drive, size_t lba, uint8_t num_sectors, uint8_t* buffer)
{
	auto port = drive.port;

	port->is = (uint32_t)-1;		// Clear pending interrupt bits
	size_t slot = find_cmdslot(drive);
	if(slot >= drive.controller->num_cmd_slots)
	{
		return false;
	}

	uintptr_t phys_addr = memmanager_get_physical((uintptr_t)buffer);

	hba_cmd_hdr& header = drive.controller->get_addr((hba_cmd_hdr*)port->clb)[slot];

	size_t num_prdts = ((num_sectors - 1) / 16) + 1;

	header.write_bit = write ? 1 : 0;
	header.fis_len = sizeof(fis_h2d) / sizeof(uint32_t);	
	header.num_prdts = (uint16_t)num_prdts;

	hba_cmd_tbl* cmd_tbl = drive.controller->get_addr((hba_cmd_tbl*)header.cmd_tbl_addr);
	memset(cmd_tbl, 0, sizeof(hba_cmd_tbl) + (num_prdts - 1) * sizeof(prdt_entry));

	// 8K bytes (16 sectors) per PRDT
	for(int i = 0; i < num_prdts - 1; i++)
	{
		cmd_tbl->prdt[i].data_addr = (uint64_t)phys_addr;
		cmd_tbl->prdt[i].data_size = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmd_tbl->prdt[i].int_bit = 1;
		phys_addr += 4 * 1024;	// 4K words
		num_sectors -= 16;	// 16 sectors
	}
	// Last entry
	cmd_tbl->prdt[num_prdts - 1].data_addr = (uint64_t)phys_addr;
	cmd_tbl->prdt[num_prdts - 1].data_size = (num_sectors << 9) - 1;	// 512 bytes per sector
	cmd_tbl->prdt[num_prdts - 1].int_bit = 1;

	// Setup command
	fis_h2d* cmd = (fis_h2d*)(&cmd_tbl->cfis);

	cmd->type = FIS_TYPE_REG_H2D;
	cmd->cmd_bit = 1;	// Command
	cmd->command = write ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;

	cmd->device = 1 << 6;	// LBA mode

	cmd->lba0 = (lba & 0x000000FF) >> 0;
	cmd->lba1 = (lba & 0x0000FF00) >> 8;
	cmd->lba2 = (lba & 0x00FF0000) >> 16;
	cmd->lba3 = (lba & 0xFF000000) >> 24;
	cmd->lba4 = (lba & 0x00FF00000000) >> 32;
	cmd->lba5 = (lba & 0xFF0000000000) >> 40;

	cmd->count = num_sectors;

	if(!ahci_wait_busy(port))
	{
		printf("Port is hung\n");
	}

	port->ci = 1 << slot;	// Issue command

	if(!ahci_wait_complete(port, slot))
	{
		printf("Disk error\n");
		return false;
	}

	return true;
}

static void ahci_read_blocks(const filesystem_drive* fd,
							size_t block_number,
							uint8_t* buf,
							size_t num_blocks)
{
	ahci_drive* d = (ahci_drive*)fd->drv_impl_data;
	ahci_access(false, *d, num_blocks, block_number, buf);
}

static disk_driver ahci_driver = {
	ahci_read_blocks,
	nullptr,
	nullptr
};

void probe_port(ahci_controller& controller)
{
	// Search disk in implemented ports
	uint32_t pi = controller.abar->pi;
	for(size_t i = 0; i < 32; i++)
	{
		if(pi & 0x01)
		{
			auto port = &controller.abar->ports[i];

			auto type = ahci_get_drive_type(port);

			switch(type)
			{
			case ahci_drive_type::SATA:
				printf("SATA drive found at port %d\n", i);
				break;
			case ahci_drive_type::SATAPI:
				printf("SATAPI drive found at port %d\n", i);
				break;
			case ahci_drive_type::SEMB:
				printf("SEMB drive found at port %d\n", i);
				break;
			case ahci_drive_type::PORT_MUL:
				printf("Port Multiplier drive found at port %d\n", i);
				break;
			default:
				printf("No drive found at port %d\n", i);
				break;
			}

			port_rebase(controller, port, i);

			/*ahci_drive* new_drive = new ahci_drive
			{
				.controller = &controller,
				.port = port,
				.type = type,
				//.num_cmd_slots = (controller.abar->cap & 0x0f00) >> 8,
			};

			uint8_t ident_buf[256 * sizeof(uint16_t)];
			if(ahci_identify(*new_drive, ident_buf))
			{
				new_drive->size = *((uint32_t*)(ident_buf + ATA_IDENT_MAX_LBA_EXT));

				// read the model string
				for(size_t k = 0; k < 40; k += 2)
				{
					new_drive->model[k] = ident_buf[ATA_IDENT_MODEL + k + 1];
					new_drive->model[k + 1] = ident_buf[ATA_IDENT_MODEL + k];
				}
				new_drive->model[40] = '\0'; // terminate string.

				ahci_drives->push_back(new_drive);
			}
			else
			{
				delete new_drive;
			}*/
		}
		pi >>= 1;
	}
}

extern "C" int ahci_init(directory_stream * cwd)
{
	ahci_controllers = new std::vector<ahci_controller*>{};
	ahci_drives = new std::vector<ahci_drive*>{};

	pci_device_by_class([](pci_device device, uint16_t vendorid, uint16_t deviceid,
						   void* extra)
	{

		auto d_class = pci_read<uint8_t>(device, PCI_CLASS);
		auto d_sclass = pci_read<uint8_t>(device, PCI_SUBCLASS);

		printf("C=%X, SC=%X\n", d_class, d_sclass);

		if(d_class != 0x01)
			return;

		if(d_sclass == 0x01)
		{
			if(pci_read<uint16_t>(device, PCI_VENDOR_ID) == 0x8086 &&
			   pci_read<uint16_t>(device, PCI_DEVICE_ID) == 0x27C0)
			{
				pci_write<uint8_t>(device, 0x90, 0x40);
			}
			else
			{
				return;
			}
		}
		else if(d_sclass != 0x06)
		{
			return;
		}

		auto bar5 = pci_read<uint32_t>(device, PCI_BAR5);
		if(!bar5)
		{
			bar5 = physical_memory_allocate(PAGE_SIZE, PAGE_SIZE);
			pci_write<uint32_t>(device, PCI_BAR5, bar5);
		}

		auto v_addr = memmanager_map_to_new_pages(	bar5,
													memmanager_minimum_pages(sizeof(hba_mem)), 
													PAGE_PRESENT | PAGE_RW);
		auto abar = (hba_mem*)v_addr;
		

		auto mem_size = 296 * 1024;
		auto phys_base = physical_memory_allocate(mem_size, PAGE_SIZE);
		auto virt_base = memmanager_map_to_new_pages(phys_base, 
													 memmanager_minimum_pages(mem_size),
													 PAGE_PRESENT | PAGE_RW);

		printf("%X\n", bar5);

		ahci_controller* c = new ahci_controller
		{
			.abar = abar,
			.num_cmd_slots = (abar->cap & 0x0f00) >> 8,
			.phys_base = phys_base,
			.virt_base = (uintptr_t)virt_base
		};

		probe_port(*c);

		ahci_controllers->push_back(c);

	}, 0xFF, 0xFF, nullptr);

	for(auto&& drive : *ahci_drives)
	{
		auto type = drive->type == ahci_drive_type::SATA
			? "SATA" : "SATAPI";

		auto size_in_GB = drive->size / 1024 / 1024 / 2;

		printf("\tFound %s Drive %dGB - %s\n",
			   type, size_in_GB, drive->model);

		filesystem_add_drive(&ahci_driver, drive,
							 drive->type == ahci_drive_type::SATA
							 ? ATA_SECTOR_SIZE
							 : ATAPI_SECTOR_SIZE, drive->size);
	}

	k_assert(false);
}