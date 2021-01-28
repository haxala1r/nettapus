
#include <fs/ustar.h>
#include <mem.h>

USTAR_FILE_t* ustar_file_lookup(file_system_t* fs, char* file_name) {
	/* This function locates the file givena a filename, and returns a struct with relevant info. */
	
	/* This is used to hold the data we read from disk. */
	uint8_t* buf = kmalloc(512);
	
	/* Holds the sector we're currently going to read. */
	uint32_t sector = 0;	
	
	
	
	/* This is tar, so the files start at the first sector of the filesystem. */
	if (fs_read_sectors(fs, 0, 1, buf)) {
		kfree(buf);
		return NULL;	/* An error must've occured. */
	}
	
	/* This loop iterates through the entire archive. */
	while (!memcmp(buf + 257, "ustar", 5)) {
		uint32_t file_size = oct2bin((char*)buf + 124, 11);
		
		/* Check if this is the file we're looking for. */
		if (!memcmp(buf, file_name, strlen(file_name) + 1)) {
			
			/* Collect the necessary info. */
			USTAR_FILE_t* file = kmalloc(sizeof(USTAR_FILE_t));
			
			memcpy(file->file_name, buf, 100);
			memcpy(file->owner_user_name, buf + 265, 32);
			memcpy(file->owner_group_name, buf + 297, 32);
			
			file->type = buf[156];	
			file->starting_lba = sector;
			file->size = file_size;
			
			return file;
		}
		
		
		/* Skip to the next file in the archive. */
		sector += (((file_size + 511) / 512) + 1);
		if (fs_read_sectors(fs, sector, 1, buf)) {
			break;
		}
	}
	
	kfree(buf);
	return NULL;
};



uint8_t ustar_load_file(file_system_t* fs, USTAR_FILE_t* file, void* buf) {
	/* This function reads the entire file, and loads it into buf. */
	if (file == NULL) {
		return 1;
	}
	
	
	uint32_t starting_lba = file->starting_lba;
	
	/* Increment this, because it is the LBA of the meta-data sector. We want the data sector. */
	starting_lba++;
	
	uint8_t* dest 	= (uint8_t*)buf;
	
	if ((file->size / 512) == 0) {
		/* The file occupies less than a full sector. */
		uint8_t* temp = kmalloc(512);
		
		if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
			kfree(temp);
			return 1;
		} 
		
		memcpy(dest, temp, file->size % 512);
		kfree(temp);
		return 0;
	}
	
	
	if (fs_read_sectors(fs, starting_lba, file->size/512, (uint16_t*)(dest))) {
		return 1;	//error while reading.
	}
	
	if (file->size % 512 == 0) {
		return 0;
	}
	
	/* Increment it, because we need to read the last sector now. */
	starting_lba += file->size / 512;
	
	
	/* First read the entire sector to temp, then only copy the requested amount. */
	uint8_t* temp = kmalloc(512);
		
	if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
		kfree(temp);
		return 1;
	} 
	
	memcpy(dest + (file->size / 512), temp, file->size % 512);
	kfree(temp);
	return 0;
	
};






uint8_t ustar_read_file(file_system_t* fs, USTAR_FILE_t* file, void* buf, uint32_t offset, uint32_t bytes) {
	/* This function reads bytes from file on fs, at offset. Hope that makes sense. */
	
	if (file == NULL) {
		return 1;
	}
	
	uint32_t starting_sector = file->starting_lba;
	
	
	/* Skip the metadata. */
	starting_sector++;
	
	/* Now we need to figure out which sector we're going to start reading from. */
	starting_sector += offset / 512;
	
	/* Now perform the disk access. */
	if (fs_read_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;
	}
	
	return 0;
};

uint8_t ustar_write_file(file_system_t* fs, USTAR_FILE_t* file, void* buf, uint32_t offset, uint32_t bytes) {
	/* This function writes bytes from buf to file on fs, at offset. */
	
	if (file == NULL) {
		return 1;
	}
	if (offset + bytes > file->size) {
		return 1;
	}
	
	
	uint32_t starting_sector = file->starting_lba;
	
	/* Skip the meta-data. */
	starting_sector++;
	
	/* Figure out at which sector we need to start reading from. */
	starting_sector += offset / 512;
	
	

	/* Perform the disk access. */
	if (fs_write_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;	//an error.
	}
	
	return 0;
};







