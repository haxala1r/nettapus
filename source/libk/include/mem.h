#ifndef _MEM_H
#define _MEM_H 1

#ifndef GENERIC_SUCCESS
#define GENERIC_SUCCESS 0
#endif


#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <multiboot.h>
//these should be defined globally in boot.S
//simply because I can't get 'em to work 'ere.
//they're only here because other functions may depend on them
extern void loadPageDirectory(uint32_t*);
extern void enablePaging();
//extern void disablePaging();	this doesn't exist. you can't run away from paging. you either enable it or you don't.

//keep in mind the *address* of these variables indicate the start and end of the kernel.
extern uint32_t kernel_start;
extern uint32_t kernel_end;

//the rest is fine though.



//structs and types for paging stuff.

struct memory_block {
	uint32_t base_page;  	//page number it starts at. Each page is 4KB
	uint32_t length;		//length in pages.
};


struct memory_map {
	struct memory_block blocks[32]; 	//32 blocks at most.
	uint32_t bitmap[32768];					//bitmap for the entire memory.
	size_t num_blocks;					//total number of blocks.
}; //lists total available physical memory and provides bitmaps for each of them.








struct page_directory {
	uint32_t dir[1024] __attribute__((aligned(4096))); //at most 1024 entries.
}	__attribute__((aligned(4096))) __attribute__((packed));






//fot he heap. stores crucial information about a chunk.
struct chunk_header {
	//first bit of this tells you whether or not it is in use. 1=used, 0=free.
	//also this is the size of its "data section" and thus excludes the header itself.
	//meaning you need to do a " + sizeof(chunk_header_t)" to determine the actual size
	//a chunk occupies.
	uint32_t size;	
	struct chunk_header *prev;
	struct chunk_header *next;
	//right here is where the data section of the chunk goes.
	//simply do a "ptr + sizeof(*ptr)" to get to the data section.
} __attribute__((packed));



struct heap {
	struct chunk_header* first_free;	//point to the first free block.
	uint32_t min_size;	//minimum size of a chunk. I like to use something like 16.
	uint32_t start;	//starting address of the heap.
	uint32_t end;	//ending address of the heap.
};




//now we make types out of these structs.
typedef struct page_directory page_directory_t;


typedef struct memory_block memory_block_t;
typedef struct memory_map memory_map_t;

typedef struct chunk_header chunk_header_t;
typedef struct heap heap_t;


//generic things.
memory_map_t* getPhysicalMem();
page_directory_t* kgetPageDir();
heap_t* kgetHeap();		//returns a pointer to kernel's heap.
uint32_t page_to_addr(uint32_t);	//tells you at which address a page starts. doesn't check the validity of the page.
uint32_t addr_to_page(uint32_t);	//tells you at which page the address resides in. doesn't check the validity of the address.
memory_block_t _create_block(uint32_t, uint32_t);	//internal function. creates a block object from address and length.

void initPageDir(page_directory_t*);		//initialises a page directory with its tables. kinda inefficient, might change this later.


//Physical memory management.
uint8_t ispaValid(uint32_t);	//tells whether an address is valid or not.(physical addresses). returns 1 if it is.
uint8_t isppValid(uint32_t);	//tells you whether or not a physical page number is valid. returns 1 if it is.
uint8_t isppUsed(uint32_t);	//tells you whether or not a physical page is currently in use.
uint8_t ispaUsed(uint32_t);//tells you whether or not a physical address is currently in use. 1 = used, 0 = free

uint8_t setppUsed(uint32_t, uint8_t);		//sets a page's 'used' bit to whatever you pass. 1 = used, 0 = free.

uint32_t allocpp();		//"allocates" a single physical page.
uint32_t allocpps(uint32_t);		//"allocates" multiple physical pages..
uint8_t freepp(uint32_t);			//"frees" a single physical page.




//Virtual memory management
uint8_t isvpMapped(uint32_t, page_directory_t*);	//tells whether a virtual page is currently mapped (used) in that page_directory.
uint8_t isvaMapped(uint32_t, page_directory_t*);	//tells whether a virtual address is currently mapped (used) in that page_directory. 
uint32_t findvp();	//finds a free (unmapped) virtual page.

uint8_t mapPage(uint32_t, uint32_t, page_directory_t*);	//maps a single physical page to a virtual page. doesn't check if pp is avilable.
uint8_t unmapPage(uint32_t, page_directory_t*);	//unmaps a virtual page, also frees the physical page attached to it.
uint8_t idMapPage(uint32_t, page_directory_t*);	//identity maps a page, if both the physical and the virtual pages are free.
uint8_t idMapRange(uint32_t, uint32_t, page_directory_t*);	//identity maps a range of pages. checks availability.

uint8_t kpmappv(uint32_t, uint32_t);	//(kernel-side) maps a physical page to a virtual page. uses kernel page directory.
uint8_t kmmappv(uint32_t, uint32_t);	//(kernel-side) maps a physical address to a virtual one. a wrapper around kpmappv(). not sure about its name.
	
	
uint8_t kpmapv(uint32_t);	//maps a random physical page to a specified virtual page.
uint8_t kmmapv(uint32_t);	//maps a random physical page to a specified virtual address.(first 12 bits ignored.)

	
uint32_t kpmap();	//maps a random physical page to a random virtual page, and returns its starting address.
uint32_t kpumap(uint32_t);	//unmaps a page from the kernel page directory.


//Heap (TM) management. (it's kinda trash, but it works okay?)
uint8_t unlink(chunk_header_t*);	//unlinks a chunk from the linked list
uint8_t link(chunk_header_t*, chunk_header_t*);	//links two chunks to each other.




//these  functions simply initalise different layers of the Memory Manager (TM)
uint8_t init_pmm(struct multiboot_header*);		//Physical Memory Manager (TM)
uint8_t init_vmm();								//Virtual  Memory Manager (TM)
uint8_t init_heap();							//Heap			  Manager (TM)

//initialises everything (a.k.a. calls the three functions declared above.)
void init_memory(struct multiboot_header*);	





#ifdef __cplusplus
}
#endif




#endif

