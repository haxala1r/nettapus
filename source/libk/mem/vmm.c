
//this file contains the Virtual Memory Management (TM) parts of the Memory Manager (TM) of my hobby OS (TM)
//for Heap (TM) and Physical Memory Management (TM) look at the other files in the same directory.

#include <mem.h>
#include <tty.h>


//the first two page tables are provided within the binary. cuz why not?
uint32_t fpage_table[1024] __attribute__((aligned(4096)));
uint32_t spage_table[1024] __attribute__((aligned(4096)));


page_directory_t kernel_page_dir __attribute__((aligned(4096)));	//page directory for the kernel process.
//I wanted this to be used internally for the VMM (TM), so I got it here. you can stil get a pointer to it with:
page_directory_t* kgetPageDir() {
	return &kernel_page_dir;
}
//...though.


uint8_t isvpMapped(uint32_t vp, page_directory_t *pd) {
	if (pd->dir[vp / 1024] & 0x1) {
		uint32_t* tb = (uint32_t*)(pd->dir[vp / 1024] & 0xFFFFF000);	//find the table address, which we'll use.
		uint32_t entry = vp % 1024;	//mod by 1024 to find the entry within the table.
		return (tb[entry] & 0b1);	//simply checks if the page is marked present or not.
	}
	//if table isn't marked present, then the page must not be present.
	return 0;
}

uint8_t isvaMapped(uint32_t va, page_directory_t *pd) {
	return isvpMapped(addr_to_page(va), pd);
}

uint32_t findvp(page_directory_t* pd) {
	//scan each table. the last one is reserved, as it is mapped to pd itself.
	for (size_t i = 0; i < 1023; i++) {
		
		//scan each entry.
		for (size_t j = 0; j < 1024; j++) {
			if (isvpMapped(i * 1024 + j, pd)) {
				//if it is used, skip.
				continue;
			}
			return (i * 1024 + j);
		}
	}
}





	//maps a physical page to a virtual one. refuses to work when virtual page is already mapped to.
uint8_t mapPage(uint32_t pp, uint32_t vp, page_directory_t *pd) {
	//maps a physical page to a vritual one, given page numbers and a page directory.
	
	if (isppValid(pp) == 0) {	//if pp isn't valid, we can't map it.
		return 1;
	}
	if (isvpMapped(vp, pd)) {
		return 1;	//can't map to it if the vp is already mapped to some address.
	}
	
	if ((pd->dir[vp / 1024] & 0xFFFFF000) == 0x00000000) {
		//the virtual page literally isn't available, not much we can do here.
		//we should return a unique error to show the need for more page tables.
		return 2;
	}
	uint32_t table_num = vp / 1024;	//get the table.
	uint32_t table_entry = vp % 1024;	//get the entry within the table.
	
	uint32_t entry = ((uint32_t*)(pd->dir[table_num] & 0xFFFFF000))[table_entry];
	entry = entry & 0xFFF;	//only the first 12 bits will be preserved, as they are flags. other bits might hold trash data.
	entry = entry | (page_to_addr(pp)) | 0b1;	//add the address to the entry, and mark it as present.
	
	
	((uint32_t*)(pd->dir[table_num] & 0xFFFFF000))[table_entry] = entry;	//write the entry to the table.
	
	
	
	return GENERIC_SUCCESS;
}

	//unmaps a virtual page, while freeing its physical page as well. this is why i didn't allow for a page to be mapped twice.
uint8_t unmapPage(uint32_t vp, page_directory_t* pd) {
	uint32_t* tb = (uint32_t*)(pd->dir[vp / 1024] & 0xFFFFF000);
	uint32_t entry_num = vp % 1024;
	uint32_t pa = tb[entry_num] & 0xFFFFF000;
	freepp(addr_to_page(pa));	//frees the physical page as well. keep this in mind.	
	tb[entry_num] = tb[entry_num] & 0xFFE;	//keeps all flags except "Present" and wipes the address.
	return GENERIC_SUCCESS;
}

	//identity maps a single page, given that both the physical and virtual pages are "free".
uint8_t idMapPage(uint32_t pp, page_directory_t* pd) {
	if (isppUsed(pp)) {
		return 1;
		
	}
	uint32_t status = mapPage(pp, pp, pd);
	if (status) {
		return status;
	}
	setppUsed(pp, 1);	//mark it as used.
	return GENERIC_SUCCESS;
}

	//identity maps a range of pages. don't know what you would use this with, but you're welcome I guess.
