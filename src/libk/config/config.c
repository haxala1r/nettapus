#include <fs/fs.h>
#include <string.h>
#include <err.h>
#include <task.h>
#include <mem.h>

struct config_variable {
	char *name;
	char *value;
	struct config_variable *next;
};

struct config_variable *first_var = NULL;
struct config_variable *last_var = NULL;
SEMAPHORE *conf_mutex = NULL;  /* Avoid nasty race conditions. */

char *config_get_variable(char *name) {
	if (name == NULL)     { return NULL; }
	if (first_var == NULL) { return NULL; }
	acquire_semaphore(conf_mutex);

	struct config_variable *i = first_var;
	while (i != NULL) {
		if (!strcmp(name, i->name)) {
			release_semaphore(conf_mutex);
			return i->value;
		}
		i = i->next;
	}
	release_semaphore(conf_mutex);
	return NULL;
}


uint8_t add_var(struct config_variable *var) {
	if (var == NULL) { return ERR_INVALID_PARAM; }
	acquire_semaphore(conf_mutex);

	if (first_var == NULL) {
		var->next = NULL;
		first_var = var;
		last_var = var;
	} else {
		last_var->next = var;
		last_var = var;
		var->next = NULL;
	}

	release_semaphore(conf_mutex);
	return GENERIC_SUCCESS;
}

uint8_t free_var_list() {

	acquire_semaphore(conf_mutex);

	struct config_variable *i = first_var;
	struct config_variable *j = (i == NULL) ? NULL : i->next;
	while (i) {
		kfree(i);
		i = j;
		j = j->next;
	}

	first_var = NULL;
	last_var = NULL;
	release_semaphore(conf_mutex);
	return GENERIC_SUCCESS;
}

uint8_t read_line(int32_t fd, char *buf, size_t max) {
	if (fd < 0)      { return ERR_INVALID_PARAM; }
	if (buf == NULL) { return ERR_INVALID_PARAM; }

	size_t i = 0;
	while (1) {
		if (kread(fd, buf + i, 1) != 1) {
			return ERR_EOF;
		}
		if (buf[i] == '\n') {
			buf[i] = '\0';
		}
		if (buf[i] == '\0') {
			break;
		}
		i++;
		if (i >= max) {
			return ERR_OUT_OF_BOUNDS;
		}
	}
	return GENERIC_SUCCESS;
}

uint8_t parse_config_line(char *line) {
	if (line == NULL) { return ERR_INVALID_PARAM; }
	size_t eq = 0; /* Index of the equals sign */

	while ((line[eq] != '=')) {
		if (line[eq] == '\0') {
			/* There's no equals sign in the line, thus it is invalid. */
			return ERR_NO_RESULT;
		}
		eq++;
	}

	/* line[eq] == '=', eq == strlen(name) */
	char *name = kmalloc(eq + 1);
	memcpy(name, line, eq);
	name[eq] = '\0';

	/* (line + eq + 1) == value */
	size_t val_len = strlen(line + eq + 1) + 1;
	char *value = kmalloc(val_len);
	memcpy(value, line + eq + 1, val_len); /* this will also copy the null byte*/

	struct config_variable *new_var = kmalloc(sizeof(*new_var));
	new_var->name = name;
	new_var->value = value;
	new_var->next = NULL;
	return add_var(new_var);
}


uint8_t reload_config(char *f) {
	if (f == NULL)          { return ERR_INVALID_PARAM; }
	if (conf_mutex == NULL) { conf_mutex = create_semaphore(1); }

	free_var_list();

	int32_t fd = kopen(f, 0);
	while (fd < 0) {
		/* Loop over all available filesystems until a config file is found. */
		struct file_system *fs = fs_get_root();
		if (fs_set_root(fs->next)) {
			fs_set_root(fs_get_first());
			return ERR_INVALID_PARAM;
		}
		fd = kopen(f, 0);
	}

	/* The first line contains the maximum line length*/
	char line_lena[24];
	if (read_line(fd, line_lena, 20)) {
		kclose(fd);
		return ERR_INVALID_PARAM;
	}

	size_t line_len = atol(line_lena);
	uint8_t stat;
	char *line_buf = kmalloc(line_len);

	while (1) {
		memset(line_buf, 0, line_len);
		if ((stat = read_line(fd, line_buf, line_len))) {
			parse_config_line(line_buf);
			kfree(line_buf);
			if (stat == ERR_EOF) {
				break;
			}
			return stat;
		}
		if ((stat = parse_config_line(line_buf))) {
			if (stat == ERR_NO_RESULT) {
				continue;
			}
			kfree(line_buf);
			return stat;
		}
	}
	return GENERIC_SUCCESS;
}


#ifdef DEBUG
void config_put_variables() {
	struct config_variable *i = first_var;
	while (i != NULL) {
		serial_puts(i->name);
		serial_puts("=");
		serial_puts(i->value);
		serial_puts("\r\n");
		i = i->next;
	}
}
#endif
