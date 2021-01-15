
#include <string.h>

void *memmove(void* dstptr, void *srcptr, size_t size) {
	uint8_t *dst = (uint8_t*) dstptr;
	uint8_t *src = (uint8_t*) srcptr;
	if (dst < src) {
		//if they overlap, it will be necessary to start from the bottom.
		for (size_t i = 0; i < size; i++) {
			*(dst + i) = *(src + i);
		}
	} else {
		//if they overlap, it will be necessary to start from the start
		for (size_t i = size; i > 0; i--) {
			*(dst + i - 1) = *(src + i - 1);
		}
	}
	
	return dstptr;
}


