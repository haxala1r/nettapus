
/* This file contains the Virtual Memory Manager. (TM)*/

#include <mem.h>


/* Define the first structs, and initialise them with all zeroes. Necessary to bootstrap. */
p_map_level4_table __attribute__((aligned(4096))) kpml4 	= {};
pd_ptr_table __attribute__((aligned(4096))) k_first_pdpt 	= {};
page_dir __attribute__((aligned(4096))) k_first_pd 			= {};
page_table __attribute__((aligned(4096))) k_first_table 	= {};
page_table __attribute__((aligned(4096))) k_sec_table 	= {};

p_map_level4_table* kgetPML4T(void) {
	return &kpml4;
};



/* 
 * In order to map things a bit more seamlessly, I decided it was a good idea to
 * reserve a portion of memory for dynamically allocating paging structures 
 * (PML4T, PDPT, PD etc.).
 * 
 * This has the benefit that it is easy to determine their physical addresses, so less
 * effort is necessary there, and it is also easier to guarantee that the allocated sturctures
 * will be page-aligned.
 * 
 * These couple global variables are used for that purpose.
 * 
 * Here's how it works: 
 * Firstly, this is a "heap" of fixed-size blocks (each block contains sizeof(page_struct) bytes).
 * Each block begins at the next page-aligned address after the end of the previous block.
 * The first two pages are reserved for a bitmap, keeping track of what blocks are used 
 * and which are free.
 * So  here's how an example would look:
 * Page 0x0 	-> Bitmap
 * Page 0x1 	-> Bitmap
 * Page 0x2 	-> Block1's beginning
 * Page 0x5 	-> Block2's beginning
 * Page 0x8 	-> Block2's beginning
 * 		.
 * 		.
 * 		.
 * page 0x1F8	-> Last block.
 * 
 * TODO: Merging this with the actual heap would be great.
 */
uint64_t page_heap_begin;			/* Beginning address. */
uint64_t page_heap_phys;			/* Beginning address. */
uint64_t page_heap_bitmap_length;	/* Amount of bytes the bitmap occcupies. */ 
uint64_t page_heap_length;			/* Length, in bytes.  */
uint64_t *page_heap_bitmap;		
struct page_struct *page_heap_first;


struct page_struct *alloc_page_struct(void) {
	for (size_t i = 0; i < (page_heap_bitmap_length * 8) ; i++) {
		uint64_t byte8 = page_heap_bitmap[i/64];
		uint64_t bit   = i % 64;
		if (byte8 & (1 << bit)) {
			/* It is used. */
			continue;
		}
		
		/* Mark it as used and return it. */
		byte8 = byte8 | (1 << bit);
		page_heap_bitmap[i/64] = byte8;
		
		struct page_struct *ps = (struct page_struct*)(page_heap_begin + page_heap_bitmap_length + i * 0x3000);
		uintptr_t ps_addr = (uintptr_t)ps;
		
		memset(ps, 0, sizeof(ps));
		ps->physical_address = (ps_addr - page_heap_begin) + page_heap_phys;
		
		return ps;
	}
	return NULL;
};

void free_page_struct(struct page_struct *ps) {
	struct page_struct *pi = page_heap_first;
	for (size_t i = 0; i < (page_heap_bitmap_length * 8) ; i++) {
		if (pi == ps) {
			/* Found its index. */
			uint64_t byte8 = page_heap_bitmap[i/64];
			uint64_t bit = i % 64;
			
			byte8 = byte8 & (0xFFFFFFFFFFFFFFFF ^ (1 << bit));
			break;
			
		}
		pi++;
	}
	return;
};




