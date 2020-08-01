


#include <string.h>


void *memset(void *dstptr, uint8_t value, size_t size) {
	uint8_t *dst = (uint8_t *) dstptr;
	for (size_t i = 0; i < size; i++) {
		*(dst + i) = value;
	}
	return dstptr;
}



