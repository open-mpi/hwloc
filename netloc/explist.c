/*
 * Copyright © 2016-2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <stdlib.h>

#include <private/netloc.h>

netloc_explist_t *netloc_explist_init(int size)
{
    netloc_explist_t *list = (netloc_explist_t *)
        malloc(sizeof(netloc_explist_t));
    list->size = 0;

    if (size > 0) {
        list->array = (void **)
            malloc(size*sizeof(void *));
        list->max_size = size;
    }
    else {
        list->array = NULL;
        list->max_size = 0;
    }

    return list;
}

void netloc_explist_add(netloc_explist_t *list, void *elem)
{
    /* We extend the array if needed */
    if (list->size >= list->max_size) {
        int new_max = 2*list->max_size;
        list->array = (void **)
            realloc(list->array, new_max*sizeof(void *));

        list->max_size = new_max;
    }

    list->array[list->size] = elem;
    list->size++;
}

void netloc_explist_set(netloc_explist_t *list, int idx, void *elem)
{
    /* We extend the array if needed */
    if (idx >= list->size) {
        int new_size = idx+1;

        if (new_size >= list->max_size) {
            list->array = (void **)
                realloc(list->array, new_size*sizeof(void *));

            list->max_size = new_size;
        }

        for (int i = list->size; i < new_size; i++)
            list->array[i] = NULL;
        list->size = new_size;
    }

    list->array[idx] = elem;
}


int netloc_explist_get_size(netloc_explist_t *list)
{
    return list->size;
}

void *netloc_explist_get(netloc_explist_t *list, int idx)
{
    if (idx >= list->size)
        return NULL;
    return list->array[idx];
}

void netloc_explist_destroy(netloc_explist_t *list)
{
    free(list->array);
    free(list);
}

void **netloc_explist_get_array_and_destroy(netloc_explist_t *list)
{
    if (!list)
        return NULL;
    void **array = list->array;
    int size = list->size;
    free(list);
    return realloc(array, size*sizeof(void **));
}


