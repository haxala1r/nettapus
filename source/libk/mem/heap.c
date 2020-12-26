
//this file contains the Heap Management (TM) parts of the Memory Manager (TM) of my hobby OS (TM)
//for Virtual Memory Management (TM) and Physical Memory Management (TM) look at the other files in the same directory.

#include <mem.h>

//note that when it comes to the heap, we generally don't want stuff to be
//ID mapped (at least its not a requirement). So we're generally just relying on
//the Physical and Virtual Memory Managers (TMTMTM) to get us some free pages.
//note that we do have a requirement that the virtual address we use is definitely
//continuous. this would work a lot better if this was a higher half kernel, but
//unfortunately I DON'T HAVE A F###ING CLUE HOW TO DO THAT
//so yeah, we'll just make do with this I guess.

//this heap implementation I came up with (aka read the osdev wiki about)
//works by having all free chunks in a doubly linked list, and
//unlinking a chunk whenever we need to allocate some memory.

//then the act of allocating simply becomes looping over the free chunks
//and finding (and chopping if necessary) a suitable chunk.

//the act of freeing simply becomes adding the chunk back to the linked list,
//and merging adjacent chunks if you find any. As I said, this implementation

//is kinda slow, and it definitely isn't optimized, but it works,
//and it is good enough for now.


//this is the heap object we're using as default for the kernel.
//I wrapped stuff into a heap object to make it so that every process
//can have its own heap, and I can still access them without going through hell.
heap_t kheap_default;



//this exists to make things easier to refactor in case I want to rename the variable.
//and also maybe to make it easier to add a secondary heap reserved for 
//emergencies and stuff. IDK, that's what I could think of.
heap_t *kgetHeap() {
	return &kheap_default;
}



uint8_t unlink(chunk_header_t* h) {
	//unlinks a chunk.
	if (h == NULL) {
		return 1;	//error on NULL.
	}
	if (h->next != NULL) {
		//after all if the next entry is NULL we can't do much.
		h->next->prev = h->prev;
	}
	if (h->prev != NULL) {
		//same applies for the previous entry.
		h->prev->next = h->next;
	}
	//to ensure things can't go wrong because of leftover data.
	//kinda unnecessary, might get rid of this part later.
	h->prev = NULL;
	h->next = NULL;
	return GENERIC_SUCCESS;
}


uint8_t link(chunk_header_t* h1, chunk_header_t* h2) {
	if ((h1 == NULL) || (h2 == NULL)) {
		return 1;	//error on NULL value.
	}
	//keep in mind this function does not automatically unlink- it *just* links.
	//this also assumes "h1 < h2" evaluates to true.
	h1->next = h2;
	h2->prev = h1;
	
	return GENERIC_SUCCESS;
}



chunk_header_t* divideChunk(uint32_t size, chunk_header_t* chunk) {
	if (chunk == NULL) {
		return NULL;
	}
	if (size < 16) {
		//minimum size of a chunk. since we wouldn't want to store
		//12 bytes of metadata for a single fucking byte.
		size = 16;		
	}
	if (size % 2 != 0) {
		size += 1;	
		//rounds up in case of odd numbers. I just don't want
		//to see chunks that start at 0x12E357 y'know? it just doesn't look right.
	}
	if (chunk->size <= (size + sizeof(chunk_header_t) + 16)) {
		//this ssimply makes sure that there is enough space within the
		//first chunk so that we can actually divide it.
		//since if there isn't enough space we might overwrite some important stuff,
		//or we might cause a page fault.
		//so yeah, this is important. also 16 is the minimum size of a chunk,
		//if you're wondering.
		return NULL;
	}
	

	
	//we're beasically taking the memory of the chunk, and splitting it.
	//it shouldn't be hard to understand, but it somehow is.
	chunk_header_t *c1 = chunk;
	chunk_header_t *c2;
	
	
	c2 = (chunk_header_t*)((uint32_t)chunk + size + sizeof(chunk_header_t));
	
	
	c2->size = chunk->size - (size + sizeof(chunk_header_t));
	if (chunk->next != NULL) {
		//we have to update the surrounding chunks as well. 
		//otherwise, everything could go downhill real quick.
		chunk->next->prev = c2;		
	}
	c2->next = chunk->next;
	
	c2->prev = c1;
	
	
	c1->size = size;
	c1->next = c2;
	return c2;
}

