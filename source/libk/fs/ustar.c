
#include <fs/ustar.h>
#include <mem.h>

USTAR_FILE_t* ustar_file_lookup(file_system_t* fs, char* file_name) {
	//fs is the file system to look in.
	//file_name is the file name.
	//the return value is a USTAR_FILE structure with the necessary data.
	//NULL is returned when the file cannot be found.
	
	//this buffer is used to hold the data read from the disk. it doesn't serve a purpose
	//other than that.
	uint8_t* buf = kmalloc(512);
	
	//this variable holds which sector of the fs we need to read. it will be incremented
	//as necessary in the loop.
	uint32_t sector = 0;	
	
	
	
	//load the first sector of the file system into memory.
	if (fs_read_sectors(fs, 0, 1, buf)) {
		//an error occured.
		kfree(buf);
		return NULL;
	}
	
	//this loop iterates through the entire archive.
	while (!memcmp(buf + 257, "ustar", 5)) {
		uint32_t file_size = oct2bin(buf + 124, 11);
		
		//check if this is the file we're looking for
		if (!memcmp(buf, file_name, strlen(file_name) + 1)) {
			//we found it, now we can return.
			
			//this is the data we're going to return.
			USTAR_FILE_t* file = kmalloc(sizeof(USTAR_FILE_t));
			
			memcpy(file->file_name, buf, 100);
			memcpy(file->owner_user_name, buf + 265, 32);
			memcpy(file->owner_group_name, buf + 297, 32);
			
			file->type = buf[156];	//the type flag.
			file->starting_lba = sector;
			file->size = file_size;
			
			return file;
		}
		
		
		//this is not it either, skip.
		
		sector += (((file_size + 511) / 512) + 1);
		if (fs_read_sectors(fs, sector, 1, buf)) {
			//an error occured.
			
			break;
		}
		
		
		
	}
	
	kfree(buf);
	return NULL;
};



uint8_t ustar_load_file(file_system_t* fs, char* file_name, void* buf) {
	//fs is the file system to be read from.
	//file_name is the file name.
	//buf is the buffer to be written to. The file will be loaded there.
	//return value indicates whether the function encountered an error or not.
	//return value of 0 = success, while any other value means an error.
	
	
	USTAR_FILE_t* file = ustar_file_lookup(fs, file_name);
	if (!file) {
		return 1;
	}
	
	
	uint32_t starting_lba = file->starting_lba;
	starting_lba++;	//increment it, because we don't need the "meta-data sector".
	
	
	
	uint8_t* dest 	= (uint8_t*)buf;
	
	if ((file->size / 512) == 0) {
		//then the file doesn't occupy a full sector.
		//so we do the "remainder" part early.
		
		//we need a temporary buffer. we will first read the sector here, and then copy the
		//necessary parts to dest. This is necessary as reading the entire sector
		//directly to dest can cause data corruption.
		uint8_t* temp = kmalloc(512);
		
		if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
			//error.
			kfree(temp);
			return 1;
		} 
		
		//now a memcpy call should do it.
		memcpy(dest, temp, file->size % 512);
		kfree(temp);
		return 0;
	}
	
	
	
	
	//this loop transfers the whole sectors within the file. the remainder will be done
	//after this.
	
	if (fs_read_sectors(fs, starting_lba, file->size/512, (uint16_t*)(dest))) {
		return 1;	//error while reading.
	}
	
	
	
	if (file->size % 512 == 0) {
		//if there is no "remainder", don't even bother continuing after here.
		//I'm doing this because reading from a disk is pretty damn slow.
		return 0;
	}
	
	
	//increment it, because we need to read the last sector now.
	starting_lba += file->size / 512;
	
	
	//we need a temporary buffer. we will first read the sector here, and then copy the
	//necessary parts to dest. This is necessary as reading the entire sector
	//directly to dest can cause data corruption.
	uint8_t* temp = kmalloc(512);
		
	if (fs_read_sectors(fs, starting_lba, 1, (uint16_t*)temp)) {
		//error.
		kfree(temp);
		return 1;
	} 
	
	//now a memcpy call should do it.
	memcpy(dest + (file->size / 512), temp, file->size % 512);
	kfree(temp);
	return 0;
	
};

uint8_t ustar_read_file(file_system_t* fs, char* file_name, void* buf, uint32_t offset, uint32_t bytes) {
	//fs is the file system to be read from.
	//file_name is self-explanatory.
	//buf is the buffer to be read into. It is assumed to have enough space.
	//offset is the offset to be read from, in bytes.
	//bytes is the amount of bytes to be read.
	
	
	
	//first, some info.
	
	USTAR_FILE_t* file = ustar_file_lookup(fs, file_name);
	if (!file) {
		return 1;
	}
	
	
	uint32_t starting_sector = file->starting_lba;
	
	
	//skip the metadata.
	starting_sector++;
	
	//now we need to figure out which sector we're gonna start reading from.
	starting_sector += offset / 512;
	
	
	
	//now to read and store the proper data.
	if (fs_read_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;	//an error.
	}
	
	return 0;
};


uint8_t ustar_write_file(file_system_t* fs, char* file_name, void* buf, uint32_t offset, uint32_t bytes) {
	//fs is the file system to be read from.
	//file_name is self-explanatory.
	//buf is the buffer to be read into. It is assumed to have enough space.
	//offset is the offset to be read from, in bytes.
	//bytes is the amount of bytes to be written.
	
	
	
	//first, some info.
	
	USTAR_FILE_t* file = ustar_file_lookup(fs, file_name);
	
	if (!file) {
		return 1;
	}
	
	
	uint32_t starting_sector = file->starting_lba;
	
	
	//skip the metadata.
	starting_sector++;
	
	//now we need to figure out which sector we're gonna start reading from.
	starting_sector += offset / 512;
	
	
	
	//now to read and store the proper data.
	if (fs_write_bytes(fs, buf, starting_sector, offset % 512, bytes)) {
		return 1;	//an error.
	}
	
	return 0;
};


