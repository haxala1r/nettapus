#ifndef IDE_H
#define IDE_H 1

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
#include <stdint.h>
#include <pci.h>

struct ide_drive {
	uint16_t drive_id;
	//ports that are associated with this drive.
	uint16_t io_port_base;
	uint16_t control_port_base;

	uint8_t slave;	//1 if this is the slave drive on its bus.

	uint32_t lba28_sectors;
	uint64_t lba48_sectors;

	struct ide_drive* next;		//it's a linked list.
};


struct ide_bus {
	uint16_t io_base;
	uint16_t control_base;
	uint16_t secondary_io_base;
	uint16_t secondary_control_base;

	uint8_t dma_flag;	/* 1 if DMA is supported.*/
	uint16_t busmaster_base;

	pci_device_t* pci;

	struct ide_bus* next;	//linked list. see below.
};

typedef 	struct ide_drive 	ide_drive_t;
typedef 	struct ide_bus 		ide_bus_t;

uint8_t ide_pio_read28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t *buf);
uint8_t ide_pio_write28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t *buf);

uint8_t ide_pio_read48(uint16_t drive_id, uint64_t starting_lba, uint16_t sector_count, uint16_t *buf);
uint8_t ide_pio_write48(uint16_t drive_id, uint64_t starting_lba, uint16_t sector_count, uint16_t *buf);

//basically, all disks in the system will be kept in a linked list.
//this will eventually be updated to include systems like USB, AHCI etc.
//but currently, all disks in the system are first scanned, and registered
//(added to the linked list.) after that, whenever the kernel makes a
//disk access, it will use a "disk id", which will be used internally
//by the driver to make the actual access.
ide_drive_t *ide_find_drive(uint16_t drive_id);
ide_drive_t *ide_get_first_drive();
ide_bus_t *ide_get_first_bus();
uint8_t ide_identify_noid(uint16_t io_base, uint8_t slave, uint16_t* buf);

uint8_t init_ide();

#ifdef DEBUG

void ide_print_state();
#endif




#ifdef __cplusplus
}
#endif

#endif


