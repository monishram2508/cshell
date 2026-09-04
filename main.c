#include <stdio.h>
#include <stdlib.h>

#include "builtins.h"
#include "display.h"
#include "execute.h"
#include "history.h"
#include "parser.h"
#include "shell.h"

int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    CommandLine cl;

    display_init();
    history_init();

    while (1) {
        reap_background();
        print_prompt();

        if (getline(&line, &cap, stdin) == -1) {
            printf("\n");
            break;
        }

        history_add(line);

        if (parse_line(line, &cl) < 0)
            continue;

        if (cl.n_stages == 0)
            continue;

        if (cl.n_stages == 1 && run_builtin(&cl.stages[0]))
            continue;

        execute_command_line(&cl);
    }

    free(line);

    return 0;
}