uint8_t idMapRange(uint32_t start, uint32_t end, page_directory_t* pd) {
	if (start >= end) {
		return 1;	//if that's the case, then what the fuck?
	}
	
	//first, check if all the physical and virtual pages in that region are free.
	
	//if they are all free, we can safely ID map them.
	for (uint32_t i = start; i < end; i++) {
		if (isppUsed(i)) {
			continue;	//if any one of the physical pages in that region are used, then skip this iteration.
			//I thought about just freeing it but that could lead to a lot of problems, so for now
			//we just pretend we can't do anything about it.
		}
		if (isvpMapped(i, pd)) {
			continue;
		}
		if (idMapPage(i, pd)) {
			return 1;	//if an error occurs (somehow) then return.
		}
	}
	return GENERIC_SUCCESS;
}




	//kernel page map physical (to) virtual.
uint8_t kpmappv(uint32_t pp, uint32_t vp) {
	//since all other kernel-page-mapping functions rely on this one, I thought it
	//would be a good place to do the "flush" thing where you reload the page directory
	//to make the changes take effect.
	uint8_t status = mapPage(pp, vp, kgetPageDir()); //because we need to compare the status to multiple values.
	if (status == 1) {
		return 1;
	} /*else if (status == 2) {
		//the page table the vp resides in does not exist. we should create a page table, 
		//call it again.
		
	}*/
	
	loadPageDirectory(kernel_page_dir.dir);	//"flush"
	return 0;
}

uint8_t kmmappv(uint32_t pa, uint32_t va) {
	if ((pa & 0xFFF) != (va & 0xFFF)) {
		//its impossible to map addresses with last 12 bits that don't match. 
		//for now, at least.
		return 1;
	}
	return kpmappv(addr_to_page(pa), addr_to_page(va));
}




	//kernel page map (to) virtual. 
uint8_t kpmapv(uint32_t vp) {
	return kpmappv(allocpp(), vp);
}

uint8_t kmmapv(uint32_t va) {
	return kmmappv(page_to_addr(allocpp()), va);
}


	//kernel page map
uint32_t kpmap() {
	uint32_t vp = findvp(kgetPageDir());
	if (kpmapv(vp)) {
		return 0;
	}
	return page_to_addr(vp);
}

	//kernel page ID map. returns a random ID mapped page.
uint32_t kpimap() {
	uint32_t pp = allocpp();
	freepp(pp);
	while (idMapPage(pp, kgetPageDir()) == 1) {
		char str[16];
		xtoa(pp, str);
		terminal_puts(str);
		//simple loop to find an id-mappable page, then return its addres

		pp++;	//simply try the next one.
	}
	return page_to_addr(pp);
}
	//kernel page unmap. unmaps a vp for the kernel.
uint32_t kpumap(uint32_t page_num) {
	if (unmapPage(page_num, kgetPageDir())) {
		return 1;
	} else {
		loadPageDirectory(kernel_page_dir.dir);
		return 0;
	}
}





//uint32_t kernel_end = 0x123;
uint8_t init_vmm() {
	memory_map_t *pm = getPhysicalMem();
	initPageDir(&kernel_page_dir);	//initialise the page directory for the kernel
	char str[16];
	memset(str, 0, 16);
	kernel_page_dir.dir[0] = ((uint32_t)(&fpage_table)) | 0x2 | 0x1;
	//added a second page table to actually be able to create heap outside of the first allocated 4M. 
	kernel_page_dir.dir[1] = ((uint32_t)(&spage_table)) | 0x2 | 0x1;
	
	//essentially, we map the last entry to the directory itself.
	//this makes it easier to access this thing, as it is literally always at a fixed virtual address.
	//now 0xffc00000 always points to the start of the page directory.
	kernel_page_dir.dir[1023] = ((uint32_t)kernel_page_dir.dir) | 0x2 | 0x1;	//map the last entry to itself.
	
	/* 
	 * we need to do the area below 1M manually because the PMM considers it
	 * "unavailable" but the tty.h header file depends on it.
	 * And thats why we're basically ID mapping the entire area below 
	 * the end of the kernel.
	 */
	 
	for (size_t i = 0; i < (addr_to_page((uint32_t)&kernel_end)); i++) {
		fpage_table[i] = (page_to_addr(i)) | 0x2 | 0x1;
		setppUsed(i, 1);	//mark the physical page as used, because... it kind of... is?
	}
	
	loadPageDirectory(kernel_page_dir.dir);
	
	kernel_page_dir.dir[2] = (kpimap()) | 0x2 | 0x1;	//read-write, present. allocates another page table.
	
	return GENERIC_SUCCESS;
}







