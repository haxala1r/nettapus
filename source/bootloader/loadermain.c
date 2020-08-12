
#include <stdint.h>

void loader_main() {
	//some test output.
	uint16_t* buf = (uint16_t*) 0xB8000;
	buf[0] = 0x0141;
	buf[1] = 0x0242;
	buf[2] = 0x0343;
	buf[3] = 0x0444;
	buf[4] = 0x0545;
	
	
	while(1) {
		asm("cli;hlt;");
	}
	return;
} 

