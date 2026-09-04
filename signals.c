#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"
#include "signals.h"

static pid_t fg_pids[MAX_STAGES];
static volatile sig_atomic_t fg_count;

static void sigint_handler(int sig)
{
    (void)sig;

    write(STDOUT_FILENO, "\n", 1);

    for (int i = 0; i < fg_count; i++)
        kill(fg_pids[i], SIGINT);
}

static void sigchld_handler(int sig)
{
    (void)sig;

    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

void signals_init(void)
{
    struct sigaction sa_int;
    struct sigaction sa_chld;
    struct sigaction sa_ign;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) < 0)
        perror("sigaction");

    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa_chld, NULL) < 0)
        perror("sigaction");

    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;

    if (sigaction(SIGTSTP, &sa_ign, NULL) < 0)
        perror("sigaction");

    if (sigaction(SIGQUIT, &sa_ign, NULL) < 0)
        perror("sigaction");
}

void signals_set_foreground(const pid_t *pids, int count)
{
    if (count > MAX_STAGES)
        count = MAX_STAGES;

    for (int i = 0; i < count; i++)
        fg_pids[i] = pids[i];

    fg_count = count;
}

void signals_clear_foreground(void)
{
    fg_count = 0;
}
