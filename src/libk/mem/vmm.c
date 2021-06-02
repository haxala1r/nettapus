
/* This file contains the Virtual Memory Manager. (TM)*/

#include <mem.h>
#include <err.h>


/* Define the first structs, and initialise them with all zeroes. Necessary to bootstrap. */
p_map_level4_table __attribute__((aligned(4096))) kpml4;
pd_ptr_table __attribute__((aligned(4096))) k_first_pdpt;
page_dir __attribute__((aligned(4096))) k_first_pd;
page_table __attribute__((aligned(4096))) k_first_table;
page_table __attribute__((aligned(4096))) k_sec_table;

p_map_level4_table *kgetPML4T(void) {
	return &kpml4;
}

/* The kernel occupies only a single PDPT, and is forever doomed to live there.
 * Every task has the Kernel's PDPT mapped into its address space. This allows
 * system calls, interrupts etc to function properly.
 */
pd_ptr_table *kgetPDPT(void) {
	return &k_first_pdpt;
}

void krefresh_vmm(void) {
	loadPML4T((uint64_t*)kpml4.physical_address);
}

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
uint64_t page_heap_phys;			/* Beginning physical address. */
uint64_t page_heap_bitmap_length;	/* Amount of bytes the bitmap occcupies. */
uint64_t page_heap_length;			/* Length, in bytes.  */
uint64_t *page_heap_bitmap;
struct page_struct *page_heap_first;


struct page_struct *alloc_page_struct(void) {
	for (size_t i = 0; i < (page_heap_bitmap_length * 8) ; i++) {
		uint64_t byte8 = page_heap_bitmap[i/64];
		uint64_t bit   = i % 64;
		if (byte8 & ((uint64_t)1 << bit)) {
			/* It is used. */
			continue;
		}

		/* Mark it as used and return it. */
		byte8 = byte8 | ((uint64_t)1 << bit);
		page_heap_bitmap[i/64] = byte8;

		struct page_struct *ps = (struct page_struct*)(page_heap_begin + page_heap_bitmap_length + i * 0x3000);
		uintptr_t ps_addr = (uintptr_t)ps;

		memset(ps, 0, sizeof(*ps));
		ps->physical_address = (ps_addr - page_heap_begin) + page_heap_phys;

		return ps;
	}
	return NULL;
}

void free_page_struct(struct page_struct *ps) {
	struct page_struct *pi = page_heap_first;
	for (size_t i = 0; i < (page_heap_bitmap_length * 8) ; i++) {
		if (pi == ps) {
			/* Found its index. */
			uint64_t byte8 = page_heap_bitmap[i/64];
			uint64_t bit = i % 64;

			byte8 = byte8 & (0xFFFFFFFFFFFFFFFF ^ ((uint64_t)1 << bit));
			break;

		}
		pi++;
	}
	return;
}





