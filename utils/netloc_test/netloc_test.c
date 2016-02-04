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

#include <jansson.h>

#include "netloc/dc.h"
#include "private/netloc.h"

static char *read_param(int *argc, char ***argv)
{
    if (!*argc)
        return NULL;

    char *ret = **argv;
    (*argv)++;
    (*argc)--;

    return ret;
}

int main(int argc, char **argv)
{
    int ret;

    if (argc < 2 || argc > 3) {
        goto wrong_params;
    }

    int cargc = argc;
    char **cargv = argv;
    char *netloc_dir;
    char *param;

    read_param(&cargc, &cargv);
    char *path = read_param(&cargc, &cargv);
    asprintf(&netloc_dir, "file://%s/%s", path, "netloc");

    char *partition_name = read_param(&cargc, &cargv);

    netloc_network_t *network = NULL;
    netloc_topology_t topology;

    // Find a specific InfiniBand network
    network = netloc_dt_network_t_construct();
    network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;

    // Search for the specific network
    ret = netloc_find_network(netloc_dir, network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: network not found!\n");
        netloc_dt_network_t_destruct(network);
        return NETLOC_ERROR;
    }
    printf("Found Network: %s\n", netloc_pretty_print_network_t(network));

    // Attach to the network
    ret = netloc_attach(&topology, *network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }

    //netloc_topology_set_all_partitions(topology);
    netloc_topology_keep_partition(topology, partition_name);
    //netloc_read_hwloc(topology);
    netloc_topology_simplify(topology);
    netloc_topology_analyse_as_tree(topology);

    return 0;

wrong_params:
    printf("Usage: %s <netloc_dir>\n", argv[0]);
    return -1;
}

