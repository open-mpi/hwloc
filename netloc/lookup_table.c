/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2015-2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <netloc.h>
#include <private/netloc.h>

#define HASH_GROWS_BY 8

/**
 * Constructor for netloc_lookup_table_entry_t
 *
 * User is responsible for calling the destructor on the handle returned.
 *
 * Returns
 *   A newly allocated pointer to the lookup table entry
 */
netloc_lookup_table_entry_t *netloc_lookup_table_entry_t_construct();

/**
 * Do a -shallow copy- of the data in the table entry.
 *
 * User is responsible for calling the destructor on the handle returned.
 *
 * \param hte A valid pointer to a lookup table entry
 * \param dup Set to 1 if keys are internally duplicated
 *
 * Returns
 *   NULL if an error occurs
 *   otherwise returns a newly allocated pointer to a lookup table entry.
 */
netloc_lookup_table_entry_t *netloc_copy_lookup_table_entry_t(netloc_lookup_table_entry_t* hte, int dup);

/**
 * Destructor for a netloc_lookup_table_entry_t
 *
 * \param hte A void pointer to a lookup table entry
 * \param dup Set to 1 if keys are internally duplicated
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
int netloc_lookup_table_entry_t_destruct(netloc_lookup_table_entry_t* hte, int dup);


/**********************************************************************
 * Function Definitions
 **********************************************************************/
struct netloc_dt_lookup_table_iterator *netloc_dt_lookup_table_iterator_t_construct(struct netloc_dt_lookup_table *ht)
{
    struct netloc_dt_lookup_table_iterator* hti = NULL;

    hti = (struct netloc_dt_lookup_table_iterator*)malloc(sizeof(struct netloc_dt_lookup_table_iterator));
    if( NULL == hti ) {
        return NULL;
    }

    hti->htp = ht;
    hti->loc = 0;
    hti->at_end = false;

    return hti;
}

int netloc_dt_lookup_table_iterator_t_destruct(struct netloc_dt_lookup_table_iterator* hti)
{
    if( NULL == hti ) {
        return NETLOC_SUCCESS;
    }

    hti->htp = NULL;
    hti->loc = 0;
    hti->at_end = true;

    free(hti);
    return NETLOC_SUCCESS;
}

size_t netloc_lookup_table_size(struct netloc_dt_lookup_table *table) {
    if( NULL == table ) {
        return 0;
    } else {
        return table->ht_used_size;
    }
}

size_t netloc_lookup_table_size_alloc(struct netloc_dt_lookup_table *table) {
    if( NULL == table ) {
        return 0;
    } else {
        return table->ht_size;
    }
}

const char * netloc_lookup_table_iterator_next_key(struct netloc_dt_lookup_table_iterator* hti)
{
    size_t i;

    for(i = hti->loc; i < netloc_lookup_table_size(hti->htp); ++i) {
        if( NULL != hti->htp->ht_entries[i] ) {
            hti->loc = i+1;
            return hti->htp->ht_entries[i]->key;
        }
    }

    hti->loc = netloc_lookup_table_size(hti->htp);
    hti->at_end = true;

    return NULL;
}

unsigned long netloc_lookup_table_iterator_next_key_int(struct netloc_dt_lookup_table_iterator* hti)
{
    size_t i;

    for(i = hti->loc; i < netloc_lookup_table_size(hti->htp); ++i) {
        if( NULL != hti->htp->ht_entries[i] ) {
            hti->loc = i+1;
            return hti->htp->ht_entries[i]->__key__;
        }
    }

    hti->loc = netloc_lookup_table_size(hti->htp);
    hti->at_end = true;

    return 0;
}

