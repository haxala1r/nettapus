

#include <ide.h>

ide_drive_t* ide_first_drive;
ide_bus_t* ide_first_bus;

ide_drive_t* ide_get_first_drive() {
	return ide_first_drive;
};

ide_bus_t* ide_get_first_bus() {
	return ide_first_bus;
};


//functions to do stuff.

ide_drive_t* ide_find_drive(uint16_t drive_id) {
	//finds the drive with the given id, and returns it.
	
	ide_drive_t* i = ide_get_first_drive();
	
	while (i != NULL) {
		if (i->drive_id == drive_id) {
			return i;
		}
		i = i->next;
	}
	return NULL;
};


uint8_t ide_pio_poll(uint16_t port) {
	//Poll until data is available.
	//a return of 1 means error, and 0 means success.
	
	
	uint8_t stat = inb(port + 7);
	
	while (1) {
		stat = inb(port + 7);
		if ((stat & 1) || (stat & 0x20)) {
			return 1;	//an error bit is set.
		}
		
		if (((stat & 0x80) == 0) || (stat & 8)) {
			//BSY clear and DRQ is set -- data is ready.
			return 0;
		}
		
	}
	
};


uint8_t ide_pio_read28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t* buf) {
	//Does an ATA PIO read for given drive.
	//buf is assumed to be large enough to hold the data.
	
	ide_drive_t* drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;	//error. Obviously.
	}
	if (buf == NULL) {
		return 1;
	}
	
	
	uint16_t count_w = sector_count;
	if (count_w == 0) {
		count_w = 256;	//we're going to use this to loop.
	}
	
	
	//Drive, OR'ed with highest 4 bits of starting_lba
	outb(drive->io_port_base + 6, 0xE0 | (drive->slave << 4) | ((starting_lba >> 24) & 0x0F) );
	
	//sectorcount
	outb(drive->io_port_base + 2, sector_count);
	
	//low eight bits of lba
	outb(drive->io_port_base + 3, (uint8_t)starting_lba & 0xFF);
	
	//next eight bits
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 8) & 0xFF);
	
	//and the next eight bits.
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 16) & 0xFF);
	
	//now the command
	outb(drive->io_port_base + 7, 0x20);
	
	
	
	for (uint16_t i = 0; i < count_w; i++) {
		
		
		
		//now we poll.
		if (ide_pio_poll(drive->io_port_base)) {
			//we reach here if an error occurs.
			//not much we can do, so let's just return an error.
			return 1;
		}
		
		//data should be ready.
		for (uint16_t j = 0; j < 256; j++) {
			buf[i * 256 + j] = inw(drive->io_port_base);
		}
		
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	
		
	}
	
	return 0;	//Success!
};


uint8_t ide_pio_write28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t* buf) {
	//Does an ATA PIO read on the given disk.
	//buf is assumed to hold the data that will be written.
	
	ide_drive_t* drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;	//can't write if there's no drive.
	}
	if (buf == NULL) {
		return 1;
	}
	
		
	uint16_t count_w = sector_count;
	if (count_w == 0) {
		count_w = 256;	//we're going to use this to loop.
	}
	
	
	//Drive, OR'ed with highest 4 bits of starting_lba
	outb(drive->io_port_base + 6, 0xE0 | (drive->slave << 4) | ((starting_lba >> 24) & 0x0F) );
	
	//sectorcount
	outb(drive->io_port_base + 2, sector_count);
	
	//low eight bits of lba
	outb(drive->io_port_base + 3, (uint8_t)starting_lba & 0xFF);
	
	//next eight bits
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 8) & 0xFF);
	
	//and the next eight bits.
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 16) & 0xFF);
	
	//now the command
	outb(drive->io_port_base + 7, 0x30);
	
	
	
	for (uint16_t i = 0; i < count_w; i++) {
		//now we poll.
		if (ide_pio_poll(drive->io_port_base)) {
			//we reach here if an error occurs.
			//not much we can do, so let's just return an error.
			return 1;
		}
		
		//data should be ready.
		for (uint16_t j = 0; j < 256; j++) {
			outw(drive->io_port_base, buf[i * 256 + j]);
			
		}
		
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	
		
	}
	//we need to do a cache flush, since the command should be complete now.
	outb(drive->io_port_base + 7, 0xE7);
	
	while (inb(drive->io_port_base + 7) & 0x80) {
		;	//wait until BSY clears.
	}
	
	//we should be done!
	return 0;
};

