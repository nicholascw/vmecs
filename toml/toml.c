#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "toml.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"

enum {
    T_LBRACK = 0,
    T_RBRACK,
    T_DOT,
    T_EQ,
    T_NEWLINE,
    T_SPACE,

    T_ID,
    T_STRING,
    T_INT,
    T_FLOAT,

    T_FALSE,
    T_TRUE
};

enum {
    S_INIT = 0,
    S_IN_ID,
    S_IN_SPACE,
    S_IN_NEWLINE,

    S_IN_DOUBLE_STR,
    S_IN_DOUBLE_STR_ESCAPE,

    S_IN_SINGLE_STR,
    S_IN_SINGLE_STR_ESCAPE,

    S_IN_INT,
    S_IN_FLOAT
};

static const char *token_name[] = {
    "left bracket", // T_LBRACK
    "right bracket", // T_RBRACK
    "dot", // T_DOT
    "eq", // T_EQ
    "newline", // T_NEWLINE
    "space", // T_SPACE

    "id", // T_ID
    "string", // T_STRING
    "int", // T_INT
    "float", // T_FLOAT

    "false", // T_FALSE
    "true", // T_TRUE
};

static const struct {
    const char *keyword;
    int token;
} keyword_table[] = {
    { "false", T_FALSE },
    { "true", T_TRUE }
};

void print_token(token_t *token)
{
    fprintf(stderr, "token: %s, str: '%.*s'\n", token_name[token->token], (int)token->len, token->str);
}

void
_toml_keyword(token_list_t *list)
{
    size_t i, j;
    
    if (list) {
        for (i = 0; i < list->size; i++) {
            if (list->tokens[i].token == T_ID) {
                for (j = 0; j < sizeof(keyword_table) / sizeof(*keyword_table); j++) {
                    if (strncmp(keyword_table[j].keyword,
                                list->tokens[i].str,
                                list->tokens[i].len) == 0) {
                        list->tokens[i].token = keyword_table[j].token;
                        break;
                    }
                }
            }
        }
    }
}

token_list_t *
toml_lexer(const char *src, lexer_err_t *err)
{
    static const state_trans_t table[][LEXER_MAX_RULE] = {
        // S_INIT
        {
            { "-az", S_IN_ID, .token = -1 },
            { "-AZ", S_IN_ID, .token = -1 },
            { "|_", S_IN_ID, .token = -1 },

            { "|-+", S_IN_INT, .token = -1 },
            { "-09", S_IN_INT, .token = -1 },

            // { "=.", S_DOT, .token = -1 },

            { "=[", S_INIT, .token = T_LBRACK },
            { "=]", S_INIT, .token = T_RBRACK },
            { "==", S_INIT, .token = T_EQ },
            { "=.", S_INIT, .token = T_DOT },

            { "|\r\n", S_IN_NEWLINE, .token = -1, .unwind = true },

            { "| \t", S_IN_SPACE, .token = -1, .unwind = true },

            { "=\"", S_IN_DOUBLE_STR, .token = -1 },
            { "='", S_IN_SINGLE_STR, .token = -1 },

            { "=\0", S_INIT, .token = -1 },

            { NULL, .err = "unrecognized char" }
        },

        // S_IN_ID
        {
            { "-az", S_IN_ID, .token = -1 },
            { "-AZ", S_IN_ID, .token = -1 },
            { "-09", S_IN_ID, .token = -1 },
            { "|_", S_IN_ID, .token = -1 },

            { ".", S_INIT, .token = T_ID, .unwind = true }
        },

        // S_IN_SPACE
        {
            { "| \t", S_IN_SPACE, .token = -1 },

            { ".", S_INIT, .token = T_SPACE, .unwind = true }
        },

        // S_IN_NEWLINE
        {
            { "|\r\n", S_IN_NEWLINE, .token = -1 },

            { ".", S_INIT, .token = T_NEWLINE, .unwind = true }
        },

        // S_IN_DOUBLE_STR
        {
            { "=\"", S_INIT, .token = T_STRING },
            { "=\\", S_IN_DOUBLE_STR_ESCAPE, .token = -1 },
            { "=\0", .err = "unclosed double quote string literal" },
            { ".", S_IN_DOUBLE_STR, .token = -1 }
        },

        // S_IN_DOUBLE_STR_ESCAPE
        {
            { ".", S_IN_DOUBLE_STR, .token = -1 }
        },

        // S_IN_SINGLE_STR
        {
            { "='", S_INIT, .token = T_STRING },
            { "=\\", S_IN_SINGLE_STR_ESCAPE, .token = -1 },
            { "=\0", .err = "unclosed single quote string literal" },
            { ".", S_IN_SINGLE_STR, .token = -1 }
        },

        // S_IN_SINGLE_STR_ESCAPE
        {
            { ".", S_IN_SINGLE_STR, .token = -1 }
        },

        // S_IN_INT
        {
            { "-09", S_IN_INT, .token = -1 },
            { "=_", S_IN_INT, .token = -1 },
            
            { "=.", S_IN_FLOAT, .token = -1 },

            { ".", S_INIT, .token = T_INT, .unwind = true }
        },

        // S_IN_FLOAT
        {
            { "-09", S_IN_FLOAT, .token = -1 },
            { "=_", S_IN_FLOAT, .token = -1 },

            { "=.", .err = "more than one floating point" },

            { ".", S_INIT, .token = T_FLOAT, .unwind = true }
        }
    };

    token_list_t *list = lexer(src, table, err);
    size_t i;

    _toml_keyword(list);

    if (list) {
        for (i = 0; i < list->size; i++) {
            print_token(list->tokens + i);
        }
    } else if (err) {
        fprintf(stderr, "error: %lu: %s\n", err->pos, err->msg);
    }

    return list;
}

