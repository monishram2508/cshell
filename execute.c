#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins.h"
#include "execute.h"

static int apply_redirection(const Command *cmd)
{
    if (cmd->infile != NULL) {
        int fd = open(cmd->infile, O_RDONLY);

        if (fd < 0) {
            perror(cmd->infile);
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    if (cmd->outfile != NULL) {
        int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            perror(cmd->outfile);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

static void close_pipes(int pipes[][2], int n_pipes)
{
    for (int i = 0; i < n_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

static void run_stage(const CommandLine *cl, int index, int pipes[][2], int n_pipes)
{
    const Command *cmd = &cl->stages[index];

    if (index > 0 && dup2(pipes[index - 1][0], STDIN_FILENO) < 0) {
        perror("dup2");
        _exit(1);
    }

    if (index < cl->n_stages - 1 && dup2(pipes[index][1], STDOUT_FILENO) < 0) {
        perror("dup2");
        _exit(1);
    }

    close_pipes(pipes, n_pipes);

    if (apply_redirection(cmd) < 0)
        _exit(1);

    if (run_builtin(cmd)) {
        fflush(stdout);
        _exit(0);
    }

    execvp(cmd->argv[0], cmd->argv);
    perror(cmd->argv[0]);
    _exit(127);
}

void execute_command_line(const CommandLine *cl)
{
    int pipes[MAX_STAGES][2];
    pid_t pids[MAX_STAGES];
    int n_pipes = cl->n_stages - 1;
    int forked = 0;

    for (int i = 0; i < n_pipes; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            close_pipes(pipes, i);
            return;
        }
    }

    fflush(stdout);

    for (int i = 0; i < cl->n_stages; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            break;
        }

        if (pid == 0)
            run_stage(cl, i, pipes, n_pipes);

        pids[forked++] = pid;
    }

    close_pipes(pipes, n_pipes);

    if (cl->background) {
        if (forked > 0)
            printf("[%d]\n", pids[forked - 1]);
        return;
    }

    for (int i = 0; i < forked; i++)
        if (waitpid(pids[i], NULL, 0) < 0)
            perror("waitpid");
}

void reap_background(void)
{
    pid_t pid;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("[%d] done\n", pid);
}
