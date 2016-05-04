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
#include <netloc.h>

int hwloc_get_core_number(int *pnum_cores)
{
    // TODO
    int num_cores;
    num_cores = 32;

    *pnum_cores = num_cores;
    return NETLOC_SUCCESS;
}