chunk_header_t* mergeChunk(chunk_header_t* h1, chunk_header_t* h2) {
	if ((h1 == NULL) || (h2 == NULL)) {
		return NULL;	//can't merge NULL pointers.
	}
	//this assumes both chunks are free, and it does not check or care
	//whether the chunks are in use. also keep in mind that h2 must come
	//*directly after* h1, it does not work the other way around.
	//I might add a conditional to detect if it is the other way around
	//later, but for now that's how it works.
	if ((((uint32_t)h1) + h1->size + sizeof(chunk_header_t)) != ((uint32_t)h2)) {
		//if h2 does not come directly  after h1, then there ain't much
		//we can do.
		return NULL;
	}
	//we gotta make it so that the other chunks also think that h2 does not exist.
	//because it doesn't anymore. it's a part of h1 now. it's gone.
	h1->next = h2->next;
	if (h2->next != NULL) {
		h2->next->prev = h1;
	}
	
	//now we can update size and stuff.
	h1->size = h1->size + h2->size + sizeof(chunk_header_t);
	
	//zero out h2. this ain't really necessary, might remove it.
	memset(h2, 0, sizeof(chunk_header_t));
	
	//we should be done! now return h1 so that you can chain this with other 
	//functions.
	return h1;
}




//now comes the legendary malloc and free!
void *kmalloc(uint32_t bytes) {
	if (bytes == 0) {
		return NULL;	//NULL if you want 0 bytes. what did you expect?
	}
	//for any value above zero (literally every value), we round it up to
	//a multiple of four, because I want things to be aligned, y'know?
	if ((bytes % 4) != 0) {
		bytes += 4 - (bytes % 4);
	}
	
	//now we loop over the list of free chunks. this uses the kernel heap.
	heap_t* hp = kgetHeap();
	chunk_header_t* chunk = hp->first_free;
	
	while (chunk != NULL) {
		if (chunk->size < bytes) {
			//if the chunk's size is insufficient, then don't bother with it.
			
			//TODO: make it so that this algorithm can also merge chunks
			//so that extremely small chunks that were requested do not go to waste.
			//kfree() does already merge chunks as it goes, so this might be unnecessary,
			//considering the size of the heap, but it might become important in some cases,
			//so I should probably implement that sometime.
			
			
			chunk = chunk->next;
			continue;
		} else if (chunk->size == bytes) {
			//if the size somehow matches prefectly, just return that chunk.
			unlink(chunk);
			
			//this part could probably be made to look less insane, but whatever.
			return (void*)(((uint32_t)chunk) + sizeof(chunk_header_t));
		} else {
			//chunk's size must be bigger than the size requested.
			//if the chunk's size is significantly bigger, we should just chop it up.
			//if not, we can afford to have a couple bytes of waste.
			//I should probably change this to a more clever algorithm,
			//but for now this should suffice.
			if ((chunk->size - bytes) >= 32) {
				//if difference >= 32, chop the chunk to be as big as requested,
				//then return it.
				
				
				//keep in mind if we chop this up, we must also update the
				//next and previous chunks so that the new-born chunk
				//is recognized as a proper free chunk.
				
				chunk_header_t* newchunk = divideChunk(bytes, chunk);	//returns the second chunk, look at the function.
				if (chunk->prev != NULL) {
					chunk->prev->next = newchunk;
				}
				if (chunk->next != NULL) {
					chunk->next->prev = newchunk;
				}
				
				//UPDATE: We must also update first_free if this is the first
				//chunk we encountered.
				if (chunk == hp->first_free) {
					hp->first_free = newchunk;
				}
				
				
				//chunk should now hold the chunk with the requested size,
				//and should be ready-for-use.
				
				//again, this part could probably be made to look less insane, but whatever.
				return (void*)(((uint32_t)chunk) + sizeof(chunk_header_t));
				
			} else {
				//if the difference isn't that big, just return the chunk without chopping.
				unlink(chunk);
				
				//again again, this part could probably be made to look less insane, but whatever.
				return (void*)(((uint32_t)chunk) + sizeof(chunk_header_t));
			}
			
			
			
		}
	}
	//if it reaches here, we must have gone through the entire heap, 
	//and there was no matching chunks. return NULL in this case.
	
	return NULL;
}


