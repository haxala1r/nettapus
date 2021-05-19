#ifndef _MEM_H
#define _MEM_H 1


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stivale2.h>

/*
 * Loading a Page-Map Level 4 Table is one of the rare things we can't do in C. Because
 * of this, they are defined in an assembly file.
 */
extern void  loadPML4T(volatile uint64_t*);


/* kernel_virt_base + kernel_phys_base == the beginning of the kernel binary. */
static uint64_t kernel_virt_base = 0xffffffff80000000;
static uint64_t kernel_phys_base = 0x200000;

/* The actual virtual addresses the kernel starts and ends at */
extern char* kernel_start;
extern char* kernel_end;





//PMM stuff.
struct memory_block {
	uint64_t base_page;  	//Page number it starts at. Stored as a page number for convenience.
	uint64_t length;		//Length in pages. Also equal to the amount of pages.

};


struct memory_map {
	struct memory_block blocks[32]; 		//32 blocks at most.
	uint64_t num_blocks;						//total number of blocks.
	uint64_t bitmap[98304];					//This is equal to 24 GB
}; //lists total available physical memory and provides bitmaps for each of them.

typedef struct memory_block memory_block_t;
typedef struct memory_map memory_map_t;







/*
 * VMM.
 *
 * There is only one structure that is used for PD, PDPT and PML4T.
 * This is because even though they're supposed to be different things, they all have
 * the same structure (512 uint64_t's). And thus, declaring multiple structs is unnecessary,
 * as they will all contain the same things anyway. This may change later on.
 */
struct page_struct {
	volatile uint64_t entries[512] __attribute__((aligned(4096))); //at most 1024 entries.

	/*
	 * This stores all the lower-level tables' virtual addresses, so that we can actually
	 * change them more easily. This is simply not used for a page table, so it has its own struct,
	 * to avoid wasting memory.
	 */
	void* child[512];

	/* This is stored here for convenience. */
	uintptr_t physical_address;
}	__attribute__((aligned(4096))) __attribute__((packed));




/* Page tables, however, have their own struct. This is simply because they are the lowest
 * tables in the hierarchy, and thus don't need the lower[] field.
 */
struct page_table {
	volatile uint64_t entries[512] __attribute__((aligned(4096)));

	uintptr_t physical_address;
} __attribute__((aligned(4096))) __attribute__((packed));




typedef struct page_table page_table;
typedef struct page_struct page_dir;
typedef struct page_struct pd_ptr_table;
typedef struct page_struct p_map_level4_table;



/* This struct stores information about a chunk in the heap. */
struct chunk_header {
	/* This is the size of the chunk's "data section" and thus excludes the header itself.
	 * That means you need to do a " + sizeof(chunk_header_t)" to determine the actual size
	 * a chunk occupies, along with its header.
	 */
	uint32_t size;
	struct chunk_header *prev;
	struct chunk_header *next;
	/* Right here is where the data section of the chunk goes.
	 * You can simply do a "ptr + 1" to get to the data section.
	 */
} __attribute__((packed));



struct heap {
	/* This points to the first free block in the heap. Chunks are in a doubly-linked list.
	 * Keep in mind that the list excludes used chunks.
	 */
	struct chunk_header* first_free;
	uint64_t min_size;	//minimum size of a chunk.
	uint64_t start;	//starting address of the heap.
	uint64_t end;	//ending address of the heap.
};


typedef struct chunk_header chunk_header_t;
typedef struct heap heap_t;


//generic things.
memory_map_t* getPhysicalMem();
p_map_level4_table* kgetPML4T();
heap_t* kgetHeap();		//returns a pointer to kernel's heap.
uint64_t page_to_addr(uint64_t);	//tells you at which address a page starts. doesn't check the validity of the page.
uint64_t addr_to_page(uint64_t);	//tells you at which page the address resides in. doesn't check the validity of the address.
void _create_block(uint64_t, uint64_t, memory_block_t*);	//internal function. creates a block object from address and length.
void krefresh_vmm();

//Physical memory management.
uint8_t ispaValid(uint64_t);	//tells whether an address is valid or not.(physical addresses). returns 1 if it is.
uint8_t isppValid(uint64_t);	//tells you whether or not a physical page number is valid. returns 1 if it is.
uint8_t isppUsed(uint64_t);	//tells you whether or not a physical page is currently in use.
uint8_t ispaUsed(uint64_t);//tells you whether or not a physical address is currently in use. 1 = used, 0 = free

uint8_t setppUsed(uint64_t, uint8_t);		//sets a page's 'used' bit to whatever you pass. 1 = used, 0 = free.

uint64_t allocpp();		//"allocates" a single physical page.
uint64_t allocpps(uint64_t);		//"allocates" multiple physical pages..
uint8_t freepp(uint64_t);			//"frees" a single physical page.




//Virtual memory management

uint8_t map_memory(uint64_t phys, uint64_t virt, uint64_t amount, p_map_level4_table*);	//maps a single physical page to a virtual page. doesn't check if pp is avilable.
uint8_t unmap_page(uint64_t virt, uint64_t amount, p_map_level4_table*);	//unmaps a virtual page, also frees the physical page attached to it.

/* Allocates a random physical page and a random virtual one. Starting address is returned. */
uint64_t alloc_pages(uint64_t amount, uint64_t base, uint64_t limit);




//Heap (TM) management. (it's kinda trash, but it works okay i guess? though that could be said about everything I write.)
uint8_t unlink(chunk_header_t*);	//unlinks a chunk from the linked list
uint8_t link(chunk_header_t*, chunk_header_t*);	//links two chunks to each other.
void* kmalloc(uint64_t);
uint8_t kfree(void*);



//these  functions simply initalise different layers of the Memory Manager (TM)
uint8_t init_pmm(struct stivale2_struct_tag_memmap*);		//Physical Memory Manager (TM)
uint8_t init_vmm();											//Virtual  Memory Manager (TM)
uint8_t init_heap();										//Heap			  Manager (TM)

//initialises everything (a.k.a. calls the three functions declared above.)
uint8_t init_memory(struct stivale2_struct_tag_memmap*);




#ifdef DEBUG

void heap_print_state();

#endif	/* DEBUG */



#ifdef __cplusplus
}
#endif

#endif /* _MEM_H*/
