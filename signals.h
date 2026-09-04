#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

void signals_init(void);
void signals_set_foreground(const pid_t *pids, int count);
void signals_clear_foreground(void);

#endif
