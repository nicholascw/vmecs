#include <assert.h>
#include <string.h>

#include "table.h"

#define TABLE_INIT_BUF 8 // has to be power of 2

static size_t _hash_table_hash(const char *key)
{
    // djb2 from http://www.cse.yorku.ca/~oz/hash.html
    size_t hash = 5381;
    int c;
    while ((c = *key++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

hash_table_t *
hash_table_new()
{
    hash_table_t *ret = malloc(sizeof(*ret));
    assert(ret);

    ret->tab = calloc(TABLE_INIT_BUF, sizeof(*ret->tab));
    assert(ret->tab);

    ret->size = 0;
    ret->cap = TABLE_INIT_BUF;

    return ret;
}

void
hash_table_free(hash_table_t *table)
{
    size_t i;

    for (i = 0; i < table->cap; i++) {
        free(table->tab[i].key);
    }

    free(table->tab);
    free(table);
}

static void
_hash_table_insert(hash_table_t *table, char *key, void *val);

static void
_hash_table_expand(hash_table_t *table)
{
    hash_table_entry_t *old_tab;
    size_t i, old_cap;

    old_tab = table->tab;
    old_cap = table->cap;

    table->cap <<= 1;
    table->tab = calloc(table->cap, sizeof(*table->tab));
    table->size = 0;

    for (i = 0; i < old_cap; i++) {
        if (old_tab[i].key) {
            _hash_table_insert(table, old_tab[i].key, old_tab[i].val);
        }
    }

    free(old_tab);
}

static hash_table_entry_t *
_hash_table_find(hash_table_t *table, const char *key)
{
    size_t hash = _hash_table_hash(key);
    size_t pos = hash & (table->cap - 1);
    size_t i;

    for (i = pos; i < table->cap; i++) {
        if (!table->tab[i].key ||
            strcmp(table->tab[i].key, key) == 0) {
            return table->tab + i;
        }
    }

    for (i = 0; i < pos; i++) {
        if (!table->tab[i].key ||
            strcmp(table->tab[i].key, key) == 0) {
            return table->tab + i;
        }
    }

    return NULL;
}

static void
_hash_table_insert(hash_table_t *table, char *key, void *val)
{
    hash_table_entry_t *found;

    if (table->size == table->cap) {
        _hash_table_expand(table);
    }

    found = _hash_table_find(table, key);
    assert(found);

    free(found->key);
    found->key = key;
    found->val = val;
}

// val != NULL
void
hash_table_insert(hash_table_t *table, const char *key, void *val)
{
    _hash_table_insert(table, strdup(key), val);
}

void *
hash_table_lookup(hash_table_t *table, const char *key)
{
    hash_table_entry_t *found = _hash_table_find(table, key);
    return found && found->key ? found->val : NULL;
}
