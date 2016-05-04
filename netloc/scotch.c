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

static int tree_to_scotch_arch(netloc_arch_tree_t *tree, SCOTCH_Arch *scotch);

static int build_subarch(netloc_arch_t *arch, char **nodelist,
        int num_nodes, SCOTCH_Arch *scotch, SCOTCH_Arch *subarch)
{
    int ret;
    if (arch->type == NETLOC_ARCH_TREE) {
        netloc_arch_tree_t *tree = arch->arch.tree;
        int num_cores = tree->num_cores;
        int *idx_list = (int *)malloc(num_nodes*sizeof(int));
        netloc_arch_host_t *all_hosts = tree->hosts;

        char *last_nodename = "";
        netloc_arch_host_t *host;
        int core_idx;
        for (int n = 0; n < num_nodes; n++) {
            char *nodename = nodelist[n];

            /* Another processor from the previous node */
            if (!strcmp(nodename, last_nodename)) {
                if (core_idx >= num_cores) {
                    fprintf(stderr, "Error: node %s is present more than"
                            "%d times in the node list file\n", nodename, core_idx);
                    return NETLOC_ERROR;
                }
            } else {
                core_idx = 0;
                HASH_FIND_STR(all_hosts, nodename, host);
                last_nodename = nodename;
                if (host) {
                    idx_list[n] = host->idx;
                }
                else {
                    fprintf(stderr, "Error: node %s not present in the all architecture\n", nodename);
                    return NETLOC_ERROR;
                }
            }
            idx_list[n] = num_cores*host->idx+core_idx;
            core_idx++;
        }

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

int tree_to_scotch_arch(netloc_arch_tree_t *tree, SCOTCH_Arch *scotch)
{
    int ret;
    /* Add hwloc information to go beyond nodes */
    /* TODO improve by using the tree from hwloc */
    tree->num_levels++;
    tree->degrees = (int *)realloc(tree->degrees, tree->num_levels*sizeof(int));
    tree->throughput = (int *)realloc(tree->throughput, tree->num_levels*sizeof(int));

    int num_cores;
    ret = hwloc_get_core_number(&num_cores);

    if (ret != NETLOC_SUCCESS) {
        return ret;
    }

    tree->degrees[tree->num_levels-1] = num_cores;
    tree->throughput[tree->num_levels-1] = 1000; // TODO
    tree->num_cores = num_cores;

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
    ret = tree_to_scotch_arch(arch.arch.tree, scotcharch);
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