/*

syntax:

id
    : ID
    | STRING

space_opt
    : SPACE
    | empty

key_tail
    : space_opt DOT space_opt id key_tail
    | empty

key
    : id key_tail

bool
    : false
    | true

value
    : int
    | float
    | string
    | bool

entry
    : key space_opt EQ space_opt value

table
    : LBRACK space_opt key space_opt RBRACK

*/

// sv for syntax var

#define SPACE_OPT NONTERM(sv_space_opt)

RULE(sv_space_opt, {
    IF_MATCH(SYMBOLS(TERM(T_SPACE)), {
        RETURN_NODE(NULL);
    })

    RETURN_NODE(NULL);
})

RULE(sv_bool, {
    IF_MATCH(SYMBOLS(TERM(T_FALSE)), {
        RETURN_NODE(ast_node_bool_new(false));
    })

    IF_MATCH(SYMBOLS(TERM(T_TRUE)), {
        RETURN_NODE(ast_node_bool_new(true));
    })
})

RULE(sv_string, {
    token_t *tok;

    IF_MATCH(SYMBOLS(TERM(T_STRING)), {
        tok = TOKEN(0);
        RETURN_NODE(ast_node_string_new_n(tok->str + 1, tok->len - 2));
    })
})

RULE(sv_int, {
    token_t *tok;
    char *tmp;
    size_t i;
    size_t j;
    long long val;

    IF_MATCH(SYMBOLS(TERM(T_INT)), {
        tok = TOKEN(0);

        tmp = malloc(tok->len + 1);
        assert(tmp);
        
        for (i = 0, j = 0; i < tok->len; i++) {
            if (tok->str[i] != '_') {
                tmp[j++] = tok->str[i];
            }
        }

        tmp[j] = 0;

        val = atoll(tmp);
        free(tmp);
        
        RETURN_NODE(ast_node_int_new(val));
    })
})

RULE(sv_value, {
    IF_MATCH(SYMBOLS(NONTERM(sv_bool)), {
        RETURN_NODE(ast_node_value_new(VALUE_TYPE_BOOL, CHILD(0)));
    })

    IF_MATCH(SYMBOLS(NONTERM(sv_string)), {
        RETURN_NODE(ast_node_value_new(VALUE_TYPE_STRING, CHILD(0)));
    })
    
    IF_MATCH(SYMBOLS(NONTERM(sv_int)), {
        RETURN_NODE(ast_node_value_new(VALUE_TYPE_INT, CHILD(0)));
    })
})