//info gathering functions.

uint8_t ide_identify_noid(uint16_t io_base, uint8_t slave, uint16_t* buf) {
	//executes an identify command. Doesn't use an ID because this is 
	//supposed to be used before the list is set up.
	//returns 0 on success. 1 otherwise.
	
	outb(io_base + 6, 0xA0 | (slave << 4));
	outb(io_base + 2, 0);
	outb(io_base + 3, 0);
	outb(io_base + 4, 0);
	outb(io_base + 5, 0);
	
	outb(io_base + 7, 0xEC);
	
	if (inb(io_base + 7) == 0) {
		//the drive does not exist.
		return 1;	//error.
	}
	
	//we gotta poll a bit.
	while (1) {
		uint8_t stat = inb(io_base + 7);
		if (stat & 0x80) {
			//BSY set, continue until it clears.
			continue;
		}
		break;	//BSY clear, stop polling.
	}
	
	
	
	//check lbamid and lbahi.
	if ((inb(io_base + 4) != 0) || (inb(io_base + 5) != 0)) {
		//if these are non-zero, the drive is not ATA.
		//If I ever add support for ATAPI and SATA, this is where the code should go for them.
		return 1;
	}
	
	
	
	//otherwise, we poll some more.
	
	while (1) {
		uint8_t stat = inb(io_base + 7);
		if (stat & 8) {
			//DRQ is set, meaning we should stop polling.
			break;
		}
		if (stat & 1) {
			return 1;	//ERR is set.
		}
	}
	
	
	
	//if we're here, that means the drive is ATA and we can extract the info.
	if (buf != NULL) {
		for (size_t i = 0; i < 256; i++) {
			buf[i] = inw(io_base);
		}
	} else {
		for (size_t i = 0; i < 256; i++) {
			inw(io_base);	//we can't store them, but we need to read them.
		}
	}
	return 0;
};


void ide_register_disk(uint16_t id, uint16_t io, uint16_t control, uint8_t slave) {
	/* 
	 * this adds a disk to the list, but not if a disk with that id 
	 * already exists. This is not a bug. This is how it's supposed to work.
	 * that's because ide_refresh_disks assigns ID's based on the addresses
	 * used to access a disk. This means a disk will always have the same ID.
	 * newly added disks also follow this, so it's impossible for 2 disks to have
	 * the same id anyway. Which means, if that hppens, it's because ide_refresh_disks()
	 * was called, and thus that specific disk doesn't need to be re-registered.
	 * Thanks for coming to my TED talk.
	 */
	 
	 
	ide_drive_t* ndrive = kmalloc(sizeof(ide_drive_t));
	ndrive->drive_id = id;
	ndrive->io_port_base = io;
	ndrive->control_port_base = control;
	ndrive->slave = slave;
	ndrive->next = NULL;
	 
	//if no drives exist yet, simply add it.
	if (ide_first_drive == NULL) {
		ide_first_drive = ndrive;
	}
	 
	//first, we need to get to the last element.
	//we can  also check if the ID already exists while  we're at it.
	ide_drive_t* i = ide_first_drive;
	while (1) {
		if (i->drive_id == id) {
			kfree(ndrive);
			return;	//the device already exists.
		}
		if (i->next == NULL) {
			break;
		}
		 
		 
		i = i->next;
	}
	
	//if we reached here that must mean we're at the end of the list,
	//and we also haven't encountered a device with the same id.
	//so we can add our new drive.
	i->next = ndrive;
	
	//disable interrupts.
	outb(ndrive->io_port_base + 6, 0xA0 | (ndrive->slave << 4));
	outb(ndrive->control_port_base, 0b10);
	
	return;		//we're done here.
};


