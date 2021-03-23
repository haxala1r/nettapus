
//this file contains the Heap Management (TM) parts of the Memory Manager (TM) of my hobby OS (TM)
//for Virtual Memory Management (TM) and Physical Memory Management (TM) look at the other files in the same directory.

#include <mem.h>
#include <err.h>


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

	/* Ensure things can't go wrong because of leftover data. */
	h->prev = NULL;
	h->next = NULL;
	return GENERIC_SUCCESS;
}


uint8_t link(chunk_header_t* h1, chunk_header_t* h2) {
	if ((h1 == NULL) || (h2 == NULL)) {
		return 1;	//error on NULL value.
	}
	/* Keep in mind this function does not automatically unlink- it *just* links.
	 * It also assumes "h1 < h2" evaluates to true.
	 */
	h1->next = h2;
	h2->prev = h1;

	return GENERIC_SUCCESS;
}



chunk_header_t* divideChunk(uint64_t size, chunk_header_t* chunk) {
	if (chunk == NULL) {
		return NULL;
	}
	if (size < 16) {
		/* Minimum size of a chunk. Because we wouldn't want to store
		 * 12 bytes of metadata for a single fucking byte.
		 */
		size = 16;
	}
	if ((size % 2) != 0) {
		/* Round up in case of odd numbers. I just don't want
		 * to see chunks that start at 0x12E357 y'know? it just doesn't look right.
		 */
		size += 1;
	}
	if (chunk->size <= (size + sizeof(chunk_header_t) + 16)) {
		/* This makes sure that there is enough space to actually split. */
		return NULL;
	}



	chunk_header_t *c1 = chunk;
	chunk_header_t *c2;


	c2 = (chunk_header_t*)(((uintptr_t)chunk) + size + sizeof(chunk_header_t));


	c2->size = chunk->size - (size + sizeof(chunk_header_t));
	if (chunk->next != NULL) {
		/* We need to update the next chunk as well. */
		chunk->next->prev = c2;
	}
	c2->next = chunk->next;

	c2->prev = c1;


	c1->size = size;
	c1->next = c2;
	return c2;
}

chunk_header_t* mergeChunk(chunk_header_t *h1, chunk_header_t *h2) {
	if ((h1 == NULL) || (h2 == NULL)) {
		return NULL;
	}
	/*
	 * This assumes both chunks are free, and it does not check or care
	 * whether the chunks are in use. Also keep in mind that h2 must come
	 * *directly after* h1, it does not work the other way around.
	 */

	if ((((uintptr_t)h1) + h1->size + sizeof(chunk_header_t)) != ((uintptr_t)h2)) {
		return NULL;
	}

	/* We also need to update the surrounding chunks. */
	h1->next = h2->next;
	if (h2->next != NULL) {
		h2->next->prev = h1;
	}


	h1->size = h1->size + h2->size + sizeof(chunk_header_t);

	memset(h2, 0, sizeof(chunk_header_t));

	return h1;
}



#include <tty.h>
/* Now comes the legendary malloc and free! */
void *kmalloc(uint64_t bytes) {
	if (bytes == 0) {
		return NULL;
	}

	if ((bytes % 4) != 0) {
		/* Things are better when aligned. */
		bytes += 4 - (bytes % 4);
	}

	heap_t *hp = kgetHeap();
	chunk_header_t *chunk = hp->first_free;


	while (chunk != NULL) {
		if (chunk->size < bytes) {
			/*
			 * If the chunk's size is insufficient, then don't bother with it.
			 *
			 * TODO: Make it so that kmalloc() also merges chunks (like kfree()).
			 */
			chunk = chunk->next;
			continue;
		} else if (chunk->size == bytes) {
			/* If the size somehow matches prefectly, just return that chunk. */
			if (chunk == hp->first_free) {
				hp->first_free = chunk->next;
			}
			unlink(chunk);

			/* This part could probably be made to look less insane, but whatever. */
			return (void*)(((uintptr_t)chunk) + sizeof(chunk_header_t));
		} else {
			/* Chunk's size is bigger than the size requested.
			 * if the chunk's size is significantly bigger, we should just chop it up.
			 * if not, we can afford to have a couple bytes of waste.
			 * I should probably change this to a more clever algorithm,
			 * but for now this should suffice.
			 */
			if ((chunk->size - bytes) > 32) {
				/* if difference >= 32, chop the chunk to be as big as requested,
				 * then return it.
				 */

				chunk_header_t *newchunk = divideChunk(bytes, chunk);	//returns the second chunk, look at the function.
				unlink(chunk);

				/*
				 * We must also update first_free if this is the first
				 * chunk we encountered.
				 */
				if (chunk == hp->first_free) {
					hp->first_free = newchunk;
				}

				return (void*)(((uintptr_t)chunk) + sizeof(chunk_header_t));

			}
			/* If the difference isn't that big, just return the chunk without chopping.*/

			if (chunk == hp->first_free) {
				hp->first_free = chunk->next;
			}
			unlink(chunk);

			return (void*)(((uintptr_t)chunk) + sizeof(chunk_header_t));


		}
	}
	/* No chunks left. */
	return NULL;
}


