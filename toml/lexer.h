#ifndef _TOML_LEXER_H_
#define _TOML_LEXER_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define LEXER_MAX_RULE 20

#ifndef LEXER_MAX_RULE
    #error "LEXER_MAX_RULE not defined"
#endif

typedef struct {
    const char *pattern;
    int next;
    int token; // -1 for no token

    bool unwind;
    const char *err;
} state_trans_t;

typedef struct {
    int token;
    const char *str; // note this is not nil-ended
    size_t len;
} token_t;

typedef struct {
    token_t *tokens;
    size_t size;
    size_t cap;
} token_list_t;

typedef struct {
    const char *msg;
    size_t pos;
} lexer_err_t;

token_list_t *
lexer(const char *src, const state_trans_t table[][LEXER_MAX_RULE], lexer_err_t *err);

token_list_t *
token_list_new();

void
token_list_push(token_list_t *list, const token_t *token);

void
token_list_free(token_list_t *list);

#endif
