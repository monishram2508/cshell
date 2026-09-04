#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"

static char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
static int n_tokens;

static int is_special(char c)
{
    return c == '|' || c == '&' || c == '<' || c == '>';
}

static int is_special_token(const char *t)
{
    return t[1] == '\0' && is_special(t[0]);
}

static int tokenize(const char *line)
{
    int i = 0;

    n_tokens = 0;

    while (line[i] != '\0') {
        if (isspace((unsigned char)line[i])) {
            i++;
            continue;
        }

        if (n_tokens >= MAX_TOKENS) {
            fprintf(stderr, "shell: too many tokens\n");
            return -1;
        }

        if (is_special(line[i])) {
            tokens[n_tokens][0] = line[i];
            tokens[n_tokens][1] = '\0';
            n_tokens++;
            i++;
            continue;
        }

        int start = i;

        while (line[i] != '\0' && !isspace((unsigned char)line[i]) &&
               !is_special(line[i]))
            i++;

        int len = i - start;

        if (len >= MAX_TOKEN_LEN) {
            fprintf(stderr, "shell: token too long\n");
            return -1;
        }

        memcpy(tokens[n_tokens], line + start, len);
        tokens[n_tokens][len] = '\0';
        n_tokens++;
    }

    return 0;
}

int parse_line(const char *line, CommandLine *cl)
{
    memset(cl, 0, sizeof *cl);

    if (tokenize(line) < 0)
        return -1;

    if (n_tokens == 0)
        return 0;

    if (strcmp(tokens[n_tokens - 1], "&") == 0) {
        cl->background = 1;
        n_tokens--;
    }

    for (int i = 0; i < n_tokens; i++) {
        if (strcmp(tokens[i], "&") == 0) {
            fprintf(stderr, "shell: syntax error near '&'\n");
            return -1;
        }
    }

    if (n_tokens == 0) {
        fprintf(stderr, "shell: syntax error near '&'\n");
        return -1;
    }

    Command *stage = &cl->stages[0];
    int argc = 0;

    cl->n_stages = 1;

    for (int i = 0; i < n_tokens; i++) {
        char *t = tokens[i];

        if (strcmp(t, "|") == 0) {
            if (argc == 0) {
                fprintf(stderr, "shell: syntax error near '|'\n");
                return -1;
            }
            if (cl->n_stages >= MAX_STAGES) {
                fprintf(stderr, "shell: too many pipeline stages\n");
                return -1;
            }

            stage->argv[argc] = NULL;
            stage->argc = argc;
            stage = &cl->stages[cl->n_stages++];
            argc = 0;
            continue;
        }

        if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0) {
            if (i + 1 >= n_tokens || is_special_token(tokens[i + 1])) {
                fprintf(stderr, "shell: syntax error: expected filename after '%s'\n", t);
                return -1;
            }

            i++;

            if (t[0] == '<')
                stage->infile = tokens[i];
            else
                stage->outfile = tokens[i];

            continue;
        }

        if (argc >= MAX_ARGS - 1) {
            fprintf(stderr, "shell: too many arguments\n");
            return -1;
        }

        stage->argv[argc++] = t;
    }

    if (argc == 0) {
        fprintf(stderr, "shell: syntax error: missing command\n");
        return -1;
    }

    stage->argv[argc] = NULL;
    stage->argc = argc;

    return 0;
}
