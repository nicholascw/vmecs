#ifndef _TOML_TOML_H_
#define _TOML_TOML_H_

/*

1. comment
2. support types: string, int, float, bool, array

*/

#include <stdbool.h>

#include "lexer.h"
#include "ast.h"
#include "obj.h"

token_list_t *
toml_lexer(const char *src, lexer_err_t *err);

typedef struct {
    const char *msg;
} parse_err_t;

ast_node_entry_t *
toml_parse(token_list_t *list, parse_err_t *err);

typedef table_object_t toml_object_t;

typedef struct {
    const char *msg;
    size_t pos;
} toml_err_t;

toml_object_t *
toml_load(const char *src, toml_err_t *err);

#define toml_free object_free

object_t *
toml_lookup(toml_object_t *root, const char *query, toml_err_t *err);

#endif
