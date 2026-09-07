#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "history.h"
#include "shell.h"

#define HIST_MAX 20
#define HIST_SHOW 10
#define HIST_FILE ".shell_history"

static char *entries[HIST_MAX];
static int count;
static char hist_path[PATH_MAX];

static void push(char *entry)
{
    if (count == HIST_MAX) {
        free(entries[0]);
        memmove(entries, entries + 1, (HIST_MAX - 1) * sizeof *entries);
        count--;
    }

    entries[count++] = entry;
}

static char *trimmed_copy(const char *line)
{
    size_t end = strlen(line);
    size_t start = 0;
    size_t last = 0;

    if (end > 0 && line[end - 1] == '\n')
        end--;

    while (start < end && isspace((unsigned char)line[start]))
        start++;

    for (size_t i = start; i < end; i++)
        if (!isspace((unsigned char)line[i]))
            last = i;

    if (start == end)
        return NULL;

    char *copy = malloc(end - start + 1);
    size_t len = 0;

    if (copy == NULL)
        return NULL;

    for (size_t i = start; i < end; i++) {
        if (i > last) {
            copy[len++] = line[i];
            continue;
        }

        if (isspace((unsigned char)line[i])) {
            copy[len++] = ' ';

            while (i + 1 < end && isspace((unsigned char)line[i + 1]))
                i++;

            continue;
        }

        copy[len++] = line[i];
    }

    copy[len] = '\0';

    return copy;
}

static void history_save(void)
{
    FILE *fp = fopen(hist_path, "w");

    if (fp == NULL) {
        perror("history");
        return;
    }

    for (int i = 0; i < count; i++)
        fprintf(fp, "%s\n", entries[i]);

    fclose(fp);
}

void history_init(void)
{
    snprintf(hist_path, sizeof hist_path, "%s/%s", display_home(), HIST_FILE);

    FILE *fp = fopen(hist_path, "r");

    if (fp == NULL)
        return;

    char *line = NULL;
    size_t cap = 0;

    while (getline(&line, &cap, fp) != -1) {
        char *entry = trimmed_copy(line);

        if (entry != NULL)
            push(entry);
    }

    free(line);
    fclose(fp);
}

void history_add(const char *line)
{
    char *entry = trimmed_copy(line);

    if (entry == NULL)
        return;

    if (count > 0 && strcmp(entries[count - 1], entry) == 0) {
        free(entry);
        return;
    }

    push(entry);
    history_save();
}

void history_print(void)
{
    int start = count > HIST_SHOW ? count - HIST_SHOW : 0;

    for (int i = start; i < count; i++)
        printf("%s\n", entries[i]);
}
