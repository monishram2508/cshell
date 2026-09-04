#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "builtins.h"
#include "cd.h"
#include "history.h"

static void builtin_echo(const Command *cmd)
{
    for (int i = 1; i < cmd->argc; i++)
        printf("%s%s", cmd->argv[i], i + 1 < cmd->argc ? " " : "");

    printf("\n");
}

static int exit_flag;

static void builtin_pwd(void)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof cwd) == NULL) {
        perror("pwd");
        return;
    }

    printf("%s\n", cwd);
}

int run_builtin(const Command *cmd)
{
    if (strcmp(cmd->argv[0], "echo") == 0) {
        builtin_echo(cmd);
        return 1;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        builtin_pwd();
        return 1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        builtin_cd(cmd);
        return 1;
    }

    if (strcmp(cmd->argv[0], "history") == 0) {
        history_print();
        return 1;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit_flag = 1;
        return 1;
    }

    return 0;
}

int exit_requested(void)
{
    return exit_flag;
}
