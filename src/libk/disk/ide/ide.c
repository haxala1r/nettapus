

#include <disk/ide.h>
#include <pci.h>
#include <mem.h>


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



//info gathering functions.

uint8_t ide_identify_noid(uint16_t io_base, uint8_t slave, uint16_t* buf) {
	/* Executes an identify command. Doesn't use an ID because this is
	 * only supposed to check whether the disk exists or not.
	 * Returns 0 if disk exists, 1 otherwise.
	 */

	outb(io_base + 6, 0xA0 | (slave << 4));
	outb(io_base + 2, 0);
	outb(io_base + 3, 0);
	outb(io_base + 4, 0);
	outb(io_base + 5, 0);

	outb(io_base + 7, 0xEC);

	if (inb(io_base + 7) == 0) {
		return 1;	 /* Drive does not exist. */
	}

	/* We need to poll until BSY clears. */
	while (1) {
		uint8_t stat = inb(io_base + 7);

		if (stat & 0x80) {
			continue;
		}

		break;
	}

	/* Now check lbamid and lbahi */
	if ((inb(io_base + 4) != 0) || (inb(io_base + 5) != 0)) {
		/* İf either of these are non-zero, then the drive is not IDE. */
		return 1;
	}

	/* Now we need to poll until either DRQ or ERR is set.*/
	while (1) {
		uint8_t stat = inb(io_base + 7);
		if (stat & 8) {
			break;		/* DRQ set. */
		}
		if (stat & 1) {
			return 1;	/* ERR set.*/
		}
	}

	/* Once we reach here, it is certain the drive is IDE and we can extract info. */
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


void ide_register_disk(uint16_t id, uint16_t io, uint16_t control, uint8_t slave, uint32_t max_lba28, uint64_t max_lba48) {

	/*
	 * this adds a disk to the list, and if that disk already exists, refreshes its data.
	 */


	ide_drive_t* ndrive = kmalloc(sizeof(ide_drive_t));
	ndrive->drive_id = id;
	ndrive->io_port_base = io;
	ndrive->control_port_base = control;
	ndrive->slave = slave;
	ndrive->lba28_sectors = max_lba28;
	ndrive->lba48_sectors = max_lba48;
	ndrive->next = NULL;

	//if no drives exist yet, simply add it.
	if (ide_first_drive == NULL) {
		ide_first_drive = ndrive;
		return;
	}

	//first, we need to get to the last element.
	//we can  also check if the ID already exists while  we're at it.
	ide_drive_t* i = ide_first_drive;
	while (1) {
		if (i->drive_id == id) {
			//a disk with that id exists. no need for the allocation we made for ndrive.
			//we just update the entry, then free the memory we allocated.
			*i = *ndrive;
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

	//discard the list if it already exists.
	if (ide_first_drive != NULL) {
		ide_drive_t* j = ide_first_drive;
		ide_drive_t* k = j->next;
		while (1) {
			kfree(j);
			j = k;

			if (j == NULL) {
				break;
			}
			k = k->next;
		}
		ide_first_drive = NULL;
	}



	uint8_t i = 0;	//a simple iterator to determine the disk IDs.
	ide_bus_t* ibus = ide_get_first_bus();
	uint16_t* buf = kmalloc(512);	//used to temporarily store identify info.

	while (1) {
		if (ibus == NULL) {
			break;
		}

		uint8_t stat = inb(ibus->io_base);
		if ((stat == 0xFF) && (stat == 0x7F)) {
			continue;	//there are no drives on this bus.
		}

		/* Sorry for the long lines. The compiler wouldn't let me divide these into different
		 * lines via variables (it randomly crashed when I attempted that.)
		 * The lengthy parts are the shifts necessary to assemble the maximum lba28 and lba48
		 * (respectively) addressable sectors.
		 */

		//check and register drives.
		if (!ide_identify_noid(ibus->io_base, 0, buf)) {
			ide_register_disk(i * 4, ibus->io_base, ibus->control_base, 0, buf[60] | (buf[61] << 16), (buf[100]) | (buf[101] << 16) | ((uint64_t)buf[102] << 32) | ((uint64_t)buf[103] << 48));
		}
		if (!ide_identify_noid(ibus->io_base, 1, buf)) {
			ide_register_disk(i * 4 + 1, ibus->io_base, ibus->control_base, 1, buf[60] | (buf[61] << 16), (buf[100]) | (buf[101] << 16) | ((uint64_t)buf[102] << 32) | ((uint64_t)buf[103] << 48 ));
		}


		stat = inb(ibus->secondary_io_base);
		if ((stat == 0xFF) && (stat == 0x7F)) {
			continue;	//there are no drives on this bus.
		}

		//check and register drives.
		if (!ide_identify_noid(ibus->secondary_io_base, 0, buf)) {
			ide_register_disk(i * 4 + 2, ibus->secondary_io_base, ibus->secondary_control_base, 0, buf[60] | (buf[61] << 16), (buf[100]) | (buf[101] << 16) | ((uint64_t)buf[102] << 32) | ((uint64_t)buf[103] << 48 ));
		}
		if (!ide_identify_noid(ibus->secondary_io_base, 1, buf)) {
			ide_register_disk(i * 4 + 3, ibus->secondary_io_base, ibus->secondary_control_base, 1, buf[60] | (buf[61] << 16), (buf[100]) | (buf[101] << 16) | ((uint64_t)buf[102] << 32) | ((uint64_t)buf[103] << 48 ));
		}


		i++;
		ibus = ibus->next;
	}
	kfree(buf);
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

	/* Busmastering DMA */
	if (bus->bar4 != 0) {
		nbus->dma_flag = 1;
		nbus->busmaster_base = bus->bar4;
	} else {
		nbus->dma_flag = 0;
	}




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


uint8_t init_ide() {
	//first, we're going to loop through all the pci devices to find a
	//IDE controller. if we can't find one, we're kinda screwed.
	//because y'know, this is mainly a ATA PIO driver.

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

	//check if everything went as smoothly as they are supposed to.
	if (ide_get_first_bus() == NULL) {
		return 1;
	}
	if (ide_get_first_drive() == NULL) {
		return 2;
	}
	return 0;
};







#ifdef DEBUG
#include <tty.h>

void ide_print_state() {
	kputs("\nIDE STATE\n {drive id}: {io base} {control base} {lba28 sectors} {lba48 sectors}\n");
	ide_drive_t *di = ide_first_drive;

	while (di) {
		kputx(di->drive_id);
		kputs(": ");

		kputx(di->io_port_base);
		kputs(" ");
		kputx(di->control_port_base);
		kputs(" ");

		kputx(di->lba28_sectors);
		kputs(" ");
		kputx(di->lba48_sectors);
		kputs("\n");

		di = di->next;
	}
};


#endif