void ide_refresh_disks() {
	//re-loads all the information about all disks on the system.
	//can also be used to load all disks at the start.
	uint8_t i = 0;	//a simple iterator to determine the disk IDs.
	ide_bus_t* ibus = ide_get_first_bus();
	while (1) {
		if (ibus == NULL) {
			break;
		}
		
		uint8_t stat = inb(ibus->io_base);
		if ((stat == 0xFF) && (stat == 0x7F)) {
			continue;	//there are no drives on this bus.
		}
		
		//check and register drives.
		if (!ide_identify_noid(ibus->io_base, 0, NULL)) {
			ide_register_disk(i * 4, ibus->io_base, ibus->control_base, 0);
		}
		if (!ide_identify_noid(ibus->io_base, 1, NULL)) {
			ide_register_disk(i * 4 + 1, ibus->io_base, ibus->control_base, 1);
		}
		
		
		stat = inb(ibus->secondary_io_base);
		if ((stat == 0xFF) && (stat == 0x7F)) {
			continue;	//there are no drives on this bus.
		}
		
		//check and register drives.
		if (!ide_identify_noid(ibus->secondary_io_base, 0, NULL)) {
			ide_register_disk(i * 4 + 2, ibus->secondary_io_base, ibus->secondary_control_base, 0);
		}
		if (!ide_identify_noid(ibus->secondary_io_base, 1, NULL)) {
			ide_register_disk(i * 4 + 3, ibus->secondary_io_base, ibus->secondary_control_base, 1);
		}
		
		
		i++;
		ibus = ibus->next;
	}
	
}


void ide_register_bus(pci_device_t* bus) {
	//remember, a bus is added regardless of whether it has devices connected
	//to it or not. this is because a disk might connect later on,
	//and we want to be able to refresh our list of disks without
	//breaking that.
	
	if (bus == NULL) {
		return;
	}
	
	ide_bus_t* nbus = (ide_bus_t*)kmalloc(sizeof(ide_bus_t));
	
	nbus->pci = bus;
	
	//io 
	if ((bus->bar0 == 0) || (bus->bar0 == 1)) {
		nbus->io_base = 0x1f0;
	} else {
		nbus->io_base = bus->bar0 & 0xFFFE;
	};
	
	//control 
	if ((bus->bar1 == 0) || (bus->bar1 == 1)) {
		nbus->control_base = 0x3F6;
	} else {
		nbus->control_base = bus->bar1 & 0xFFFE;
	};
	
	//secondary io
	if ((bus->bar2 == 0) || (bus->bar2 == 1)) {
		nbus->secondary_io_base = 0x170;
	} else {
		nbus->secondary_io_base = bus->bar2 & 0xFFFE;
	};
	
	//secondary control
	if ((bus->bar3 == 0) || (bus->bar3 == 1)) {
		nbus->secondary_control_base = 0x376;
	} else {
		nbus->secondary_control_base = bus->bar3 & 0xFFFE;
	};
	
	nbus->next = NULL;
	//now we should add this to the linked list
	
	if (ide_first_bus == NULL) {
		ide_first_bus = nbus;
		return;
	}
	
	//we need to find the last device.
	ide_bus_t* i = ide_first_bus;
	while (1) {
		if (i->next == NULL) {
			//this is the last element, we should add the new bus here.
			i->next = nbus;
			return;
		}
		i = i->next;
	}
};


void init_ide() {
	//first, we're going to loop through all the pci devices to find a 
	//IDE controller. if we can't find one, we're kinda screwed.
	//because y'know, this is mainly a ATA PIO driver.
	
	//first, let's find an IDE controller. there's almost always only one,
	//so we only find *a* valid IDE controller, and use that. if there are
	//any others, they will be ignored. hopefully I can add support for multiple
	//IDE controllers later on. Problems can arise otherwise.
	pci_device_t* i = pci_get_first_dev();
	while (1) {
		if (i == NULL) {
			break;
		}
		if ((i->class_code == 1) && (i->subclass_code == 1)) {
			//ide_register_bus does all the checking before actually
			//registering it, so no need to do it here.
			ide_register_bus(i);
		}
		i = i->next;
	}
	ide_refresh_disks();
};


void ide_print_devs() {
	ide_drive_t* i = ide_get_first_drive();
	while (i != NULL) {
		char str[16] = {};
		xtoa(i->drive_id, str);
		terminal_puts(str);
		terminal_putc(' ');
		i = i->next;
	}
	
};



