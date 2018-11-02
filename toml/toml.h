#ifndef _TOML_TOML_H_
#define _TOML_TOML_H_

/*

1. comment
2. support types: string, int, float, bool, array

*/

#include "lexer.h"
#include "ast.h"

token_list_t *
toml_lexer(const char *src, lexer_err_t *err);

ast_node_entry_t *
toml_parse(token_list_t *list);

#endif
