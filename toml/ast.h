#ifndef _TOML_AST_H_
#define _TOML_AST_H_

#include <stdbool.h>

#include "parser.h"

typedef struct {
    AST_NODE_HEADER
    bool val;
} ast_node_bool_t;

ast_node_t *ast_node_bool_new(bool val);

typedef struct {
    AST_NODE_HEADER
    long long val;
} ast_node_int_t;

ast_node_t *ast_node_int_new(long long val);

typedef struct {
    AST_NODE_HEADER

    enum {
        VALUE_TYPE_INT,
        VALUE_TYPE_FLOAT,
        VALUE_TYPE_STRING,
        VALUE_TYPE_BOOL
    } type;

    ast_node_t *child;
} ast_node_value_t;

ast_node_t *ast_node_value_new(int type, ast_node_t *child);

typedef struct {
    AST_NODE_HEADER
    char *str;
} ast_node_string_t;

ast_node_t *ast_node_string_new_n(const char *str, size_t len);

typedef struct ast_node_key_t_tag {
    AST_NODE_HEADER
    ast_node_string_t *id;
    struct ast_node_key_t_tag *next;
} ast_node_key_t;

ast_node_t *ast_node_key_new(ast_node_string_t *id, ast_node_key_t *next);

typedef struct ast_node_entry_t_tag {
    AST_NODE_HEADER
    ast_node_key_t *key;
    ast_node_value_t *value; // if value is null, the entry is a table decaration
    struct ast_node_entry_t_tag *next;
} ast_node_entry_t;

ast_node_t *ast_node_entry_new(ast_node_key_t *key, ast_node_value_t *value, ast_node_entry_t *next);

typedef struct {
    const char *msg;
} gen_err_t;

#endif