void * netloc_lookup_table_iterator_next_entry(struct netloc_dt_lookup_table_iterator* hti)
{
    size_t i;

    for(i = hti->loc; i < netloc_lookup_table_size(hti->htp); ++i) {
        if( NULL != hti->htp->ht_entries[i] ) {
            hti->loc = i+1;
            return hti->htp->ht_entries[i]->value;
        }
    }

    hti->loc = netloc_lookup_table_size(hti->htp);
    hti->at_end = true;

    return NULL;
}

bool netloc_lookup_table_iterator_at_end(struct netloc_dt_lookup_table_iterator *hti) {
    return hti->at_end;
}

void netloc_lookup_table_iterator_reset(struct netloc_dt_lookup_table_iterator *hti) {
    hti->loc = 0;
    hti->at_end = false;
}

netloc_lookup_table_entry_t *netloc_lookup_table_entry_t_construct()
{
    netloc_lookup_table_entry_t* hte = NULL;

    hte = (netloc_lookup_table_entry_t*)malloc(sizeof(netloc_lookup_table_entry_t));
    if( NULL == hte ){
        return NULL;
    }

    hte->__key__ = 0;
    hte->key = NULL;
    hte->value = NULL;

    return hte;
}

netloc_lookup_table_entry_t *netloc_copy_lookup_table_entry_t(netloc_lookup_table_entry_t* orig, int dup)
{
    netloc_lookup_table_entry_t* hte = NULL;
    hte = netloc_lookup_table_entry_t_construct();

    hte->__key__ = orig->__key__;
    hte->key = dup ? strdup(orig->key) : orig->key;
    hte->value = orig->value;

    return hte;
}

int netloc_lookup_table_entry_t_destruct(netloc_lookup_table_entry_t* hte, int dup)
{
    if( NULL == hte ) {
        return NETLOC_SUCCESS;
    }

    hte->__key__ = 0;

    if (dup) {
        free((char*) hte->key);
    }
    hte->key = NULL;
    hte->value = NULL;

    free(hte);

    return NETLOC_SUCCESS;
}

int netloc_dt_lookup_table_t_copy(struct netloc_dt_lookup_table *from, struct netloc_dt_lookup_table *to)
{
    size_t i;
    int dup;

    if( NULL == from || NULL == to ) {
        return NETLOC_ERROR;
    }

    dup = !(from->flags & NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);

    netloc_lookup_table_init(to, netloc_lookup_table_size(from), from->flags);

    for(i = 0; i < from->ht_size; ++i ) {
        if( NULL != from->ht_entries[i] ) {
            to->ht_entries[i] = netloc_copy_lookup_table_entry_t(from->ht_entries[i], dup);
        }
    }
    to->ht_used_size = from->ht_used_size;

    return NETLOC_SUCCESS;
}

int netloc_lookup_table_init(struct netloc_dt_lookup_table *ht, size_t size, unsigned long flags)
{
    size_t i;

    if( NULL == ht ) {
        fprintf(stderr, "Error: Hash Table handle is NULL!\n");
        return NETLOC_ERROR;
    }

    ht->ht_entries = (netloc_lookup_table_entry_t**)malloc(sizeof(netloc_lookup_table_entry_t*) * size);
    if( NULL == ht->ht_entries ) {
        return NETLOC_ERROR;
    }
    ht->ht_size = size;
    ht->ht_used_size = 0;
    ht->flags = flags;

    for(i = 0; i < ht->ht_size; ++i ) {
        ht->ht_entries[i] = NULL;
    }

    return NETLOC_SUCCESS;
}

int netloc_lookup_table_destroy(struct netloc_dt_lookup_table *ht)
{
    size_t i;
    int dup;

    if( NULL == ht ) {
        fprintf(stderr, "Error: Hash Table handle is NULL!\n");
        return NETLOC_ERROR;
    }

    dup = !(ht->flags & NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);

    for(i = 0; i < ht->ht_size; ++i) {
        if( NULL != ht->ht_entries[i] ) {
            netloc_lookup_table_entry_t_destruct(ht->ht_entries[i], dup);
        }
    }
    free(ht->ht_entries);
    ht->ht_entries = NULL;
    ht->ht_size = 0;
    ht->ht_used_size = 0;

    return NETLOC_SUCCESS;
}

