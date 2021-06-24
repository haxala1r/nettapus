#include "std.h"


void main(uint64_t argc) {

	char buf[256] = {};
	if (getarg(0, buf, 256) < 0) {
		buf[0] = '.';
		buf[1] = '\0';
	}
	puts("Entries for '");
	puts(buf);
	puts("'\n");

	int64_t fd = open(buf, O_READ);

	struct dirent *ent = (void*)buf;
	while (read(fd, (void*)ent, 256) > 0) {
		puts(ent->name);
		puts("\n");
	}
	close(fd);

	exit(0);
}
