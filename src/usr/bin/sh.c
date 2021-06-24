#include "std.h"


int64_t memcpy(char *dest, char *src, size_t amount) {
	for (size_t i = 0; i < amount; i++) {
		dest[i] = src[i];
	}
	return 0;
}


int64_t split_str(char *in, char *out, char sep) {
	char *i = in;
	while (1) {
		if (*i == '\0') {
			return -1;
		}
		if (*i == sep) {
			*i = '\0';
			i++;
			break;
		}
		i++;
	}

	memcpy(out, i, strlen(i) + 1);
	return 0;
}

char *argv[2] = {
	NULL,
	NULL
};

int64_t main(uint64_t argc) {
	while (1) {
		puts("~ $ ");

		char buf[256] = {};
		gets(buf, 256 - 7); /* Also account for the beginning "/bin/sh" path for binaries. */

		/* This is the argument we'll pass to the program. */
		char arg[128] = {};
		if (split_str(buf, arg, ' ')) {
			argv[0] = NULL;
		} else {
			argv[0] = arg;
		}

		if (!strcmp(buf, "cd")) {
			/* Simply change directories. */
			chdir(arg);
			continue;
		}
		if (!strcmp(buf, "exit")) {
			break;
		}
		if ((buf[0] != '/') && (buf[0] != '.')) {
			/* This basically simulates what $PATH does on bash. Search /bin/
			 * when a path wasn't given.
			 */
			char tmp[256];
			memcpy(tmp, buf, strlen(buf) + 1);
			memcpy(buf + 5, tmp, strlen(tmp) + 1);
			memcpy(buf, "/bin/", 5);
		}

		int64_t pid = fork();
		if (pid > 0) {
			wait(pid);
		} else if (pid == 0) {
			/* Child process*/
			exec(buf, argv);

			/* There must have been an error if exec returns. */
			puts("Failed to find file '");
			puts(buf);
			puts("'");
			exit(0);
		} else {
			puts("Error on fork()\n");
			break;
		}
	}
	exit(0);
}
