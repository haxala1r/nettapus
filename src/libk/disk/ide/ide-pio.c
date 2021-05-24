
#include <disk/ide.h>

uint8_t ide_pio_poll(uint16_t port) {
	/* Poll until the drive has data to transfer, or an error occurs. */

	uint8_t stat;// = inb(port + 7);

	while (1) {
		stat = inb(port + 7);
		if (stat & 1) {
			return inb(port + 1);	//an error bit is set.
		}
		if (stat & 0x20) {
			return 0xFF;
		}

		if (((stat & 0x80) == 0) && (stat & 8)) {
			//BSY clear and DRQ is set -- data is ready.
			return 0;
		}

	}
}


uint8_t ide_pio_read28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t *buf) {
	//Does an ATA PIO read for given drive.
	//buf is assumed to be large enough to hold the data.

	ide_drive_t* drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;	//error. Obviously.
	}
	if (buf == NULL) {
		return 1;
	}
	if (drive->lba28_sectors == 0) {
		return 1;	//drive does not support lba28.
	}

	uint16_t count_w = sector_count;
	if (count_w == 0) {
		count_w = 256;	//we're going to use this to loop.
	}

	//Drive, OR'ed with highest 4 bits of starting_lba
	outb(drive->io_port_base + 6, (uint8_t)(0xE0 | (drive->slave << 4) | ((starting_lba >> 24) & 0x0F)) );

	//sectorcount
	outb(drive->io_port_base + 2, sector_count);

	//low eight bits of lba
	outb(drive->io_port_base + 3, (uint8_t)starting_lba & 0xFF);

	//next eight bits
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 8) & 0xFF);

	//and the next eight bits.
	outb(drive->io_port_base + 5, (uint8_t)(starting_lba >> 16) & 0xFF);

	//now the command
	outb(drive->io_port_base + 7, 0x20);

	while (!(inb(drive->io_port_base + 7) | 0x40)) {
		;
	}

	for (uint16_t i = 0; i < count_w; i++) {

		/* Poll. */
		if (ide_pio_poll(drive->io_port_base)) {
			return 1;
		}

		/* Data is ready, transfer it. */
		for (uint16_t j = 0; j < 256; j++) {
			buf[i * 256 + j] = inw(drive->io_port_base);
		}

		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	}

	return 0;
}

uint8_t ide_pio_read48(uint16_t drive_id, uint64_t starting_lba, uint16_t sector_count, uint16_t *buf) {
	/* Does a 48-bit PIO read on a given drive. */

	ide_drive_t *drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;
	}
	if (buf == NULL) {
		return 1;
	}
	if (drive->lba48_sectors < (starting_lba + sector_count)) {
		return 1;
	}

	uint8_t sectorhi = (uint8_t)(sector_count >> 8);
	uint8_t sectorlo = (uint8_t)(sector_count);
	uint8_t LBA1, LBA2, LBA3, LBA4, LBA5, LBA6;
	LBA1 = (uint8_t)(starting_lba);
	LBA2 = (uint8_t)(starting_lba >> 8);
	LBA3 = (uint8_t)(starting_lba >> 16);
	LBA4 = (uint8_t)(starting_lba >> 24);
	LBA5 = (uint8_t)(starting_lba >> 32);
	LBA6 = (uint8_t)(starting_lba >> 40);

	/* Select the drive. */
	outb(drive->io_port_base + 6, 0x40 | (drive->slave << 4));

	/* Send the relevant information to the drive. */
	outb(drive->io_port_base + 2, sectorhi);

	outb(drive->io_port_base + 3, LBA4);
	outb(drive->io_port_base + 4, LBA5);
	outb(drive->io_port_base + 5, LBA6);

	outb(drive->io_port_base + 2, sectorlo);

	outb(drive->io_port_base + 3, LBA1);
	outb(drive->io_port_base + 4, LBA2);
	outb(drive->io_port_base + 5, LBA3);

	/* Now send the command to read. */
	outb(drive->io_port_base + 7, 0x24);

	uint32_t countl = sector_count;
	if (sector_count == 0) {
		countl = 65536;
	}

	/* Give a 400ns delay. */
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);

	for (size_t i = 0; i < countl; i++) {

		/* Poll. */
		if (ide_pio_poll(drive->io_port_base)) {
			return 1;	/* Error must have occured. */
		}

		/* Data is ready. */
		for (size_t j = 0; j < 256; j++) {
			buf[i * 256 + j] = inw(drive->io_port_base);
		}

		/* Give the drive a 400ns delay. */
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	}

	return 0;
}



