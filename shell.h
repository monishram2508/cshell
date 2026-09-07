#ifndef SHELL_H
#define SHELL_H

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_ARGS 64
#define MAX_STAGES 16
#define MAX_TOKENS 128
#define MAX_TOKEN_LEN 256

typedef struct {
    char *argv[MAX_ARGS];
    int argc;
    char *infile;
    char *outfile;
} Command;

typedef struct {
    Command stages[MAX_STAGES];
    int n_stages;
    int background;
} CommandLine;

#endif