uint8_t kfree(void *ptr) {
	if (ptr == NULL) {
		return 1;
	}
	/* I'm going to use two variables to loop over free chunks,
	 * in order to find where this chunk we're freeing should be placed.
	 * We're also going to be merging adjacent free chunks if we find any along the way.
	 * this is to make sure the heap doesn't get too fragmented.
	 */

	chunk_header_t *chunk = (chunk_header_t*)((uintptr_t)ptr - sizeof(chunk_header_t));

	heap_t* hp = kgetHeap();


	chunk_header_t *i = hp->first_free;
	chunk_header_t *j = i->next;

	if (chunk < i) {
		/*
		 * Because i is the first free chunk, we must add this chunk
		 * to the beginning of the linked list.
		 */
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
			kputs("\nFUCK BITH15\n");
			return 1;	//might use a more descriptive error.
		}
		if (j == NULL) {
			/* We are at the end of the list, and i holds the last element.
			 * this means ptr belongs to the last chunk so we gotta
			 * add it to the end of the linked list.
			 */
			i->next = chunk;
			chunk->prev = i;
			chunk->next = NULL;
			return GENERIC_SUCCESS;
		}

		/* The free function also merges some chunks if it finds any adjacent
		 * chunks while searching for matches.
		 */
		if ((((uintptr_t)i) + sizeof(chunk_header_t) + i->size) == ((uintptr_t)j)) {
			/* Update stuff, merge the chunks then re-do the iteration.*/
			chunk_header_t *temp = j->next;
			mergeChunk(i, j);
			j = temp;
			continue;
		}




		if ((chunk > i) && (chunk < j)) {
			/* If chunk is between i and j, then this is where it should be in the linked list. */
			i->next = chunk;
			chunk->prev = i;

			j->prev = chunk;
			chunk->next = j;

			return GENERIC_SUCCESS;
		}




		/* Repeat until a match is found. */
		i = j;
		j = j->next;
	}

	return 1;
}

//allocates more pages for heap usage. Takes argument in bytes,
//rounded up to multiple of 4096 (0x1000) because of paging.

//uint8_t kenlargeHeap(uint64_t amount) {

	////rounds up.
	//uint64_t amount_pages = addr_to_page(amount);
	//if (amount % 0x1000) {
		//amount_pages += 1;
	//}
	//heap_t *hp = kgetHeap();
	////if no heap is present yet, then make one by allocating literally a single page.
	////note: type casts are generally there to shut the compiler up.
	//if ((void*)hp->start == NULL) {
		//return 1;
		///* YOU SHOULD ALWAYS SET UP THE
		   //FIRST PAGE OF THE HEAP YOURSELF. */
	//}


	////now continue as usual, and try to allocate as many pages as needed
	////starting from the end of the current heap (in the virtual address space.)

	//for (size_t i = 0; i < amount_pages; i++) {
		//if ((void*)hp->end == NULL) {
			//return 1;	//then we got a huge fucking problem. I should probably handle this, but fuck that.
		//}
		//if (!kmmapv(hp->end)) {
			////if successful, then we gotta increment HEAP_END
			//hp->end += 0x1000;
		//} else {
			////if not successful, then we can't do much anymore.
			////simply return an error.
			//return 1;
		//}
	//}

	//return GENERIC_SUCCESS;
//}

uint8_t init_heap(void) {
	/* Maps 256 pages to the heap. 0xFFFFFFFFA0000000 is the kernel's heap's address.*/
	//while (1) { asm("hlt;"); };
	if (map_memory(page_to_addr(allocpps(256)), 0xFFFFFFFFA0000000, 256, kgetPML4T())) {
		return 1;
	}
	krefresh_vmm();

	kheap_default.start = 0xFFFFFFFFA0000000;

	kheap_default.end = kheap_default.start + 256 * 0x1000;



	/* Now we make the first chunk, "the wilderness".
	 * Whenever we need a chunk, we will simply chop a part of this chunk,
	 * and hand it to whomever needs it. (or just hand out another free chunk
	 * if there is a suitable one).
	 */

	kheap_default.first_free = (chunk_header_t*)kheap_default.start;

	/* Beware! The size attribute of a chunk only contains info about its data section! */
	kheap_default.first_free->size = (kheap_default.end - kheap_default.start) - sizeof(chunk_header_t);

	kheap_default.first_free->prev = NULL;
	kheap_default.first_free->next = NULL;

	return GENERIC_SUCCESS;
}






#ifdef DEBUG

#include <tty.h>
#include <string.h>

void heap_print_state(void) {
	heap_t* hp = kgetHeap();

	chunk_header_t* i = hp->first_free;

	kputs("\nHEAP STATE:\n");
	kputs("{address}   {size}\n");

	while (i) {

		kputx((uint64_t)i);
		kputs("   ");

		kputx(i->size);
		kputs("\n");

		i = i->next;
	}

};

#endif





