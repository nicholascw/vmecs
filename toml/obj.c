#include <stdlib.h>
#include <stdio.h>

#include "obj.h"

static void
_int_object_dump(object_t *_obj)
{
    int_object_t *obj = (int_object_t *)_obj;
    fprintf(stderr, "%lld", obj->val);
}

int_object_t *
int_object_new(long long val)
{
    int_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_INT);
    ret->dump_func = _int_object_dump;
    ret->val = val;

    return ret;
}

static void
_bool_object_dump(object_t *_obj)
{
    bool_object_t *obj = (bool_object_t *)_obj;

    if (obj->val)
        fprintf(stderr, "true");
    else
        fprintf(stderr, "false");
}

bool_object_t *
bool_object_new(bool val)
{
    bool_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_BOOL);
    ret->dump_func = _bool_object_dump;
    ret->val = val;

    return ret;
}

static void
_string_object_free(object_t *_obj)
{
    string_object_t *obj = (string_object_t *)_obj;

    if (obj) {
        free(obj->str);
        free(obj);
    }
}

static void
_string_object_dump(object_t *_obj)
{
    string_object_t *obj = (string_object_t *)_obj;
    fprintf(stderr, "\"%s\"", obj->str);
}

string_object_t *
string_object_new(const char *str)
{
    string_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_STRING);
    ret->free_func = _string_object_free;
    ret->dump_func = _string_object_dump;
    ret->str = strdup(str);

    return ret;
}

static void
_table_object_free(object_t *_obj)
{
    table_object_t *obj = (table_object_t *)_obj;

    if (obj) {
        HASH_TABLE_FOREACH(obj->table, entry, {
            object_free(entry->val);
        });

        hash_table_free(obj->table);
        free(obj);
    }
}

static void
_table_object_dump(object_t *_obj)
{
    table_object_t *obj = (table_object_t *)_obj;
    bool is_first = true;

    fprintf(stderr, "{");

    HASH_TABLE_FOREACH(obj->table, entry, {
        if (is_first) {
            is_first = false;
        } else {
            fprintf(stderr, ",");
        }

        fprintf(stderr, "\"%s\":", entry->key);
        object_dump(entry->val);
    });

    fprintf(stderr, "}");
}

table_object_t *
table_object_new()
{
    table_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_TABLE);
    ret->free_func = _table_object_free;
    ret->dump_func = _table_object_dump;
    ret->table = hash_table_new();

    return ret;
}

void
table_object_insert(table_object_t *obj, const char *key, object_t *val)
{
    hash_table_insert(obj->table, key, val);
}

object_t *
table_object_lookup(table_object_t *obj, const char *key)
{
    return hash_table_lookup(obj->table, key);
}
