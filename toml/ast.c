#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ast.h"

static void
_ast_node_bool_free(ast_node_t *node)
{
    free(node);
}

static void
_ast_node_bool_dump(ast_node_t *_node)
{
    ast_node_bool_t *node = (ast_node_bool_t *)_node;

    if (node->val)
        fprintf(stderr, "true");
    else
        fprintf(stderr, "false");
}

ast_node_t *ast_node_bool_new(bool val)
{
    ast_node_bool_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_bool_free;
    ret->dump_func = _ast_node_bool_dump;
    ret->val = val;

    return (ast_node_t *)ret;
}

static void
_ast_node_int_free(ast_node_t *node)
{
    free(node);
}

static void
_ast_node_int_dump(ast_node_t *_node)
{
    ast_node_int_t *node = (ast_node_int_t *)_node;

    if (node) {
        fprintf(stderr, "%lld", node->val);
    }
}

ast_node_t *ast_node_int_new(long long val)
{
    ast_node_int_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_int_free;
    ret->dump_func = _ast_node_int_dump;
    ret->val = val;

    return (ast_node_t *)ret;
}

static void
_ast_node_value_free(ast_node_t *_node)
{
    ast_node_value_t *node = (ast_node_value_t *)_node;

    if (node) {
        ast_node_free(node->child);
        free(node);
    }
}

static void
_ast_node_value_dump(ast_node_t *_node)
{
    ast_node_value_t *node = (ast_node_value_t *)_node;

    if (node) {
        ast_node_dump(node->child);
    }
}

ast_node_t *ast_node_value_new(int type, ast_node_t *child)
{
    ast_node_value_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_value_free;
    ret->dump_func = _ast_node_value_dump;
    ret->type = type;
    ret->child = child;

    return (ast_node_t *)ret;
}

static void
_ast_node_string_free(ast_node_t *_node)
{
    ast_node_string_t *node = (ast_node_string_t *)_node;

    if (node) {
        free(node->str);
        free(node);
    }
}

static void
_ast_node_string_dump(ast_node_t *_node)
{
    ast_node_string_t *node = (ast_node_string_t *)_node;

    if (node) {
        fprintf(stderr, "\"%s\"", node->str);
    }
}

ast_node_t *ast_node_string_new_n(const char *str, size_t len)
{
    ast_node_string_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_string_free;
    ret->dump_func = _ast_node_string_dump;
    ret->str = strndup(str, len);

    return (ast_node_t *)ret;
}

static void
_ast_node_key_free(ast_node_t *_node)
{
    ast_node_key_t *node = (ast_node_key_t *)_node;

    if (node) {
        ast_node_free(node->id);
        ast_node_free(node->next);
        free(node);
    }
}

static void
_ast_node_key_dump(ast_node_t *_node)
{
    ast_node_key_t *node = (ast_node_key_t *)_node;

    if (node) {
        ast_node_dump(node->id);

        if (node->next) {
            fprintf(stderr, ".");
            ast_node_dump(node->next);
        }
    }
}

ast_node_t *ast_node_key_new(ast_node_string_t *id, ast_node_key_t *next)
{
    ast_node_key_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_key_free;
    ret->dump_func = _ast_node_key_dump;
    ret->id = id;
    ret->next = next;

    return (ast_node_t *)ret;
}

static void
_ast_node_entry_free(ast_node_t *_node)
{
    ast_node_entry_t *node = (ast_node_entry_t *)_node;

    if (node) {
        ast_node_free(node->key);
        ast_node_free(node->value);
        ast_node_free(node->next);
        free(node);
    }
}

static void
_ast_node_entry_dump(ast_node_t *_node)
{
    ast_node_entry_t *node = (ast_node_entry_t *)_node;

    if (node) {
        if (node->value) {
            ast_node_dump(node->key);
            fprintf(stderr, " = ");
            ast_node_dump(node->value);
        } else {
            fprintf(stderr, "[");
            ast_node_dump(node->key);
            fprintf(stderr, "]");
        }

        if (node->next) {
            fprintf(stderr, "\n");
            ast_node_dump(node->next);
        }
    }
}

ast_node_t *ast_node_entry_new(ast_node_key_t *key, ast_node_value_t *value, ast_node_entry_t *next)
{
    ast_node_entry_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_entry_free;
    ret->dump_func = _ast_node_entry_dump;
    ret->key = key;
    ret->value = value;
    ret->next = next;

    return (ast_node_t *)ret;
}