uint8_t kfree(void *ptr) {
	if (ptr == NULL) {
		//in this case: what do you even expect from me?
		return 1;
	}
	//I'm going to be using two variables to loop over free chunks,
	//in order to find where this chunk we're freeing should be placed.
	//we're also going to be merging adjacent free chunks if we find any along the way.
	//this is to make sure the heap doesn't get too fragmented.
	
	chunk_header_t* chunk = (chunk_header_t*)((uint32_t)ptr - sizeof(chunk_header_t));
	
	heap_t* hp = kgetHeap();

	
	chunk_header_t* i = hp->first_free;
	chunk_header_t* j = i->next;
	
	if (chunk < i) {
		//since i is the first free chunk, then we must add this chunk
		//to the beginning of the linked list.
		chunk->prev = NULL;
		chunk->next = i;
		i->prev = chunk;
		
		//gotta update stuff.
		hp->first_free = chunk;
		
		return GENERIC_SUCCESS;	//we succeeded already.
	}
	
	
	while (1) {
		if (i == NULL) {
			//if it reaches here, then dafuq?
			return 1;	//might use a more descriptive error.
		}
		if (j == NULL) {
			//we are at the end of the list, and i holds the last element.
			//this means ptr belongs to the last chunk so we gotta 
			//add it to the end of the linked list.
			i->next = chunk;
			chunk->prev = i;
			chunk->next = NULL;
			return GENERIC_SUCCESS;
		}
		
		//this also merges some chunks if it finds any adjacent chunks while searching for matches
		if ((((uint32_t)i) + sizeof(chunk_header_t) + i->size) == ((uint32_t)j)) {
			//update stuff, merge the chunks then re-do the iteration.
			chunk_header_t* temp = j->next;
			mergeChunk(i, j);
			j = temp;	
			continue;
		}
		
		
		
		
		if ((chunk > i) && (chunk < j)) {
			//if chunk is between i and j, then we just found exactly where
			//we need to put chunk in, hurray!
			i->next = chunk;
			chunk->prev = i;
			
			j->prev = chunk;
			chunk->next = j;
			//we should be done now.
			return GENERIC_SUCCESS;
		}
		
		
		
		
		//now skip to the next iteration. this repeats until a match is found.
		i = j;
		j = j->next;
	}
	return 1;	//error if we cannot find a match.
}

//allocates more pages for heap usage. Takes argument in bytes,
//rounded up to multiple of 4096 (0x1000) because of paging.
uint8_t kenlargeHeap(uint32_t amount /*in bytes*/) {
	uint32_t amount_pages = addr_to_page(amount) + 1;	//rounds up.
	
	heap_t *hp = kgetHeap();
	//if no heap is present yet, then make one by allocating literally a single page.
	//note: type casts are generally there to shut the compiler up.
	if ((void*)hp->start == NULL) {
		return 1;
		/* YOU SHOULD ALWAYS SET UP THE 
		   FIRST PAGE OF THE HEAP YOURSELF. */
	}
	
	
	//now continue as usual, and try to allocate as many pages as needed
	//starting from the end of the current heap (in the virtual address space.)
	
	for (size_t i = 0; i < amount_pages; i++) {
		if ((void*)hp->end == NULL) {
			return 1;	//then we got a huge fucking problem. I should probably handle this, but fuck that.
		}
		if (!kmmapv(hp->end)) {
			//if successful, then we gotta increment HEAP_END
			hp->end += 0x1000;
		} else {
			//if not successful, then we can't do much anymore.
			//simply return an error.
			return 1;
		}
	}
	//if it reaches here, then it must have succeded in enlarging the heap.
	return GENERIC_SUCCESS;
}

uint8_t init_heap() {
	//this should be called before anything else, mostly because it initialises
	//important variables and as such, should be called in any custom "init"
	//functions you can make. the original "init_memory" already calls this,
	//if you haven't edited it out.
	
	
	
	//allocates some space for the initial heap.
	
	//we first gotta make the inital heap, then we'll enlarge it.
	if (kmmapv(0xC0400000)) { //because I believe this address would be the best to start the heap at.
		return 1;
	}	
	kheap_default.start = 0xC0400000;	


	if ((void*)kheap_default.start == NULL) {
		//then kpmap() must have failed. we don't have much to do here then, 
		//so just return an error.
		return 1;
	}	
	kheap_default.end = kheap_default.start + 0x1000;	
	
	
	
	//okay, done. Now we enlarge it using this function. kmalloc will try to enlarge
	//the heap when necessary, so this number does not really matter.
	kenlargeHeap(page_to_addr(50));	//page_to_addr can be used to translate pages to bytes as well.	
	
	//now we make the first chunk, "the wilderness" as some would say.
	//whenever we need a chunk, we will simply chop a part of this chunk,
	//and hand it to whomever needs it. (or just hand out another free chunk
	//if there is a suitable one)
	
	kheap_default.first_free = (chunk_header_t*)kheap_default.start;
	
	//only count the size of the data it can hold. this is to make it easier to compare
	//sizes. though it can get confusing at times, so beware.
	kheap_default.first_free->size = (kheap_default.end - kheap_default.start) - sizeof(chunk_header_t);	
	
	//since this is literally the only chunk that exists for now, 
	//the previous and the next values must be set to NULL.
	kheap_default.first_free->prev = NULL;	
	kheap_default.first_free->next = NULL;
	
	//the heap should be fully functional now! all you need to do is simply
	//use kmalloc and kfree, which will take care of most stuff for you.
	
	
	return GENERIC_SUCCESS;
}


