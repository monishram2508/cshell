#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

int run_builtin(const Command *cmd);
int exit_requested(void);

#endif
