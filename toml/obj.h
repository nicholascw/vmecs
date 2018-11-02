#ifndef _TOML_OBJ_H_
#define _TOML_OBJ_H_

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "table.h"

struct object_t_tag;

typedef void (*object_free_t)(struct object_t_tag *obj);
typedef void (*object_dump_t)(struct object_t_tag *obj);

enum {
    OBJECT_TYPE_INT,
    OBJECT_TYPE_BOOL,
    OBJECT_TYPE_STRING,
    OBJECT_TYPE_TABLE
};

typedef uint8_t object_type_t;

#define OBJECT_HEADER \
    object_free_t free_func; \
    object_dump_t dump_func; \
    object_type_t type;

#define IS_TYPE(obj, t) ((obj)->type == OBJECT_TYPE_ ## t)

typedef struct object_t_tag {
    OBJECT_HEADER
} object_t;

static inline void
object_free(void *obj)
{
    if (obj) {
        if (((object_t *)obj)->free_func)
            ((object_t *)obj)->free_func(obj);
        else
            free(obj);
    }
}

static inline void
object_dump(void *obj)
{
    if (!obj) {
        fprintf(stderr, "(NULL)");
    } else if (((object_t *)obj)->dump_func) {
        ((object_t *)obj)->dump_func(obj);
    } else {
        fprintf(stderr, "object at %p", obj);
    }
}

static inline void
object_init(object_t *obj, object_type_t type)
{
    memset(obj, 0, sizeof(*obj));
    obj->type = type;
}

typedef struct {
    OBJECT_HEADER
    long long val;
} int_object_t;

int_object_t *
int_object_new(long long val);

typedef struct {
    OBJECT_HEADER
    bool val;
} bool_object_t;

bool_object_t *
bool_object_new(bool val);

typedef struct {
    OBJECT_HEADER
    char *str;
} string_object_t;

string_object_t *
string_object_new(const char *str);

typedef struct {
    OBJECT_HEADER
    hash_table_t *table;
} table_object_t;

table_object_t *
table_object_new();

void
table_object_insert(table_object_t *obj, const char *key, object_t *val);

object_t *
table_object_lookup(table_object_t *obj, const char *key);

#endif
