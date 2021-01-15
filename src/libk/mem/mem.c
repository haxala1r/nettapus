
#include <mem.h> 

//generic memory operations and the physical memory allocator is in this file.

uint8_t PAGING_ENABLED = 0;






//generic page things.
uint32_t page_to_addr(uint32_t page) {
	//turns any page number into an address. note that it doesn't check whether page is valid or not.
	return page * 0x1000;
}


uint32_t addr_to_page(uint32_t addr) {
	return (uint32_t)(addr / 0x1000); //note that this gives the page the address is in, by truncating it.
}


struct memory_block _create_block(uint32_t base_addr, uint32_t len) {
	//create block from base address and length.
	struct memory_block n;
	
	//if base address isn't divisable by 0x1000 (4KB) then start at the next page.
	if ((base_addr & 0xFFFFF000) == base_addr) {
		n.base_page = base_addr / 0x1000;
	} else {
		n.base_page = (base_addr / 0x1000) + 1;
	}
	
	//likewise, if length isn't divisable by 0x1000 (4KB) then end at the second last page.
	//this is a temporary solution to the problem of not having perfectly aligned addresses.
	//this is a problem because it might let us write to locations we shouldn't write to.
	if ((len & 0xFFFFF000) == len) {
		n.length = len / 0x1000;
	} else {
		n.length = (len / 0x1000) - 1;
	}

	return n;
}


void initPageDir(page_directory_t *pd) {
	for (size_t i = 0; i < 1024; i++) {
		
		//read/write, not present. we will make space for page tables later.
		
		pd->dir[i] =  0x00000002;
		
		
	}
	pd->physical_address = ((uint32_t)pd) - 0xC0000000;
}







uint8_t init_memory(struct multiboot_header *mbh) {

	if (init_pmm(mbh)) {
		return 1;
	}
	
	if (init_vmm()) {
		return 2;
	}
	
	//init_vmm loads the page directory automatically.
	//all we need to do is enable paging, and we should be done.
	enablePaging();
	PAGING_ENABLED = 1;
	
	
	//now that Paging is fully set up, we can set up the heap.
	if (init_heap()) {
		return 3;
	}
	
	//if we reached here, then everything must have went well.
	//we can safely return.
	return GENERIC_SUCCESS;
	
	

}




