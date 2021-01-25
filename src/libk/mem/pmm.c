

//this file contains the Physical Memory Management (TM) parts of the Memory Manager (TM) of my hobby OS (TM)
//for Heap (TM) and Virtual Memory Management (TM) look at the other files in the same directory.

#include <mem.h>


memory_map_t physical_memory = {};

memory_map_t* getPhysicalMem() {
	return &physical_memory;
}




uint8_t isppValid(uint64_t page) {
	
	for (size_t i = 0; i < physical_memory.num_blocks; i++) {
		
		uint64_t base = physical_memory.blocks[i].base_page;
		uint64_t top = base + physical_memory.blocks[i].length;
		
		if ((page < top) && (page >= base)) {
			return 1;	/* It is within the boundries of a block, so it must be valid. */
		}
		
	}
	
	return 0; /* The page isn't valid/usable. */
}


uint8_t ispaValid(uintptr_t addr) {
	return isppValid(addr_to_page(addr));
}


uint8_t isppUsed(uint64_t page) {
	if (!isppValid(page)) {
		return 2;
		/* 
		 * Return a value of 2 if the page isn't valid.
		 * Most functions will just use this function to determine whether they can 
		 * use this page like this:
		 * 	if (!isPhysicalPageUsed(page)) { <call to allocpp here>;}
		 * a value of 2 still indicates it isn't usable. Thus preventing some errors.
		 */
	}
	
	/* Find the byte our bit will be in. */
	uint64_t byte = physical_memory.bitmap[page/64]; 	
	
	/* Get the value of the corresponding bit. */
	uint64_t bit = (byte >> (page % 64)) & 0b1;
	
	
	return (uint8_t)(bit);
}


uint8_t ispaUsed(uintptr_t addr) {
	return isppUsed(addr_to_page(addr));
}



uint8_t setppUsed(uint64_t page, uint8_t value) {
	if (!isppValid(page)) {
		return 1;	/* Page is invalid. */
	}
	
	/* A page on the bitmap can only have a single bit for it.*/
	value = value & 0b1; 
	
	

	uint64_t byte = physical_memory.bitmap[page/64];
	
	/* Bit number. */
	uint64_t bit = page % 64; 	
	
	
	if (value == 0) {
		/* 
		 * This sets it to zero by xor'ing all 1's with the bit, which is then AND'ed
		 * with byte to set it to zero.
		 */
		byte = byte & (0xFFFFFFFFFFFFFFFF ^ (1 << bit));
		
	} else { 
		byte = byte | (1 << bit); 
	}
	
	/* Now write the byte back into the bitmap. */
	physical_memory.bitmap[page/64] = byte;
	
	
	return GENERIC_SUCCESS;
}



uint64_t allocpp() {
	/* This function allocates a single (usable) physical page, and returns its page number. */
	
	for (uint64_t i = 0; i < (sizeof(physical_memory.bitmap) * 8); i++) {
		if (!isppUsed(i)){
			setppUsed(i, 1);
			return i;
		}
	}
	
	/* 
	 * This is supposed to be an invalid page value. 
	 * Might be a good idea to change it later. 
	 */
	return 0;	
}


uint64_t allocpps(uint64_t amount) {
	/* 
	 * This function is like allocpp(), except it allocates multiple, *continous* physical
	 * pages. It doesn't loop over allocpp() because that would be very inefficient.
	 */
	
	
	
	uint64_t ia = 0;
	for (uint64_t i = 0; i < (sizeof(physical_memory.bitmap))*8; i++) {
		/* This loop goes over every page available. */
		
		for (uint64_t j = 0; j < amount; j++) {
			if (!isppUsed(i + j)) {
				ia++;
			}
		}
		
		if (ia != amount) {
			/* The amount of continous pages at i was less than the requested amount. */
			ia = 0;
			i += amount;	/* We already checked this much. */
			continue;
		}
		
		
		/* If we reached here, then we found the requested amount of pages. */
		
		/* Mark the pages as used. */
		for (uint64_t j = 0; j < amount; j++) {
			setppUsed(i + j, 1);	
		}
		
		/* Return the first page's number. */
		return i;
	}
	
	/* If we're here, then the amount of pages requested could not be found. */
	return 0;	
}


uint8_t freepp(uint64_t page) {
	return setppUsed(page, 0);	/* Might be a good idea to make this a macro. */
}




uint8_t init_pmm(struct stivale2_struct_tag_memmap *memtag) {
	if (memtag == NULL) {
		return 1;
	}
	if (memtag->tag.identifier != STIVALE2_STRUCT_TAG_MEMMAP_ID) {
		return 1;
	}
	
	/* 
	 * This is technically undefined behaviour if we don't set it, so we initialise it here
	 * with a zero.
	 */
	physical_memory.num_blocks = 0;
	
	/* Extract the necessary info from the memory map provided by the bootloader. */
	for (size_t i = 0; i < memtag->entries; i++) {
		
		if (memtag->memmap[i].type != STIVALE2_MMAP_USABLE){
			/* TODO: add the *-reclaimable fields to the memory map as well. */
			continue;
		}
		_create_block(memtag->memmap[i].base, memtag->memmap[i].length, &(physical_memory.blocks[physical_memory.num_blocks]));
		physical_memory.num_blocks++;
	}
	
	
	/* Mark the first 4 MiBs as used. */
	for (size_t i = 0; i < 16; i++) {
		physical_memory.bitmap[i] = 0xFFFFFFFFFFFFFFFF;
	}
	
	return 0;
}


