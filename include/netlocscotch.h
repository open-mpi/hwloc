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

#ifndef _NETLOCSCOTCH_H_
#define _NETLOCSCOTCH_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for asprintf
#endif

/* Includes for Scotch */
#include <stdio.h>
#include <netloc.h>
#include <scotch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    netloc_node_t *node;
    int core;
} netlocscotch_core_t;

int netlocscotch_build_current_arch(SCOTCH_Arch *subarch);
int netlocscotch_get_mapping_from_comm_file(char *filename, int *pnum_processes,
        netlocscotch_core_t **pcores);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif // _NETLOC_H_