int netloc_lookup_table_append(struct netloc_dt_lookup_table *ht, const char *key, void *value)
{
    unsigned long hashed_key;
    // JJH : Add hash function
    hashed_key = 0;

    return netloc_lookup_table_append_with_int(ht, key, hashed_key, value);
}

int netloc_lookup_table_append_with_int(struct netloc_dt_lookup_table *ht, const char *key, unsigned long key_int, void *value)
{
    int dup = !(ht->flags & NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);
    size_t i, a;
    int len;

    // Check if key already exists!
    // If not then we are looking at the next free entry
    for(i = 0; i < ht->ht_size; ++i ) {
        if( NULL == ht->ht_entries[i] ) {
            break;
        }
        else {
            if( 0 != key_int ) {
                if( key_int == ht->ht_entries[i]->__key__ ) {
                    return NETLOC_ERROR_EXISTS;
                }
            }
            else {
                if( strlen(key) > strlen(ht->ht_entries[i]->key) ) {
                    len = strlen(key);
                } else {
                    len = strlen(ht->ht_entries[i]->key);
                }
                if( 0 == strncmp(ht->ht_entries[i]->key, key, len) ) {
                    return NETLOC_ERROR_EXISTS;
                }
            }
        }
    }

    /*
     * Grow the lookup table as needed
     */
    if( i == ht->ht_size ) {
        ht->ht_size += HASH_GROWS_BY;
        ht->ht_entries = (netloc_lookup_table_entry_t**)realloc(ht->ht_entries, sizeof(netloc_lookup_table_entry_t*) * ht->ht_size);
        for(a = i; a < ht->ht_size; ++a) {
            ht->ht_entries[a] = NULL;
        }
    }

    ht->ht_entries[i] = netloc_lookup_table_entry_t_construct();
    ht->ht_entries[i]->key   = dup ? strdup(key) : key;
    ht->ht_entries[i]->value = value;
    ht->ht_entries[i]->__key__ = key_int;
    ht->ht_used_size += 1;

    return NETLOC_SUCCESS;
}

void * netloc_lookup_table_access(struct netloc_dt_lookup_table *ht, const char *key)
{
    unsigned long hashed_key;
    // JJH : Add hash function
    hashed_key = 0;

    return netloc_lookup_table_access_with_int(ht, key, hashed_key);
}

void * netloc_lookup_table_access_with_int(struct netloc_dt_lookup_table *ht, const char *key, unsigned long key_int)
{
    size_t i;
    int len;

    for(i = 0; i < ht->ht_size; ++i ) {
        if( NULL == ht->ht_entries[i] ) {
            continue;
        }
        else {
            if( 0 != key_int ) {
                if( key_int == ht->ht_entries[i]->__key__ ) {
                    return ht->ht_entries[i]->value;
                }
            }
            else {
                if( strlen(key) > strlen(ht->ht_entries[i]->key) ) {
                    len = strlen(key);
                } else {
                    len = strlen(ht->ht_entries[i]->key);
                }
                if( 0 == strncmp(ht->ht_entries[i]->key, key, len) ) {
                    return ht->ht_entries[i]->value;
                }
            }
        }
    }

    return NULL;
}

int netloc_lookup_table_replace(struct netloc_dt_lookup_table *ht, const char *key, void *value)
{
    unsigned long hashed_key;
    // JJH : Add hash function
    hashed_key = 0;

    return netloc_lookup_table_replace_with_int(ht, key, hashed_key, value);
}