uint8_t map_memory(uint64_t pa, uint64_t va, uint64_t amount, p_map_level4_table* pml4t) {
	/* This function maps an arbitrary amount of continous physical pages to virtual ones. 
	 * A couple things to keep in mind:
	 * 	This function does not check or care whether the physical page is valid.
	 * 	This function does not check or care whether the physical page is being used.
	 * 	This function does not set the 'used' bit of the physical page.
	 * 
	 * Truth is, it is not supposed to do any of those things. This function only does what
	 * it's supposed to do. And because of that, generally, you want to use some of the other
	 * functions.
	 * 
	 * Also a note: Currently this function (if it can't find the necessary PDPT/PD/PT in place)
	 * allocates *MORE* page structs for that pml4t. This will most likely change in the future,
	 * but currently this is the case.
	 */
	

	 
	if (pml4t == NULL) {
		return 1;	
	} 
	
	/* Zero out the first 12 bits. */
	pa &= 0xFFFFFFFFFFFFF000;	
	va &= 0x0000FFFFFFFFF000;
	
	
	uint64_t pml4t_index 	= va / 0x8000000000;
	pd_ptr_table* pdpt 	= pml4t->child[pml4t_index];
	if (pdpt == NULL) {
		/* These NULL checks simply allocate more page structures if they are not found. */
		struct page_struct *news = alloc_page_struct();
		pml4t->child[pml4t_index] = news;
		pml4t->entries[pml4t_index] = news->physical_address | 2 | 1;
		pdpt = news;
	}

	uint64_t pdpt_index  	= (va % 0x8000000000) / 0x40000000;
	page_dir* pd 		= pdpt->child[pdpt_index];
	if (pd == NULL) {
		struct page_struct *news = alloc_page_struct();
		pdpt->child[pdpt_index] = news;
		pdpt->entries[pdpt_index] = news->physical_address | 2 | 1;
		pd = news;
	}

	uint64_t pd_index 		= (va % 0x40000000) / 0x200000;
	page_table* pt		= pd->child[pd_index];
	if (pt == NULL) {
		struct page_struct *news = alloc_page_struct();
		page_table *newt;
		newt = (page_table*)news;
		newt->physical_address = news->physical_address;
		
		pd->child[pd_index] = newt;
		pd->entries[pd_index] = newt->physical_address | 2 | 1;
		pt = newt;
	}

	uint64_t pt_index		= (va % 0x200000) / 0x1000;
	
	
	
	for (uint64_t i = 0; i < amount; i++) {
		if (((va/0x1000) % 512) == 0) {
			/* We crossed tables/structs, and we need to recalculate every index. */
			pml4t_index 	= va / 0x8000000000;
			pdpt 	= pml4t->child[pml4t_index];
			if (pdpt == NULL) {
				struct page_struct *news = alloc_page_struct();
				pml4t->child[pml4t_index] = news;
				pml4t->entries[pml4t_index] = news->physical_address | 2 | 1;
				pdpt = news;
			}
		
			pdpt_index  	= (va % 0x8000000000) / 0x40000000;
			pd 		= pdpt->child[pdpt_index];
			if (pd == NULL) {
				struct page_struct *news = alloc_page_struct();
				pdpt->child[pdpt_index] = news;
				pdpt->entries[pdpt_index] = news->physical_address | 2 | 1;
				pd = news;
			}
		
			pd_index 		= (va % 0x40000000) / 0x200000;
			pt		= pd->child[pd_index];
			if (pt == NULL) {
				struct page_struct *news = alloc_page_struct();
				page_table *newt;
				newt = (page_table*)news;
				newt->physical_address = news->physical_address;
				
				pd->child[pd_index] = newt;
				pd->entries[pd_index] = newt->physical_address | 2 | 1;
				pt = newt;
			}
		}
		
		/* The page table index needs to be "recalculated" every time. */
		pt_index		= (va % 0x200000) / 0x1000;
		pt->entries[pt_index] = (pa + i * 0x1000) | 2 | 1;
		
		va += 0x1000;
	}
	
	
	return 0;
};

