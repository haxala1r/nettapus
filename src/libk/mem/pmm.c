

//this file contains the Physical Memory Management (TM) parts of the Memory Manager (TM) of my hobby OS (TM)
//for Heap (TM) and Virtual Memory Management (TM) look at the other files in the same directory.

#include <mem.h>


memory_map_t physical_memory ;

memory_map_t* getPhysicalMem() {
	return &physical_memory;
}


uint8_t ispaValid(uint32_t addr) {
	for (size_t i = 0; i < physical_memory.num_blocks; i++) {
		uint32_t base = page_to_addr(physical_memory.blocks[i].base_page);
		uint32_t top = base + page_to_addr(physical_memory.blocks[i].length);
		//keep in mind if it is *equal* to "top" then it will be counted as invalid.
		if ((addr < top) && (addr >= base)) {
			return 1; //if there's a single match then it is valid.
		}
	}
	return 0;	//if it reaches here, then we didn't match any valid ranges. that means it is invalid.
}


uint8_t isppValid(uint32_t page) {
	uint32_t addr = page_to_addr(page);
	return ispaValid(addr);
}


uint8_t isppUsed(uint32_t page) {
	if (!isppValid(page)) {
		return 2;//return a value of 2 if the page isn't valid.
		//since most functions just use it to determine whether they can use this page like this:
		// if (!isPhysicalPageUsed(page)) { allocate;}
		//a value of 2 still indicates it isn't usable. thus preventing some errors.
	}
	uint32_t byte = physical_memory.bitmap[page/32]; //find the byte ou bit will be in.
	uint32_t bit = (byte >> (page % 32)) & 0b1;	//get the value of the corresponding bit.
	return (uint8_t)(bit);
}


uint8_t ispaUsed(uint32_t addr) {
	uint32_t page = addr_to_page(addr);
	return isppUsed(page);
}



uint8_t setppUsed(uint32_t page, uint8_t value) {
	if (!isppValid(page)) {
		return 1;	//error if it isn't valid
	}
	value = value & 0b1; //take only the first bit.
	
	uint32_t byte = physical_memory.bitmap[page/32];		//this is slightly misnamed but whatever.
	uint32_t bit = page % 32;
	
	if (value == 0) {
		//set it to zero.
		byte = byte & (0xFFFFFFFF ^ (1 << bit));
	} else {
		//set it to one.
		byte = byte | (1 << bit);
	}
	physical_memory.bitmap[page/32] = byte;
	return GENERIC_SUCCESS;
}



uint32_t allocpp() {
	//allocates a single physical page and returns its page number
	for (uint32_t i = 0; i < (sizeof(physical_memory.bitmap)/sizeof(physical_memory.bitmap[0])); i++) {
		//loop over each integer in the bitmap.
		uint32_t byt4 = physical_memory.bitmap[i];
		for (size_t j = 0 ; j < 32; j++) {
			//loop over each bit.
			uint32_t bit = byt4 & (1 << j);
			
			uint32_t page_num = i * 32 + j;
			//if it isn't zero, then it is used. skip used entries.
			if (bit) {
				continue;
			}
			//if it isn't valid, we can't allocate it.
			if (!isppValid(page_num)) {
				continue;
			}
			
			//if it is valid and it is free, allocate it.
			setppUsed(page_num, 1); //mark it as used.
			return page_num; //return it.
			
		}
		
		
	}
	return 0xFFFFFFFF;	//invalid page value.
}


uint32_t allocpps(uint32_t amount) {
	//(unlike what the name suggests) this function finds free, continuous"physical" pages of arbitrary amount,
	// marks it as "used" and returns the first one's page number. you can then use
	//that number to calculate starting address, from where you can map it
	//and use it.
	//keep in mind this function assumes there is enough memory in "dest",
	//where the page numbers will be stored.
	//also keep in mind, this isn't a wrapper around allocpp()
	//because putting that in a loop would be ineficient as fuck.
	uint32_t ia = 0;
	for (size_t i = 0; i < (sizeof(physical_memory.bitmap))*8; i++) {
		//iterate through all pages.
		if (isppUsed(i)) {
			ia = 0;
			continue;	//if it is used, then start again.
		}
		
		//check if the pages that come after this are available as well.
		for (uint32_t j = 0; j < amount; j++) {
			if (!isppUsed(i + j)) {
				ia++;
			}
		}
		if (ia != amount) {
			ia = 0;
			continue;
		}
		//if we're here, then that means we could find a match.
		//we simply allocate those pages, and return the first one's number.
		for (uint32_t j = 0; j < amount; j++) {
			setppUsed(i + j, 1);	//mark them as used.
		}
		//then return.
		return i;
	}
	return -1;	//if we can't find 
}


uint8_t freepp(uint32_t page) {
	return setppUsed(page, 0);	//yup. this is all it does. deal with it.
}




uint8_t init_pmm(struct multiboot_header* mbh) {
	
	if (!(mbh->flags & 0b1000000)) {
		/* if we reach here, then bit 6 is not set, meaning mmap_* fields
		 * of the multiboot header are not valid. We currently cannot retrieve
		 * a reliable physical memory map without those fields, so we return an error.
		 * I can hopefully change this in the future.
		 */
		 return 1;
	}
	
	mboot_memmap_t *entry = (mboot_memmap_t*) mbh->mmap_addr;
	physical_memory.num_blocks = 0;
	while ((uint32_t)entry < mbh->mmap_addr + mbh->mmap_length) {
		if (entry->type == USABLE_MEM) {
			//if it is usable memory we simply add that to the available memory blocks.
			//remember, this is a 32-bit kernel. this would change dramatically if that wasn't the case.
			//though I guess a lot of other things would need to change as well, so yeah.
			physical_memory.blocks[physical_memory.num_blocks++] = _create_block(entry->base_addr_low, entry->length_low); 
			
		}
		//otherwise, we skip it.
		entry = (mboot_memmap_t*) ((uint32_t)entry + entry->size + sizeof(entry->size));
	}
	
	return GENERIC_SUCCESS;
}


