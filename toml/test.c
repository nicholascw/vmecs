#include <stdio.h>

#include "toml.h"
#include "obj.h"

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

    table_object_t *tab_obj = table_object_new();
    string_object_t *str_obj = string_object_new("yo");
    int_object_t *int_obj = int_object_new(10086);

    table_object_insert(tab_obj, "a string", (object_t *)str_obj);
    table_object_insert(tab_obj, "an int", (object_t *)int_obj);

    fprintf(stderr, "a string: %p %p\n", (void *)str_obj, (void *)table_object_lookup(tab_obj, "a string"));
    fprintf(stderr, "an int: %p %p\n", (void *)int_obj, (void *)table_object_lookup(tab_obj, "an int"));

    object_free(tab_obj);

    return 0;
}
