#include <stdio.h>

#include "toml.h"
#include "obj.h"

int main()
{
#if 0

    lexer_err_t err;
    token_list_t *list;
    ast_node_entry_t *node;
    table_object_t *res;

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
    node = toml_parse(list, NULL);
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

    fprintf(stderr, "############################\n");
    list = toml_lexer(
        "a = 10\n"
        "b.c.d = 20\n"
        "[c]\n"
        "  d = true",
        &err);
    node = toml_parse(list, NULL);
    ast_node_dump(node);
    fprintf(stderr, "\n");

    gen_err_t gen_err;

    res = (table_object_t *)ast_node_gen(node, &gen_err);

    if (!res) {
        fprintf(stderr, "%s\n", gen_err.msg);
    }

    object_dump(res);
    fprintf(stderr, "\n");

    object_free(res);
    ast_node_free(node);
    token_list_free(list);

#endif

    {
        const char *test_file = "toml/test.toml";

        FILE *fp = fopen(test_file, "rb");
        assert(fp);

        size_t size;

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *cont = malloc(size + 1);
        assert(cont);
        cont[size] = 0;

        assert(fread(cont, 1, size, fp) == size);

        fclose(fp);

        toml_err_t err;
        toml_object_t *obj = toml_load(cont, &err);
        object_t *lookup;
        
        free(cont);

        if (!obj) {
            fprintf(stderr, "error: %s at %lu\n", err.msg, err.pos);
            return 0;
        }

        object_dump(obj);
        fprintf(stderr, "\n");

        lookup = toml_lookup(obj, "inbound.protocol", NULL);
        object_dump(lookup);
        fprintf(stderr, "\n");

        lookup = toml_lookup(obj, "inbound.port", NULL);
        object_dump(lookup);
        fprintf(stderr, "\n");

        toml_free(obj);
    }

    return 0;
}
