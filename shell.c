#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* =========================
   PART 1: PROMPT
   ========================= */
void print_prompt(void)
{
    char hostname[HOST_NAME_MAX + 1];
    char cwd[PATH_MAX + 1];

    char *user = getenv("USER");

    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    printf("%s@%s:%s> ",
           user ? user : "unknown",
           hostname,
           cwd);
    fflush(stdout);
}

/* =========================
   PART 2 + 3: EXPANSIONS
   ========================= */
void expand_tokens(tokenlist *tokens)
{
    char *home = getenv("HOME");

    for (size_t i = 0; i < tokens->size; i++) {
        char *tok = tokens->items[i];

        /* ---- Environment Variable Expansion ($VAR) ---- */
        if (tok[0] == '$' && strlen(tok) > 1) {
            char *val = getenv(tok + 1);
            if (val) {
                free(tokens->items[i]);
                tokens->items[i] = strdup(val);
            }
            continue;
        }

        /* ---- Tilde Expansion ---- */
        if (strcmp(tok, "~") == 0 && home) {
            free(tokens->items[i]);
            tokens->items[i] = strdup(home);
            continue;
        }

        if (strncmp(tok, "~/", 2) == 0 && home) {
            char expanded[PATH_MAX];
            snprintf(expanded, sizeof(expanded), "%s/%s",
                     home, tok + 2);
            free(tokens->items[i]);
            tokens->items[i] = strdup(expanded);
        }
    }
}

/* =========================
   MAIN SHELL LOOP
   ========================= */
void shell_loop(void)
{
    while (1) {
        print_prompt();

        char *input = get_input();
        if (!input)
            continue;

        tokenlist *tokens = get_tokens(input);

        /* Apply Parts 2 & 3 */
        expand_tokens(tokens);

        /* Debug output (safe to remove later) */
        for (size_t i = 0; i < tokens->size; i++) {
            printf("token %zu: (%s)\n", i, tokens->items[i]);
        }

        free(input);
        free_tokens(tokens);
    }
}