uint64_t alloc_pages(uint64_t amount, uint64_t base, uint64_t limit, size_t user_accessible) {
	/* Allocates some Physical and virtual pages and maps them.
	 * The virtual pages are guaranteed to be continous. No such thing is guaranteed for the
	 * physical pages.
	 * The virtual address will be returned. The returned address is guaranteed to be above
	 * base and below limit. If no pages were available, 0 will be returned.*/

	uint64_t i = base;
	i &= 0x0000FFFFFFFFF000;

	p_map_level4_table *pml4t = kgetPML4T();
	if (pml4t == NULL) {
		return 0;
	}

	pd_ptr_table *pdpt 	= NULL;//pml4t->child[i / 0x8000000000];
	page_dir *pd 		= NULL;//pdpt->child[(i % 0x8000000000) / 0x40000000];
	page_table *pt 		= NULL;//pd->child[(i % 0x40000000) / 0x20000];


	/* This is the variable that keeps track of how many free pages we found so far.
	 * Reset to 0 when a non-free page is found. When this equals "amount", that means
	 * we found the requested amount of pages.
	 */
	uint64_t ia = 0;


	while (1){
		if (i > limit) {
			break;
		}

		if ( (((i / 0x1000) % 512) == 0) || (i == (base & 0x0000FFFFFFFFF000))) {
			/* We either crossed a boundry, or this is the first iteration.
			 * Thus, it is necessary to recalculate addresses.*/
			pdpt	= pml4t->child[i / 0x8000000000];
			if (pdpt == NULL) 	{
				pml4t->child[i / 0x8000000000] = pdpt = alloc_page_struct();
				pml4t->entries[i / 0x8000000000] = pdpt->physical_address | 4 | 2 | 1;
			}

			pd 		= pdpt->child[(i % 0x8000000000) / 0x40000000];
			if (pd == NULL) 	{
				pdpt->child[(i % 0x8000000000) / 0x40000000] = pd = alloc_page_struct();
				pdpt->entries[(i % 0x8000000000) / 0x40000000] = pd->physical_address | 4 | 2 | 1;
			}

			pt 		= pd->child[(i % 0x40000000) / 0x20000];
			if (pt == NULL) 	{
				struct page_struct *new_table = alloc_page_struct();
				pt = (page_table* )new_table;
				pt->physical_address = new_table->physical_address;

				pd->child[(i % 0x40000000) / 0x20000] = pt;
				pd->entries[(i % 0x40000000) / 0x20000] = pt->physical_address | 4 | 2 | 1;
			}

		}

		if (pt == NULL) 	{ i += 0x1000;	i &= 0x0000FFFFFFFFF000; continue; }
		/* Check this entry. */
		if (!(pt->entries[(i % 0x200000) / 0x1000] & 1)) {
			/* Page is free. */
			ia++;
		} else {
			/* Page is used. */
			ia = 0;
		}

		/* Skip a single page. */
		i += 0x1000;
		i &= 0x0000FFFFFFFFF000;

		if (ia == amount) {
			/* We already found the requested amount of pages. We can safely map them, and
			 * return.
			 */

			/* Allocate the physical pages. */
			uint64_t base_pp = allocpps(amount);

			if (base_pp == 0) {
				break;	/* Failed allocation. */
			}

			map_memory(page_to_addr(base_pp), i - ia * 0x1000, ia, pml4t, user_accessible);
			krefresh_vmm();

			return (i - ia * 0x1000);
		}
	}

	return 0;
}

/*
uint64_t free_pages(uint64_t base, uint64_t amount) {
	THIS IS YET TO BE IMPLEMENTED.
};*/





uint8_t map_memory(uint64_t pa, uint64_t va, uint64_t amount, p_map_level4_table* pml4t, size_t user_accessible) {
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
		return ERR_INVALID_PARAM;
	}

	/* Zero out the first 12 bits. */
	pa &= 0xFFFFFFFFFFFFF000;
	va &= 0x0000FFFFFFFFF000;


	uint64_t pml4t_index 	= va / 0x8000000000;
	pd_ptr_table *pdpt 	= pml4t->child[pml4t_index];
	if (pdpt == NULL) {
		/* These NULL checks simply allocate more page structures if they are not found. */
		struct page_struct *news = alloc_page_struct();
		pml4t->child[pml4t_index] = news;
		pml4t->entries[pml4t_index] = news->physical_address | 4 | 2 | 1;
		pdpt = news;
	}

	uint64_t pdpt_index  	= (va % 0x8000000000) / 0x40000000;
	page_dir* pd 		= pdpt->child[pdpt_index];
	if (pd == NULL) {
		struct page_struct *news = alloc_page_struct();
		pdpt->child[pdpt_index] = news;
		pdpt->entries[pdpt_index] = news->physical_address | 4 | 2 | 1;
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
		pd->entries[pd_index] = newt->physical_address | 4 | 2 | 1;
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
				pml4t->entries[pml4t_index] = news->physical_address | 4 | 2 | 1;
				pdpt = news;
			}

			pdpt_index  	= (va % 0x8000000000) / 0x40000000;
			pd 		= pdpt->child[pdpt_index];
			if (pd == NULL) {
				struct page_struct *news = alloc_page_struct();
				pdpt->child[pdpt_index] = news;
				pdpt->entries[pdpt_index] = news->physical_address | 4 | 2 | 1;
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
				pd->entries[pd_index] = newt->physical_address | 4 | 2 | 1;
				pt = newt;
			}
		}

		/* The page table index needs to be "recalculated" every time. */
		pt_index		= (va % 0x200000) / 0x1000;
		if (user_accessible) {
			pt->entries[pt_index] = (pa) | 4 | 2 | 1;
		} else {
			pt->entries[pt_index] = (pa) | 2 | 1;
		}
		va += 0x1000;
		pa += 0x1000;
	}


	return (uint8_t)GENERIC_SUCCESS;
}