uint8_t ustar_enlarge_file(file_system_t* fs, USTAR_FILE_t* file, uint32_t bytes) {
	/* This function enlarges file on fs by bytes. */
	
	/* This function's logic goes like this:
	 * Since the TAR format doesn't support fragmentation, that means (in order to enlarge the
	 * data section of a file) either the file is re-located to the end of the archive 
	 * (which doesn't work if the file isn't in the root directory), or, everything in the
	 * archive that comes after the file has to be moved forward by the amount of bytes we want the
	 * file to be enlarged by. This function does the latter, as the former isn't even a proper
	 * option. That means this function needs to move everything that comes (physically)
	 * after the file forward by some amount. This is done like this : First we find the last
	 * sector of the archive, then move everything by some amount of sectors, looping backwards
	 * through the archive. This ensures no data loss occurs, and continues until it reaches
	 * the file to be enlarged. Then, the files metadata is edited accordingly.
	 */ 
	
	
	
	
	if ((bytes/512) == 0) {
		if (((file->size % 512) + bytes) < 512) {
			//in this case, since enlarging the file won't cross a sector-boundary,
			//we don't actually need to move anything at all.
			//so we simply update the metadata, and return a success.
			
			
			file->size += bytes;
			
			/* Update meta-data. */
			char fsize[12] = {};
			memset(fsize, 0, 12);

			bin2oct(file->size, fsize, 11);
			
			
			
			/* No need to call fs_write_bytes. */
			uint8_t* buf = kmalloc(512);
			if (fs_read_sectors(fs, file->starting_lba, 1, buf)) {
				kfree(buf);
				return 1;
			}
			
			memcpy(buf + 124, fsize, 11);
			
			
			/* Write it back to disk. */
			if (fs_write_sectors(fs, file->starting_lba, 1, buf)) {
				kfree(buf);
				return 1;
			}
			
			kfree(buf);
			return 0;
		}
	}
	
	//amount of sectors to enlarge the file by.
	uint32_t amount_sectors = (bytes + 511)/512;
	
	//buffer to hold the sectors we read from disk.
	uint8_t* buf = kmalloc(512);
	
	
	//the current sector to be moved. Used inside the loop.
	uint32_t sector = 0;
	if (fs_read_sectors(fs, sector, 1, buf)) {
		kfree(buf);
		return 1;
	}
	//this loop determines the last sector of the archive.
	while (!memcmp(buf + 257, "ustar", 5)) {
		uint32_t fsize = oct2bin((char*)buf + 124, 11);
		
		sector += (((fsize + 511) / 512) + 1);
		if (fs_read_sectors(fs, sector, 1, buf)) {
			kfree(buf);
			return 1;
		}
	}
	
	
	
	//because of the way the loop works, sector currently points to what is *after* the
	//data section of the last file of the archive, not the last sector. 
	sector--;	
	
	//now we know the LBA of the last sector of the entire archive. now we determine the 
	//file's last sector, then loop and move everything forwards until we reach that sector.
	uint32_t file_last = file->starting_lba + ((file->size + 511)/512);
	
	
	//This loops until it reaches the file's last sector.
	//at every iteration, it moves the sector it's currently on
	while (sector > file_last) {
		if (fs_read_sectors(fs, sector, 1, buf)) {
			kfree(buf);
			return 1;
		}
		
		if (fs_write_sectors(fs, sector + amount_sectors, 1, buf)) {
			kfree(buf);
			return 1;
		}
		
		sector--;
	}
	
	
	//if everything worked correctly, we should also update some stuff on the disk as well as the
	//meta data here.
	/* 
	 * TODO: Tuck some of this stuff into some other "helper" functions. Also refactor
	 * A big chunk of the FS drivers in general. God, everything here is so damn shitty.
	 * sigh...
	 */
	file->size += bytes;
	
	char fsize[11] = {};
	memset(fsize, 0, 11);
	bin2oct(file->size, fsize, 11);
	
	
	if (fs_read_sectors(fs, file->starting_lba, 1, buf)) {
		kfree(buf);
		return 1;
	}
	
	memcpy(buf + 124, fsize, 11);
	
	
	if (fs_write_sectors(fs, file->starting_lba, 1, buf)) {
		kfree(buf);
		return 1;
	}
	
	kfree(buf);
			
			
			

	return 0;
};



