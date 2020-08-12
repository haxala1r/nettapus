

#include <disk/disk.h>


//now for some definitions

uint32_t byte_to_sector(uint32_t bytes) {
	return bytes / 512;
}

uint32_t sector_to_byte(uint32_t sector) {
	return sector * 512;
}




void init_disk(struct multiboot_header* mbh) {
	if (mbh->flags & 0b10) {
		//if the second bit is set, then the boot_device field must be valid.
		
		//this is unnecessary, but it makes it a little easier to see that we're just taking the first byte.
		DISK_NUMBER = (uint8_t)(mbh->boot_device & 0xFF);	
		
		
	}
}



 
