#include "std.h"

void memset(uint8_t *buf, int64_t len, uint8_t val) {
	for (int64_t i = 0; i < len; i++) {
		buf[i] = val;
	}
}


int64_t main(int64_t argc) {
	if (argc == 0) {
		puts("Please specify a file.\n");
		exit(0);
	}
	char buf[256] = {};
	getarg(0, buf, 255);
	int64_t fd = open(buf, 0);
	if (fd < 0) {
		puts("File '");
		puts(buf);
		puts("' not found.");
		exit(0);
	}
	while (read(fd, buf, 255) > 0) {
		write(STDOUT, buf, 255);
		memset(buf, 256, 0);
	}
	exit(0);
}
