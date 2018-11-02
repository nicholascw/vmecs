#ifndef _TOML_TABLE_H_
#define _TOML_TABLE_H_

#include <stdlib.h>

typedef struct {
    char *key;
    void *val;
} hash_table_entry_t;

typedef struct {
    hash_table_entry_t *tab;
    size_t size;
    size_t cap;
} hash_table_t;

hash_table_t *
hash_table_new();

void
hash_table_free(hash_table_t *table);

// val != NULL
void
hash_table_insert(hash_table_t *table, const char *key, void *val);

void *
hash_table_lookup(hash_table_t *table, const char *key);

#define HASH_TABLE_FOREACH(table, v, block) \
    do { \
        hash_table_t *t = (table); \
        size_t i = 0; \
        for (; i < t->cap; i++) { \
            if (t->tab[i].key) { \
                hash_table_entry_t *v = t->tab + i; \
                block; \
            } \
        } \
    } while (0);

#endif
