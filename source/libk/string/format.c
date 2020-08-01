 
#include <string.h>
#include <stdint.h>

//hex byte to ascii
char xbtoa(uint8_t val) {
	if (val < 0xA) {
		char c = ((char)val) + 0x30;
		return c;
	} else if (val < 0x10){
		char c = ((char)val) + 0x37;
		return c;
	}
	return 0;
}
//hex to ascii
void xtoa(uint32_t val, char *buf) {
	buf[0] = '0';
	buf[1] = 'x';
	for (uint8_t i = 0; i < 8; i++) {
		uint8_t tempval = (val >> (i * 4)) & (0xF);
		buf[9-i] = xbtoa((uint8_t)tempval);
	}
	
}

