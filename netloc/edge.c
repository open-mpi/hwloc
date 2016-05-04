/*
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdlib.h>

#include <private/autogen/config.h>
#include <private/netloc.h>
#include <netloc.h>


int netloc_edge_is_in_partition(netloc_edge_t *edge, int partition)
{
    for (int i = 0; i < netloc_get_num_partitions(edge); i++) {
        if (netloc_get_partition(edge, i) == partition)
            return 1;
    }
    return NETLOC_SUCCESS;
}