uint8_t ide_pio_write28(uint16_t drive_id, uint32_t starting_lba, uint8_t sector_count, uint16_t *buf) {
	/* Does a PIO write on a given drive. */

	ide_drive_t* drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;	//can't write if there's no drive.
	}
	if (buf == NULL) {
		return 1;
	}
	if (drive->lba28_sectors == 0) {
		return 1;	//drive does not support lba28.
	}


	uint16_t count_w = sector_count;
	if (count_w == 0) {
		count_w = 256;	//we're going to use this to loop.
	}


	//Drive, OR'ed with highest 4 bits of starting_lba
	outb(drive->io_port_base + 6, (uint8_t)(0xE0 | (drive->slave << 4) | ((starting_lba >> 24) & 0x0F)) );

	//sectorcount
	outb(drive->io_port_base + 2, sector_count);

	//low eight bits of lba
	outb(drive->io_port_base + 3, (uint8_t)starting_lba & 0xFF);

	//next eight bits
	outb(drive->io_port_base + 4, (uint8_t)(starting_lba >> 8) & 0xFF);

	//and the next eight bits.
	outb(drive->io_port_base + 5, (uint8_t)(starting_lba >> 16) & 0xFF);

	//now the command
	outb(drive->io_port_base + 7, 0x30);

	while (!(inb(drive->io_port_base + 7) | 0x40)) {
		;	/* Loop until RDY is set. */
	}



	for (uint16_t i = 0; i < count_w; i++) {

		/* Poll.*/
		if (ide_pio_poll(drive->io_port_base)) {
			return 1;
		}

		/* Drive is ready for transfer. */
		for (uint16_t j = 0; j < 256; j++) {
			outw(drive->io_port_base, buf[i * 256 + j]);

		}

		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	}

	/* Now we need to do a cache flush.*/
	outb(drive->io_port_base + 7, 0xE7);

	while (inb(drive->io_port_base + 7) & 0x80) {
		;	//wait until BSY clears.
	}

	//we should be done!
	return 0;
}


uint8_t ide_pio_write48(uint16_t drive_id, uint64_t starting_lba, uint16_t sector_count, uint16_t *buf) {
	/* Does a 48-bit PIO read on a given drive. */

	ide_drive_t *drive = ide_find_drive(drive_id);
	if (drive == NULL) {
		return 1;
	}
	if (buf == NULL) {
		return 1;
	}
	if (drive->lba48_sectors < (starting_lba + sector_count)) {
		return 1;
	}

	uint8_t sectorhi = (uint8_t)(sector_count >> 8);
	uint8_t sectorlo = (uint8_t)(sector_count);
	uint8_t LBA1, LBA2, LBA3, LBA4, LBA5, LBA6;
	LBA1 = (uint8_t)(starting_lba);
	LBA2 = (uint8_t)(starting_lba >> 8);
	LBA3 = (uint8_t)(starting_lba >> 16);
	LBA4 = (uint8_t)(starting_lba >> 24);
	LBA5 = (uint8_t)(starting_lba >> 32);
	LBA6 = (uint8_t)(starting_lba >> 40);

	/* Select the drive. */
	outb(drive->io_port_base + 6, 0x40 | (drive->slave << 4));

	/* Send the relevant information to the drive. */
	outb(drive->io_port_base + 2, sectorhi);

	outb(drive->io_port_base + 3, LBA4);
	outb(drive->io_port_base + 4, LBA5);
	outb(drive->io_port_base + 5, LBA6);

	outb(drive->io_port_base + 2, sectorlo);

	outb(drive->io_port_base + 3, LBA1);
	outb(drive->io_port_base + 4, LBA2);
	outb(drive->io_port_base + 5, LBA3);

	/* Now send the command to read. */
	outb(drive->io_port_base + 7, 0x34);

	uint32_t countl = sector_count;
	if (sector_count == 0) {
		countl = 65536;
	}

	/* Give a 400ns delay. */
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);
	inb(drive->io_port_base + 7);

	for (size_t i = 0; i < countl; i++) {

		/* Poll. */
		if (ide_pio_poll(drive->io_port_base)) {
			return 1;	/* Error must have occured. */
		}

		/* Drive is ready for transfer. */
		for (size_t j = 0; j < 256; j++) {
			outw(drive->io_port_base, buf[i * 256 + j]);
		}

		/* Give the drive a 400ns delay. */
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
		inb(drive->io_port_base + 7);
	}

	outb(drive->io_port_base + 7, 0xE7);

	while (inb(drive->io_port_base + 7) & 0x80) {
		;	//wait until BSY clears.
	}

	return 0;
}
