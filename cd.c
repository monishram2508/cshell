#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cd.h"
#include "display.h"

static char prev_dir[PATH_MAX];
static int has_prev;

void builtin_cd(const Command *cmd)
{
    char target[PATH_MAX];
    char cwd[PATH_MAX];
    int announce = 0;

    if (cmd->argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }

    if (cmd->argc == 1) {
        snprintf(target, sizeof target, "%s", display_home());
    } else if (strcmp(cmd->argv[1], "-") == 0) {
        if (!has_prev) {
            fprintf(stderr, "cd: No previous directory.\n");
            return;
        }
        snprintf(target, sizeof target, "%s", prev_dir);
        announce = 1;
    } else if (cmd->argv[1][0] == '~') {
        snprintf(target, sizeof target, "%s%s", display_home(), cmd->argv[1] + 1);
    } else {
        snprintf(target, sizeof target, "%s", cmd->argv[1]);
    }

    if (getcwd(cwd, sizeof cwd) == NULL) {
        perror("cd");
        return;
    }

    if (chdir(target) != 0) {
        perror("cd");
        return;
    }

    snprintf(prev_dir, sizeof prev_dir, "%s", cwd);
    has_prev = 1;

    if (announce) {
        char moved[PATH_MAX];

        if (getcwd(moved, sizeof moved) != NULL)
            printf("%s\n", moved);
    }
}
