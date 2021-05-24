#ifndef CONFIG_H
#define CONFIG_H 1

char *config_get_variable(char *name);
uint8_t reload_config(char *f);

#ifdef DEBUG
void config_put_variables();
#endif  /* DEBUG */

#endif  /* CONFIG_H */
