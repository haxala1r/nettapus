#include <task.h>
#include <string.h>
#include <err.h>

char *task_get_arg(struct task *t, uint64_t arg) {
	if (t == NULL) {
		return NULL;
	}

	struct task_arg *i = t->first_arg;
	for (size_t j = 0; j < arg; j++) {
		if (i == NULL) {
			break;
		}
		i = i->next;
	}
	if (i == NULL) {
		return NULL; /* Out of bounds. */
	}

	return i->str;
}
