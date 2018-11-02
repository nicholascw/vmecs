#include <stdio.h>

#include "toml.h"

int main()
{
    lexer_err_t err;
    token_list_t *list;
    ast_node_t *node;

#define TEST(str) \
    fprintf(stderr, "############################\n"); \
    list = toml_lexer(str, &err); \
    token_list_free(list);

    TEST("abc s \tss.o = ss. \n\noo0 \"hello\\\"\" ");
    TEST(
        "[yo.config]\n"
        "   name = 'hi'\n"
        "   port = -10_2_39.0\n"
        "   score = .1\n"
        "   kill = true\n"
        "   false = true"
    );

    fprintf(stderr, "############################\n");
    list = toml_lexer(
        "[a.b.bc]\n"
        "    a.b.c.c.c.c.c.c.c.c = true\n\n\n"
        " a = 'hi' \n  "
        " b = \"sdsd\" \n"
        " c = 101_011_020 ",
        &err);
    node = toml_parse(list);
    ast_node_dump(node);
    fprintf(stderr, "\n");

    ast_node_free(node);
    token_list_free(list);

    return 0;
}
