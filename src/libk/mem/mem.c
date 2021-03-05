
#include <mem.h> 


/* Generic functions used when dealing with pages. 
 * Might be a good idea to put some of these into macros.
 */
uint64_t page_to_addr(uint64_t page) {
	
	return page * 0x1000;
}


uint64_t addr_to_page(uintptr_t addr) {
	return (uint64_t)(addr / 0x1000); //note that this gives the page the address is in, by truncating it.
}


void _create_block(uint64_t base_addr, uint64_t len, memory_block_t *dest) {
	//This function takes a *physical* block, its base address and length,
	//and initialises the block with the given information.
	dest->base_page = addr_to_page(base_addr);
	dest->length = addr_to_page(len);
	
	return;
}



uint8_t init_memory(struct stivale2_struct_tag_memmap *memtag) {

	if (init_pmm(memtag)) {
		return 1;
	}
	
	if (init_vmm()) {
		return 2;
	}
	
	if (init_heap()) {
		return 3;
	}
	
	return GENERIC_SUCCESS;
}




