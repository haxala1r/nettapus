 

#ifndef _DISK_H
#define _DISK_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <multiboot.h>

//this file contains stuff that is common to every disk driver.
//stuff like which disk we booted from, which partitions have which file
//systems, etc. etc.

//The BIOS disk number of the drive the kernel was loaded from.
static uint8_t DISK_NUMBER;	




//just some generic stuff
uint32_t byte_to_sector(uint32_t);
uint32_t sector_to_byte(uint32_t);



void init_disk(struct multiboot_header*);	//initalises some variables with info about the system, e.g which disk we booted from









#ifdef __cplusplus
}
#endif



#endif