RULE(sv_id, {
    token_t *tok;

    IF_MATCH(SYMBOLS(TERM(T_ID)), {
        tok = TOKEN(0);
        RETURN_NODE(ast_node_string_new_n(tok->str, tok->len));
    })
    
    IF_MATCH(SYMBOLS(TERM(T_STRING)), {
        tok = TOKEN(0);
        // trim the quote before and after the string
        RETURN_NODE(ast_node_string_new_n(tok->str + 1, tok->len - 2));
    })
})

/*
key_tail
    : space_opt DOT space_opt id key_tail
    | empty
*/
RULE(sv_key_tail, {
    IF_MATCH(SYMBOLS(
        SPACE_OPT, TERM(T_DOT), SPACE_OPT, NONTERM(sv_id), NONTERM(sv_key_tail)
        ), {
        RETURN_NODE(ast_node_key_new((ast_node_string_t *)CHILD(2), (ast_node_key_t *)CHILD(3)));
    })

    RETURN_NODE(NULL);
})

RULE(sv_key, {
    IF_MATCH(SYMBOLS(
        NONTERM(sv_id), NONTERM(sv_key_tail)
        ), {
        RETURN_NODE(ast_node_key_new((ast_node_string_t *)CHILD(0), (ast_node_key_t *)CHILD(1)));
    })
})

RULE(sv_entry, {
    IF_MATCH(SYMBOLS(
        SPACE_OPT,
        NONTERM(sv_key), SPACE_OPT,
        TERM(T_EQ), SPACE_OPT,
        NONTERM(sv_value), SPACE_OPT
        ), {
        RETURN_NODE(ast_node_entry_new((ast_node_key_t *)CHILD(1), (ast_node_value_t *)CHILD(4), NULL));
    })

    IF_MATCH(SYMBOLS(
        SPACE_OPT,
        TERM(T_LBRACK), SPACE_OPT,
        NONTERM(sv_key), SPACE_OPT,
        TERM(T_RBRACK), SPACE_OPT
        ), {
        RETURN_NODE(ast_node_entry_new((ast_node_key_t *)CHILD(2), NULL, NULL));
    })

    IF_MATCH(SYMBOLS(
        SPACE_OPT,
        TERM(T_LBRACK), SPACE_OPT,
        TERM(T_RBRACK), SPACE_OPT
        ), {
        RETURN_NODE(ast_node_entry_new(NULL, NULL, NULL));
    })
})

RULE(sv_newline_list_opt, {
    IF_MATCH(SYMBOLS(SPACE_OPT, TERM(T_NEWLINE), NONTERM(sv_newline_list_opt)), {
        RETURN_NODE(NULL);
    })

    RETURN_NODE(NULL);
})

RULE(sv_newline_list, {
    IF_MATCH(SYMBOLS(SPACE_OPT, TERM(T_NEWLINE), NONTERM(sv_newline_list_opt)), {
        RETURN_NODE(NULL);
    })
})

RULE(sv_entry_list_tail, {
    ast_node_entry_t *entry;

    IF_MATCH(SYMBOLS(NONTERM(sv_newline_list), NONTERM(sv_entry), NONTERM(sv_entry_list_tail)), {
        entry = (ast_node_entry_t *)CHILD(1);
        entry->next = (ast_node_entry_t *)CHILD(2);
        RETURN_NODE(entry);
    })

    RETURN_NODE(NULL);
})

RULE(sv_entry_list, {
    ast_node_entry_t *entry;

    IF_MATCH(SYMBOLS(NONTERM(sv_newline_list_opt), NONTERM(sv_entry), NONTERM(sv_entry_list_tail)), {
        entry = (ast_node_entry_t *)CHILD(1);
        entry->next = (ast_node_entry_t *)CHILD(2);
        RETURN_NODE(entry);
    })
})

RULE(sv_parse_unit, {
    IF_MATCH(SYMBOLS(NONTERM(sv_entry_list), NONTERM(sv_newline_list_opt)), {
        RETURN_NODE(CHILD(0));
    })
})

ast_node_entry_t *
toml_parse(token_list_t *list)
{
    result_t res = PARSE(sv_parse_unit, list);

    if (res.status == RESULT_MATCH &&
        res.n_token != list->size) {
        // not fully parsed
        fprintf(stderr, "not fully parsed, %lu/%lu token(s) parsed\n", res.n_token, list->size);

        ast_node_free(res.node);
        return NULL;
    }

    return (ast_node_entry_t *)res.node;
}
