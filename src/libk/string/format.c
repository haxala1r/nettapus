
#include <string.h>
#include <stdint.h>
#include <err.h>

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
void xtoa(uint64_t val, char *buf) {
	buf[0] = '0';
	buf[1] = 'x';
	for (uint8_t i = 0; i < 16; i++) {
		uint8_t tempval = (val >> (i * 4)) & (0xF);
		buf[17-i] = xbtoa((uint8_t)tempval);
	}

}

int64_t atol(const char *buf) {
	if (buf == NULL) {
		return GENERIC_SUCCESS;
	}
	size_t minus = 0;
	size_t i = 0;
	size_t len = strlen(buf);
	int64_t val = 0;

	if (buf[0] == '-') {
		minus = 1;
		i++;
	}

	while (i < len) {
		val *= 10;
		val += buf[i] - '0';
		i++;
	}
	if (minus) {
		val = -val;
	}
	return val;
}





uint32_t oct2bin(char *buf, uint32_t size) {
	uint32_t val = 0;


	while (size-- > 0) {
		val *= 8;
		val += *((uint8_t*)buf++) - '0';
	}
	return val;
}




int32_t bin2oct(uint32_t val, char* buf, uint32_t size) {
	//turns a number to a valid octal string. Used for TAR.
	while (size-- > 0) {
		uint32_t dig = val % 8;
		val /= 8;
		*(buf + size) = (char)(dig + '0');
	};
	return 0;
}