uint8_t unmap_memory(uint64_t va, uint64_t amount, p_map_level4_table* pml4t) {
	/*
	 * This function unmaps virtual pages.
	 * It works with the same logic as map_memory, except instead of setting the entry
	 * to physical address ORed with 3, it sets the entry to 0.
	 *
	 * Also, it doesn't allocate any more memory for the page structs.
	 */
	 if (pml4t == NULL) {
		return ERR_INVALID_PARAM;
	}

	/* Zero out the first 12 bits. */
	va &= 0x0000FFFFFFFFF000;


	uint64_t pml4t_index = va / 0x8000000000;
	pd_ptr_table* pdpt = pml4t->child[pml4t_index];
	if (pdpt == NULL) {
		return ERR_NOT_FOUND;
	}

	uint64_t pdpt_index  	= (va % 0x8000000000) / 0x40000000;
	page_dir* pd = pdpt->child[pdpt_index];
	if (pd == NULL) {
		return ERR_NOT_FOUND;
	}

	uint64_t pd_index = (va % 0x40000000) / 0x200000;
	page_table* pt = pd->child[pd_index];
	if (pt == NULL) {
		return ERR_NOT_FOUND;
	}

	uint64_t pt_index = (va % 0x200000) / 0x1000;


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

	return GENERIC_SUCCESS;
}


extern void kpanic();

uint8_t init_vmm(void) {
	kpml4.child[511] 		= &k_first_pdpt;
	k_first_pdpt.child[510]	= &k_first_pd;
	k_first_pd.child[1]		= &k_first_table;
	k_first_pd.child[128]	= &k_sec_table;

	/*
	 * Set up the tables, and also record their physical addresses.
	 * Divide kernel_virt_base by 8 because a single entry occupies 8 bytes.
	 */
	kpml4.physical_address			= (uintptr_t)kpml4.entries - kernel_virt_base;
	kpml4.entries[511] 				= ((uint64_t)k_first_pdpt.entries   - kernel_virt_base) | 2 | 1;

	k_first_pdpt.physical_address 	= (uintptr_t)k_first_pdpt.entries  - kernel_virt_base;
	k_first_pdpt.entries[510] 		= ((uint64_t)k_first_pd.entries     - kernel_virt_base) | 2 | 1;

	k_first_pd.physical_address		= (uintptr_t)k_first_pd.entries    - kernel_virt_base;
	k_first_pd.entries[1] 			= ((uint64_t)k_first_table.entries  - kernel_virt_base) | 2 | 1;
	k_first_pd.entries[128] 			= ((uint64_t)k_sec_table.entries  - kernel_virt_base)  | 2 | 1;


	k_first_table.physical_address	= (uintptr_t)k_first_table.entries - kernel_virt_base;
	k_sec_table.physical_address	= (uintptr_t)k_sec_table.entries - kernel_virt_base;

	/* Map the pages the kernel is on. */
	if (map_memory(kernel_phys_base, kernel_virt_base + kernel_phys_base, 0x200, &kpml4, 0)) {
		kpanic();
	}


	/* Map the heap-like structure we will use to allocate more page structs. */
	uint64_t pp_count = 512 * 2;
	uint64_t base_pp = allocpps(pp_count);
	if (base_pp) {
		page_heap_begin = kernel_virt_base + 0x10000000;

		/* Initially, we only map a part of the allocated pages. */
		map_memory(page_to_addr(base_pp), page_heap_begin, 128, &kpml4, 0);
		krefresh_vmm();


		page_heap_phys = page_to_addr(base_pp);
		page_heap_length = page_to_addr(pp_count);

		page_heap_bitmap = (uint64_t*) page_heap_begin;
		page_heap_bitmap_length = 0x2000;	/* Two pages. */
		memset(page_heap_bitmap, 0, page_heap_bitmap_length);

		page_heap_first = (struct page_struct*) (page_heap_begin + page_heap_bitmap_length);


		/* Now that the allocation mechanism is in place, we can map the whole thing. */
		map_memory(page_to_addr(base_pp + 128), page_heap_begin + 128 * 0x1000, pp_count - 128, &kpml4, 0);

	} else {
		kpanic();
	}

	/* Load the new page tables. */
	loadPML4T((uint64_t*)kpml4.physical_address);

	return GENERIC_SUCCESS;
}
