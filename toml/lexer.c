#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "lexer.h"

#define DEFAULT_TOKEN_LIST_SIZE 8

static bool
_match(const char *pattern, char c)
{
    // TODO: error handling here
    switch (pattern[0]) {
        case '=':
            return c == pattern[1];

		case '-':
            if (pattern[2] >= pattern[1]) {
                return c >= pattern[1] && c <= pattern[2];
            } else {
                return c <= pattern[1] && c >= pattern[2];
            }

		case '.':
            return true;

		case '$':
            return (unsigned char)c > 0x7f; // not ascii

		case '|':
			for (pattern++; *pattern; pattern++) {
				if (*pattern == c)
					return true;
			}

			return false;

		default:
            assert(0);
    }
}

token_list_t *
token_list_new()
{
    token_list_t *ret = malloc(sizeof(*ret));
    assert(ret);

    ret->tokens = malloc(sizeof(*ret->tokens) * DEFAULT_TOKEN_LIST_SIZE);
    assert(ret->tokens);

    ret->size = 0;
    ret->cap = DEFAULT_TOKEN_LIST_SIZE;

    return ret;
}

void
token_list_push(token_list_t *list, const token_t *token)
{
    if (list->size == list->cap) {
        list->cap <<= 1;
        list->tokens = realloc(list->tokens, sizeof(*list->tokens) * list->cap);
        assert(list->tokens);
    }

    list->tokens[list->size++] = *token;
}

void
token_list_free(token_list_t *list)
{
    if (list) {
        free(list->tokens);
        free(list);
    }
}

token_list_t *
lexer(const char *src, const state_trans_t table[][LEXER_MAX_RULE], lexer_err_t *err)
{
    int state = 0;
    const state_trans_t *entry;

    size_t i = 0, j = 0, len = strlen(src);
    char cur;

    token_t tmp_tok;
    token_list_t *ret;

    ret = token_list_new();

    while (i != len + 1 /* last nil char included */) {
        cur = src[i];

        for (entry = table[state]; entry->pattern; entry++) {
            // fprintf(stderr, "matching %s with %c\n", entry->pattern, cur);

            if (_match(entry->pattern, cur)) {
                state = entry->next;
                
                if (entry->err) {
                    // should be an error
                    err->msg = entry->err;
                    err->pos = i;
                    
                    token_list_free(ret);
                    return NULL;
                }

                if (entry->token != -1) {
                    tmp_tok.token = entry->token;
                    tmp_tok.str = src + j;

                    if (!entry->unwind) {
                        tmp_tok.len = i - j + 1;
                        j = i + 1;
                    } else {
                        tmp_tok.len = i - j;
                        j = i;
                    }

                    token_list_push(ret, &tmp_tok);
                }

                break;
            }
        }

        // no match found
        if (!entry->pattern) {
            if (err) {
                if (entry->err) {
                    err->msg = entry->err;
                } else {
                    err->msg = "lexer failed";
                }

                err->pos = i;
            }

            // error, return
            token_list_free(ret);
            return NULL;
        } else {
            if (!entry->unwind) {
                i++;
            }
        }
    }

    return ret;
}