uint8_t unmap_memory(uint64_t va, uint64_t amount, p_map_level4_table* pml4t) {
	/* 
	 * This function unmaps virtual pages. 
	 * It works with the same logic as map_memory, except instead of setting the entry
	 * to physical address ORed with 3, it sets the entry to 0.
	 * 
	 * Also, it doesn't allocate any more memory for the page structs.
	 */
	 if (pml4t == NULL) {
		return 1;	
	} 
	
	/* Zero out the first 12 bits. */
	va &= 0x0000FFFFFFFFF000;
	
	
	uint64_t pml4t_index 	= va / 0x8000000000;
	pd_ptr_table* pdpt 	= pml4t->child[pml4t_index];
	if (pdpt == NULL) {
		return 1;
	}

	uint64_t pdpt_index  	= (va % 0x8000000000) / 0x40000000;
	page_dir* pd 		= pdpt->child[pdpt_index];
	if (pd == NULL) {
		return 1;
	}

	uint64_t pd_index 		= (va % 0x40000000) / 0x200000;
	page_table* pt		= pd->child[pd_index];
	if (pt == NULL) {
		return 1;
	}

	uint64_t pt_index		= (va % 0x200000) / 0x1000;
	
	
	
	for (uint64_t i = 0; i < amount; i++) {
		pml4t_index 	= va / 0x8000000000;
		pdpt 	= pml4t->child[pml4t_index];
		if (pdpt == NULL) {
			continue;
		}
	
		pdpt_index  	= (va % 0x8000000000) / 0x40000000;
		pd 		= pdpt->child[pdpt_index];
		if (pd == NULL) {
			continue;
		}
	
		pd_index 		= (va % 0x40000000) / 0x200000;
		pt		= pd->child[pd_index];
		if (pt == NULL) {
			continue;
		}

		pt_index		= (va % 0x200000) / 0x1000;


		pt->entries[pt_index] = 0;
		
		va += 0x1000;
	}
	
	
	return 0;
	 
};

uint8_t init_vmm(void) {
	kpml4.child[511] 		= &k_first_pdpt;
	k_first_pdpt.child[510]	= &k_first_pd;
	k_first_pd.child[0]		= &k_first_table;
	k_first_pd.child[1]		= &k_sec_table;
	
	/* 
	 * Set up the tables, and also record their physical addresses. 
	 * Divide kernel_virt_base by 8 because a single entry occupies 8 bytes.
	 */
	kpml4.physical_address			= (uintptr_t)kpml4.entries - kernel_virt_base;
	kpml4.entries[511] 				= ((uint64_t)k_first_pdpt.entries   - kernel_virt_base) | 2 | 1;
	
	k_first_pdpt.physical_address 	= (uintptr_t)k_first_pdpt.entries  - kernel_virt_base;
	k_first_pdpt.entries[510] 		= ((uint64_t)k_first_pd.entries     - kernel_virt_base) | 2 | 1; 
	
	k_first_pd.physical_address		= (uintptr_t)k_first_pd.entries    - kernel_virt_base; 
	k_first_pd.entries[0] 			= ((uint64_t)k_first_table.entries  - kernel_virt_base) | 2 | 1; 
	k_first_pd.entries[1] 			= ((uint64_t)k_sec_table.entries  - kernel_virt_base) | 2 | 1; 
	
	
	k_first_table.physical_address	= (uintptr_t)k_first_table.entries - kernel_virt_base;
	k_sec_table.physical_address	= (uintptr_t)k_sec_table.entries - kernel_virt_base;
	
	/* Map the pages the kernel is on. */
	if (map_memory(kernel_phys_base, kernel_virt_base + kernel_phys_base, 0x200, &kpml4)) {
		while (1) {
			asm volatile ("hlt;");	/* Halt if we can't map everything properly. */
		}
	}
	
	
	
	
	/* Map the heap-like structure we will use to allocate more page structs. */
	uint64_t pp_count = 512;	
	uint64_t base_pp = allocpps(pp_count);
	if (base_pp) {
		map_memory(page_to_addr(base_pp), kernel_virt_base, pp_count, &kpml4);
		
		page_heap_begin = kernel_virt_base;
		page_heap_phys = page_to_addr(base_pp);
		page_heap_length = 0x200000;
		
		page_heap_bitmap = (uint64_t*) page_heap_begin;
		page_heap_bitmap_length = 0x2000;	/* Two pages. */
		
		for (size_t i = 0; i < page_heap_bitmap_length; i++) {
			page_heap_bitmap[i] = 0;
		}
		
		page_heap_first = (struct page_struct*) (page_heap_begin + page_heap_bitmap_length);
		
	}
	
	
	/* Load the new page tables. */
	loadPML4T((uint64_t*)kpml4.physical_address);
	
	return GENERIC_SUCCESS;
}
