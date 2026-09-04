#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "builtins.h"
#include "display.h"
#include "execute.h"
#include "history.h"
#include "signals.h"
#include "parser.h"
#include "shell.h"

int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    CommandLine cl;

    display_init();
    history_init();
    signals_init();

    while (1) {
        print_prompt();

        errno = 0;

        if (getline(&line, &cap, stdin) == -1) {
            if (errno == EINTR && !feof(stdin)) {
                clearerr(stdin);
                continue;
            }

            printf("\n");
            break;
        }

        history_add(line);

        if (parse_line(line, &cl) < 0)
            continue;

        if (cl.n_stages == 0)
            continue;

        const Command *first = &cl.stages[0];

        if (cl.n_stages == 1 && !cl.background && first->infile == NULL &&
            first->outfile == NULL && run_builtin(first)) {
            if (exit_requested())
                break;

            continue;
        }

        execute_command_line(&cl);
    }

    free(line);

    return 0;
}
