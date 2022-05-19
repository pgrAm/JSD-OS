#ifndef ATA_CMD_H
#define ATA_CMD_H

#define ATA_CMD_READ_PIO          0x20u
#define ATA_CMD_READ_PIO_EXT      0x24u
#define ATA_CMD_READ_DMA          0xC8u
#define ATA_CMD_READ_DMA_EXT      0x25u
#define ATA_CMD_WRITE_PIO         0x30u
#define ATA_CMD_WRITE_PIO_EXT     0x34u
#define ATA_CMD_WRITE_DMA         0xCAu
#define ATA_CMD_WRITE_DMA_EXT     0x35u
#define ATA_CMD_CACHE_FLUSH       0xE7u
#define ATA_CMD_CACHE_FLUSH_EXT   0xEAu
#define ATA_CMD_PACKET            0xA0u
#define ATA_CMD_IDENTIFY_PACKET   0xA1u
#define ATA_CMD_IDENTIFY          0xECu

#define ATAPI_CMD_READ       0xA8u
#define ATAPI_CMD_EJECT      0x1Bu

#define ATA_IDENT_DEVICETYPE   0u
#define ATA_IDENT_CYLINDERS    2u
#define ATA_IDENT_HEADS        6u
#define ATA_IDENT_SECTORS      12u
#define ATA_IDENT_SERIAL       20u
#define ATA_IDENT_MODEL        54u
#define ATA_IDENT_CAPABILITIES 98u
#define ATA_IDENT_FIELDVALID   106u
#define ATA_IDENT_MAX_LBA      120u
#define ATA_IDENT_COMMANDSETS  164u
#define ATA_IDENT_MAX_LBA_EXT  200u

#define ATA_SR_BSY     0x80u    // Busy
#define ATA_SR_RDY	   0x60u    // Drive ready
#define ATA_SR_DRDY    0x40u    // Drive ready
#define ATA_SR_DF      0x20u    // Drive write fault
#define ATA_SR_DSC     0x10u    // Drive seek complete
#define ATA_SR_DRQ     0x08u    // Data request ready
#define ATA_SR_CORR    0x04u    // Corrected data
#define ATA_SR_IDX     0x02u    // Index
#define ATA_SR_ERR     0x01u    // Error

#define ATA_ER_BBK      0x80u    // Bad block
#define ATA_ER_UNC      0x40u    // Uncorrectable data
#define ATA_ER_MC       0x20u    // Media changed
#define ATA_ER_IDNF     0x10u    // ID mark not found
#define ATA_ER_MCR      0x08u    // Media change request
#define ATA_ER_ABRT     0x04u    // Command aborted
#define ATA_ER_TK0NF    0x02u    // Track 0 not found
#define ATA_ER_AMNF     0x01u    // No address mark

#define ATA_SECTOR_SIZE 0x200u
#define ATAPI_SECTOR_SIZE 0x800u

#endif