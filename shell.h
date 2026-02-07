#pragma once

#include "lexer.h"

/* Runs the main shell loop */
void shell_loop(void);

/* Prints USER@MACHINE:PWD> */
void print_prompt(void);

/* Expands $VARS and ~ in token list */
void expand_tokens(tokenlist *tokens);
