#ifndef _TOML_PARSER_H_
#define _TOML_PARSER_H_

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "lexer.h"

// a recursive descent parser

struct ast_node_t_tag;

typedef void (*ast_node_free_t)(struct ast_node_t_tag *node);
typedef void (*ast_node_dump_t)(struct ast_node_t_tag *node);
typedef void *(*ast_node_gen_t)(struct ast_node_t_tag *node, void *ctx);

#define AST_NODE_HEADER \
    ast_node_free_t free_func; \
    ast_node_dump_t dump_func; \
    ast_node_gen_t gen_func;

typedef struct ast_node_t_tag {
    AST_NODE_HEADER
} ast_node_t;

static inline void
ast_node_free(void *node)
{
    if (node) {
        ((ast_node_t *)node)->free_func(node);
    }
}

static inline void
ast_node_dump(void *node)
{
    if (node) {
        if (((ast_node_t *)node)->dump_func) {
            ((ast_node_t *)node)->dump_func(node);
        } else {
            fprintf(stderr, "(node at %p)", node);
        }
    } else {
        fprintf(stderr, "(NULL)");
    }
}

static inline void *
ast_node_gen(void *node, void *ctx)
{
    return ((ast_node_t *)node)->gen_func(node, ctx);
}

typedef struct {
    enum {
        RESULT_MATCH,
        RESULT_NO_MACTH
    } status;

    ast_node_t *node;
    size_t n_token; // number of token matched
} result_t;

typedef result_t (*parser_t)(const token_list_t *list, size_t cur);

typedef struct {
    enum {
        SYMBOL_TERM,
        SYMBOL_NONTERM
    } type;

    union {
        int token;
        parser_t parser;
    } symbol;
} symbol_t;

#define PARSER_NAME(name) _ ## name ## _parser

// put an immediate value to the stack and get the address
#define IMM2PTR(type, imm) (&((struct { type v; }) { .v = (imm) }).v)

#define TERM(t) IMM2PTR(symbol_t, ((symbol_t) { .type = SYMBOL_TERM, .symbol = { .token = (t) } }))
#define NONTERM(name) IMM2PTR(symbol_t, ((symbol_t) { .type = SYMBOL_NONTERM, .symbol = { .parser = PARSER_NAME(name) } }))

#define SYMBOLS(...) ((symbol_t *[]) { __VA_ARGS__, NULL })

// SYMBOLS(TERM(T_DOT) NONTERM(space_opt) NONTERM(key))

static inline ast_node_t **
match_rule(symbol_t **symbols, const token_list_t *list, size_t cur, size_t *n_matched_p)
{
    symbol_t **iter;
    symbol_t *cur_symbol;

    result_t tmp;

    size_t n_matched = 0;
    bool match = true;

    ast_node_t **children;
    size_t i, n_children = 0;
    size_t cur_child = 0;

    for (iter = symbols; *iter; iter++) {
        if ((*iter)->type == SYMBOL_NONTERM) {
            n_children++;
        }
    }

    children = calloc(n_children, sizeof(*children));
    assert(children);

    for (iter = symbols; *iter; iter++) {
        cur_symbol = *iter;

        if (cur_symbol->type == SYMBOL_TERM) {
            // terminal symbol
            
            if (cur + n_matched >= list->size) {
                // no enough token left
                match = false;
                break;
            }
                
            if (cur_symbol->symbol.token == list->tokens[cur + n_matched].token) {
                n_matched++;
            } else {
                match = false;
                break;
            }
        } else {
            // non-term symbol
            // recursive match
            tmp = cur_symbol->symbol.parser(list, cur + n_matched);

            // fprintf(stderr, "matched nonterm %lu\n", n_matched);

            if (tmp.status == RESULT_MATCH) {
                n_matched += tmp.n_token;
                // push node
                children[cur_child++] = tmp.node;
            } else {
                match = false;
                break;
            }
        }
    }

    // has one rule matched
    if (match) {
        if (n_matched_p)
            *n_matched_p = n_matched;

        return children;
    } else {
        for (i = 0; i < n_children; i++) {
            ast_node_free(children[i]);
        }

        free(children);
        return NULL;
    }
}

// if ((match_rule(SYMBOLS(TERM(T_DOT) NONTERM(space_opt) NONTERM(key)), list, cur, &n_matched))

#define RULE(name, block) \
    result_t PARSER_NAME(name)(const token_list_t *list, size_t cur) { \
        size_t n_matched = 0; \
        ast_node_t **match_res = NULL; \
        result_t res; \
        (void)n_matched; \
        (void)match_res; \
        res.n_token = 0; \
        block; \
        /* no match */ \
        res.status = RESULT_NO_MACTH; \
        res.node = NULL; \
        res.n_token = 0; \
        return res; \
    }

#define RETURN_NODE(n) \
    do { \
        res.status = RESULT_MATCH; \
        res.node = (ast_node_t *)(n); \
        res.n_token = n_matched; \
        free(match_res); \
        return res; \
    } while (0);

#define CHILD(n) (match_res[n])
#define TOKEN(n) (list->tokens + cur + (n))

#define IF_MATCH(symbols, block) \
    if ((match_res = match_rule((symbols), list, cur, &n_matched))) { \
        block; \
    }

#define PARSE(name, list) \
    PARSER_NAME(name)((list), 0)

#endif