int netloc_lookup_table_replace_with_int(struct netloc_dt_lookup_table *ht, const char *key, unsigned long key_int, void *value)
{
    size_t i;
    int len;

    // Find this value
    for(i = 0; i < ht->ht_size; ++i ) {
        if( NULL == ht->ht_entries[i] ) {
            break;
        }
        else {
            if( 0 != key_int ) {
                if( key_int == ht->ht_entries[i]->__key__ ) {
                    ht->ht_entries[i]->value = value;
                }
            }
            else {
                if( strlen(key) > strlen(ht->ht_entries[i]->key) ) {
                    len = strlen(key);
                } else {
                    len = strlen(ht->ht_entries[i]->key);
                }
                if( 0 == strncmp(ht->ht_entries[i]->key, key, len) ) {
                    ht->ht_entries[i]->value = value;
                }
            }
        }
    }

    return NETLOC_SUCCESS;
}

int netloc_lookup_table_remove(struct netloc_dt_lookup_table *ht, const char *key)
{
    unsigned long hashed_key;
    // JJH : Add hash function
    hashed_key = 0;

    return netloc_lookup_table_remove_with_int(ht, key, hashed_key);
}
int netloc_lookup_table_remove_with_int(struct netloc_dt_lookup_table *ht, const char *key, unsigned long key_int)
{
    size_t i;
    int len;
    int idx_to_remove = -1;
    int dup = !(ht->flags & NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);

    // Find this value
    for(i = 0; i < ht->ht_size; ++i ) {
        if( NULL == ht->ht_entries[i] ) {
            break;
        }
        else {
            if( 0 != key_int ) {
                if( key_int == ht->ht_entries[i]->__key__ ) {
                    idx_to_remove = i;
                    break;
                }
            }
            else {
                if( strlen(key) > strlen(ht->ht_entries[i]->key) ) {
                    len = strlen(key);
                } else {
                    len = strlen(ht->ht_entries[i]->key);
                }
                if( 0 == strncmp(ht->ht_entries[i]->key, key, len) ) {
                    idx_to_remove = i;
                    break;
                }
            }
        }
    }

    if( idx_to_remove < 0 ) {
        return NETLOC_ERROR;
    }

    netloc_lookup_table_entry_t_destruct(ht->ht_entries[idx_to_remove], dup);
    for( i = idx_to_remove; i < ht->ht_size-1; ++i ) {
        ht->ht_entries[i] = ht->ht_entries[i+1];
    }
    ht->ht_entries[ht->ht_size-1] = NULL;

    ht->ht_used_size -= 1;

    return NETLOC_SUCCESS;
}

void netloc_lookup_table_pretty_print(struct netloc_dt_lookup_table *ht)
{
    size_t i;

    for(i = 0; i < ht->ht_size; ++i ) {
        printf("%3d) ", (int)i);
        if( NULL != ht->ht_entries[i] ) {
            printf("%4s [%p]", ht->ht_entries[i]->key, ht->ht_entries[i]->value);
        } else {
            printf("NULL");
        }
        printf("\n");
    }
}

json_t* netloc_dt_lookup_table_t_json_encode(struct netloc_dt_lookup_table *table,
                                             json_t* (*func)(const char * key, void *value))
{
    json_t *json_lt = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    const char * key = NULL;
    void * value = NULL;

    json_lt = json_object();

    hti = netloc_dt_lookup_table_iterator_t_construct(table);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        value = netloc_lookup_table_access(table, key);

        json_object_set_new(json_lt, key, func(key, value));
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti);

    return json_lt;
}


struct netloc_dt_lookup_table* netloc_dt_lookup_table_t_json_decode(json_t *json_lt,
                                                               void * (*func)(const char *key, json_t* json_obj))
{
    struct netloc_dt_lookup_table *table = NULL;
    const char * key = NULL;
    json_t     * value = NULL;

    table = calloc(1, sizeof(*table));
    netloc_lookup_table_init(table, json_object_size(json_lt), 0);

    json_object_foreach(json_lt, key, value) {
        netloc_lookup_table_append(table, key, func(key, value));
    }

    return table;
}
