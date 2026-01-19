#ifndef DUCKY_H
#define DUCKY_H

/* Initializes the DuckyScript engine */
void ducky_init();

/* Loads an external variables profile if exists */
void ducky_load_profile();

/* Executes a DuckyScript file */
int ducky_execute_script(const char *filename);

/* Sets a script variable manually */
void ducky_set_var(const char *name, const char *val);

#endif // DUCKY_H
