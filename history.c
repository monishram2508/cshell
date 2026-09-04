#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "history.h"

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

    while (end > 0 && isspace((unsigned char)line[end - 1]))
        end--;

    while (start < end && isspace((unsigned char)line[start]))
        start++;

    if (start == end)
        return NULL;

    size_t len = end - start;
    char *copy = malloc(len + 1);

    if (copy == NULL)
        return NULL;

    memcpy(copy, line + start, len);
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
