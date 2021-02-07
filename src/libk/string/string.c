
#include <stddef.h> 
#include <stdint.h>


size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}


int32_t strcmp(const char *s1, const char *s2) {
	const uint8_t *i1 = (const uint8_t*) s1;
	const uint8_t *i2 = (const uint8_t*) s2;
	
	
	while (1) {
		if ((*i1 == '\0') && (*i2 == '\0')) {
			break;
		}
		
		if (*i1 > *i2) {
			return 1;
		}
		if (*i1 < *i2) {
			return -1;
		}
		i1++;
		i2++;
	}
	return 0;
};



