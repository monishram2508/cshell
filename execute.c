#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "execute.h"

void execute_command_line(const CommandLine *cl)
{
    const Command *cmd = &cl->stages[0];
    pid_t pid;

    if (cl->n_stages > 1) {
        fprintf(stderr, "shell: pipelines not implemented yet\n");
        return;
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);
        _exit(127);
    }

    if (cl->background) {
        printf("[%d]\n", pid);
        return;
    }

    if (waitpid(pid, NULL, 0) < 0)
        perror("waitpid");
}

void reap_background(void)
{
    pid_t pid;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("[%d] done\n", pid);
}
