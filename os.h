#ifndef OS_H
#define OS_H

#include "processor.h"

void os_init(void);
int os_submit(const char *program_file, const char *data_file);
void os_run(void);
void os_shell(void);
void os_shutdown(void);

#endif
