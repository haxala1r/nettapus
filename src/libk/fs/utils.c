#include <mem.h>
#include <string.h>
#include <tty.h>
#include <err.h>

/* This function is for fs_parse_path to use. */
uint8_t get_step(char *out, char **cur, char sep, size_t max) {
	if (out == NULL)   { return ERR_INVALID_PARAM; }
	if (cur == NULL)   { return ERR_INVALID_PARAM; }
	if (*cur == NULL)  { return ERR_INVALID_PARAM; }
	if (max == 0)      { return GENERIC_SUCCESS;   }

	/* Skip the first appearences of the seperator. */
	while (**cur == sep && **cur != '\0') {
		*cur += 1;
	}

	if (**cur == '\0') {
		/* The string doesn't contain any more steps. */
		return ERR_NO_RESULT;
	}

	while (max != 0 && **cur != '\0' && **cur != sep) {
		*(out++) = **(cur);
		++(*cur);
		max--;
	}

	*out = '\0';

	return GENERIC_SUCCESS;
}

size_t get_step_count(char *str, char sep) {
	if (str == NULL) { return 0; }

	size_t steps = 1;

	while (*str) {
		if ((*str == sep) && (*(str - 1) != sep) && (*(str + 1) != '\0')) {
			steps++;
		}
		str++;
	}

	return steps;
}


char **fs_parse_path(char *path) {
	/* The caller is responsible for calling fs_free_path() after this. */

	size_t steps = get_step_count(path, '/');
	char **arr = kmalloc((steps + 1) * sizeof(char*));

	for (size_t i = 0; i < (steps + 1); i++) {
		/* Allocate a string, and write the next step to it. */
		char *str = kmalloc(0x100);
		if (get_step(str, &path, '/', 0x100) == ERR_NO_RESULT) {
			/* get_step() returns ERR_NO_RESULT at the end of the string. */
			arr[i] = NULL;
			break;
		}
		arr[i] = str;
	}

	return arr;
}

void fs_free_path(char **arr) {
	if (arr == NULL) {
		return;
	}

	/* First, free all the allocated strings. */
	char **i = arr;
	while (*i != NULL) {
		kfree(*i);
		i += 1;
	}

	/* Free the array that keeps the strings. */
	kfree(arr);
	return;
}
