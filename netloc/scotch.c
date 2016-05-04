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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <scotch.h>

#include <netloc.h>
#include <netlocscotch.h>
#include <private/netloc.h>
#include <hwloc.h>
#include "support.h"

static int arch_to_scotch_arch(netloc_arch_t *arch, SCOTCH_Arch *scotch);
static int host_get_idx(netloc_arch_t *arch, int num_hosts,
        netloc_arch_host_t **host_list, int **pidx);

int host_get_idx(netloc_arch_t *arch, int num_hosts,
        netloc_arch_host_t **host_list, int **pidx)
{
    int core_idx = 0;
    netloc_arch_host_t *host;
    netloc_arch_host_t *last_host = NULL;

    int *idx = (int *)malloc(num_hosts*sizeof(int));

    int num_cores = arch->arch.tree->num_cores;

    for (int n = 0; n < num_hosts; n++) {
        netloc_arch_host_t *host = host_list[n];
        if (host == last_host) {
        } else {
            core_idx = 0;
            last_host = host;
        }
        core_idx++;
        idx[n] = host->idx*num_cores+core_idx;
    }

    *pidx = idx;

    return NETLOC_SUCCESS;
}


static int build_subarch(netloc_arch_t *arch, char **nodelist,
        int num_nodes, SCOTCH_Arch *scotch, SCOTCH_Arch *subarch)
{
    int ret;
    if (arch->type == NETLOC_ARCH_TREE) {
        netloc_arch_host_t **host_list;
        ret = netloc_arch_find_current_hosts(arch, nodelist, num_nodes, &host_list);
        if (ret != 0) {
            fprintf(stderr, "Error: netloc_arch_find_current_hosts\n");
            return NETLOC_ERROR;
        }

        int *idx_list;
        host_get_idx(arch, num_nodes, host_list, &idx_list);

        ret = SCOTCH_archSub(subarch, scotch, num_nodes, idx_list);
        if (ret != 0) {
            fprintf(stderr, "Error: SCOTCH_archSub failed\n");
            return NETLOC_ERROR;
        }

    } else {
        // TODO
    }

    return NETLOC_SUCCESS;
}

int arch_to_scotch_arch(netloc_arch_t *arch, SCOTCH_Arch *scotch)
{
    netloc_arch_tree_t *tree = arch->arch.tree;
    int ret;

    ret = SCOTCH_archTleaf(scotch, tree->num_levels, tree->degrees, tree->throughput);
    if (ret != 0) {
        fprintf(stderr, "Error: SCOTCH_archTleaf failed\n");
        return NETLOC_ERROR;
    }

    return NETLOC_SUCCESS;
}

int netlocscotch_build_current_arch(SCOTCH_Arch *subarch)
{
    int ret;
    /* First we need to get the topology of the whole machine */
    netloc_arch_t arch;
    ret = netloc_arch_build(&arch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    SCOTCH_Arch *scotcharch;
    scotcharch = (SCOTCH_Arch *)malloc(sizeof(SCOTCH_Arch));
    ret = arch_to_scotch_arch(&arch, scotcharch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Then we retrieve the list of nodes given by the resource manager */
    int num_nodes;
    char **nodes;
    ret = netloc_get_current_nodes(&num_nodes, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Now we can build the sub architecture */
    ret = build_subarch(&arch, nodes, num_nodes, scotcharch, subarch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    return NETLOC_SUCCESS;
}

