#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ast.h"
#include "obj.h"

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

static void *
_ast_node_bool_gen(ast_node_t *_node, void *ctx)
{
    ast_node_bool_t *node = (ast_node_bool_t *)_node;
    return bool_object_new(node->val);
}

ast_node_t *ast_node_bool_new(bool val)
{
    ast_node_bool_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_bool_free;
    ret->dump_func = _ast_node_bool_dump;
    ret->gen_func = _ast_node_bool_gen;
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

static void *
_ast_node_int_gen(ast_node_t *_node, void *ctx)
{
    ast_node_int_t *node = (ast_node_int_t *)_node;
    return int_object_new(node->val);
}

ast_node_t *ast_node_int_new(long long val)
{
    ast_node_int_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_int_free;
    ret->dump_func = _ast_node_int_dump;
    ret->gen_func = _ast_node_int_gen;
    ret->val = val;

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

static void *
_ast_node_string_gen(ast_node_t *_node, void *ctx)
{
    ast_node_string_t *node = (ast_node_string_t *)_node;
    return string_object_new(node->str);
}

ast_node_t *ast_node_string_new_n(const char *str, size_t len)
{
    ast_node_string_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_string_free;
    ret->dump_func = _ast_node_string_dump;
    ret->gen_func = _ast_node_string_gen;
    ret->str = strndup(str, len); // TODO: process escape characters

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

static void *
_ast_node_value_gen(ast_node_t *_node, void *ctx)
{
    ast_node_value_t *node = (ast_node_value_t *)_node;
    return ast_node_gen(node->child, ctx);
}

ast_node_t *ast_node_value_new(int type, ast_node_t *child)
{
    ast_node_value_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_value_free;
    ret->dump_func = _ast_node_value_dump;
    ret->gen_func = _ast_node_value_gen;
    ret->type = type;
    ret->child = child;

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

static void *
_ast_node_key_gen(ast_node_t *_node, void *ctx)
{
    fprintf(stderr, "no gen function for key\n");
    assert(0);
}

ast_node_t *ast_node_key_new(ast_node_string_t *id, ast_node_key_t *next)
{
    ast_node_key_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_key_free;
    ret->dump_func = _ast_node_key_dump;
    ret->gen_func = _ast_node_key_gen;
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

static void *
_ast_node_entry_gen(ast_node_t *_node, void *ctx)
{
    ast_node_entry_t *node = (ast_node_entry_t *)_node;
    table_object_t *root;
    table_object_t *cur_env, *sub;

    object_t *tmp;

    ast_node_key_t *key;

    const char *key_id;

    gen_err_t *err = (gen_err_t *)ctx;

    root = table_object_new();
    cur_env = root;

    for (; node; node = node->next) {
        if (!node->key) {
            // change back to root
            assert(node->value == NULL);
            cur_env = root;
        } else {
            if (!node->value) {
                // set original env to root whenever changing env
                cur_env = root;
            }

            sub = cur_env;
            
            for (key = node->key; key->next; key = key->next) {
                key_id = key->id->str;
                tmp = table_object_lookup(sub, key_id);

                if (!tmp) {
                    // create new one if no table exits in this path
                    tmp = (object_t *)table_object_new();
                    table_object_insert(sub, key_id, tmp);
                    sub = (table_object_t *)tmp;
                } else if (IS_TYPE(tmp, TABLE)) {
                    sub = (table_object_t *)tmp;
                } else {
                    // TODO: cannot reset value
                    sub = NULL;
                    break;
                }
            }

            key_id = key->id->str;

            if (!sub || table_object_lookup(sub, key_id)) {
                object_free(root);
                if (err) err->msg = "overriding value";
                return NULL;
            }

            if (node->value) {
                tmp = ast_node_gen(node->value, ctx);
                assert(tmp);
                table_object_insert(sub, key_id, tmp);
            } else {
                cur_env = table_object_new();
                table_object_insert(sub, key_id, (object_t *)cur_env);
            }
        }
    }

    return root;
}

ast_node_t *ast_node_entry_new(ast_node_key_t *key, ast_node_value_t *value, ast_node_entry_t *next)
{
    ast_node_entry_t *ret = calloc(1, sizeof(*ret));
    assert(ret);

    ret->free_func = _ast_node_entry_free;
    ret->dump_func = _ast_node_entry_dump;
    ret->gen_func = _ast_node_entry_gen;
    ret->key = key;
    ret->value = value;
    ret->next = next;

    return (ast_node_t *)ret;
}
