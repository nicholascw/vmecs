#include <stdlib.h>

#include "obj.h"

int_object_t *
int_object_new(long long val)
{
    int_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_INT);
    ret->val = val;

    return ret;
}

bool_object_t *
bool_object_new(bool val)
{
    bool_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_BOOL);
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

string_object_t *
string_object_new(const char *str)
{
    string_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_STRING);
    ret->free_func = _string_object_free;
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

table_object_t *
table_object_new()
{
    table_object_t *ret = malloc(sizeof(*ret));
    assert(ret);

    object_init((object_t *)ret, OBJECT_TYPE_TABLE);
    ret->free_func = _table_object_free;
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
