#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "display.h"

#define HOST_LEN 256

static char shell_home[PATH_MAX];

void display_init(void)
{
    if (getcwd(shell_home, sizeof shell_home) == NULL) {
        perror("shell: getcwd");
        strcpy(shell_home, "/");
    }
}

const char *display_home(void)
{
    return shell_home;
}

static const char *current_user(void)
{
    struct passwd *pw = getpwuid(getuid());

    if (pw != NULL && pw->pw_name != NULL)
        return pw->pw_name;

    const char *env = getenv("USER");

    return env != NULL ? env : "user";
}

static void current_dir(char *out, size_t size)
{
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof cwd) == NULL) {
        snprintf(out, size, "?");
        return;
    }

    size_t home_len = strlen(shell_home);

    if (strcmp(cwd, shell_home) == 0)
        snprintf(out, size, "~");
    else if (strncmp(cwd, shell_home, home_len) == 0 && cwd[home_len] == '/')
        snprintf(out, size, "~%s", cwd + home_len);
    else
        snprintf(out, size, "%s", cwd);
}

void print_prompt(void)
{
    char host[HOST_LEN];
    char dir[PATH_MAX + 2];

    if (gethostname(host, sizeof host) != 0)
        strcpy(host, "localhost");
    host[sizeof host - 1] = '\0';

    current_dir(dir, sizeof dir);

    printf("<%s@%s:%s> ", current_user(), host, dir);
    fflush(stdout);
}
