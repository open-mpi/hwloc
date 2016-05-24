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

#include <netloc.h>
#include <private/netloc.h>

int host_cmp(const void *a, const void *b) 
{ 
    const netloc_arch_host_t *host_a = *(const netloc_arch_host_t **)a;
    const netloc_arch_host_t *host_b = *(const netloc_arch_host_t **)b;
    return host_a->idx-host_b->idx; 
} 

int main(int argc, char **argv)
{
    int ret;
    /* First we need to get the topology of the whole machine */
    netloc_arch_t arch;
    ret = netloc_arch_build(&arch, 1);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Then we retrieve the list of nodes given by the resource manager */
    int num_nodes;
    char **nodes;
    ret = netloc_get_current_cores(&num_nodes, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Now we can get the host list */
    netloc_arch_host_t **host_list;
    ret = netloc_arch_find_current_hosts(&arch, nodes, num_nodes, &host_list);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Order the idx_list to have the nodes sorted */
    qsort(host_list, num_nodes, sizeof(netloc_arch_host_t *), host_cmp);

    /* Show the list */
    for (int n = 0; n < num_nodes; n++) {
        printf("%s%s", ((char **)arch.hosts)[host_list[n]->host_idx], n != num_nodes-1 ? " ": "\n");
    }

    return NETLOC_SUCCESS;
}
