#include <stdio.h>
#include <stdlib.h>

#include "display.h"
#include "parser.h"
#include "shell.h"

static void dump_command_line(const CommandLine *cl)
{
    printf("bg=%d stages=%d\n", cl->background, cl->n_stages);

    for (int i = 0; i < cl->n_stages; i++) {
        const Command *c = &cl->stages[i];

        printf("  [%d] argv=[", i);
        for (int j = 0; j < c->argc; j++)
            printf("%s%s", c->argv[j], j + 1 < c->argc ? ", " : "");
        printf("] in=%s out=%s\n",
               c->infile != NULL ? c->infile : "(null)",
               c->outfile != NULL ? c->outfile : "(null)");
    }
}

int main(void)
{
    char *line = NULL;
    size_t cap = 0;
    CommandLine cl;

    display_init();

    while (1) {
        print_prompt();

        if (getline(&line, &cap, stdin) == -1) {
            printf("\n");
            break;
        }

        if (parse_line(line, &cl) < 0)
            continue;

        if (cl.n_stages == 0)
            continue;

        dump_command_line(&cl);
    }

    free(line);

    return 0;
}
