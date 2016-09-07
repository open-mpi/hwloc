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

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdlib.h>

#include <private/autogen/config.h>
#include <private/netloc.h>
#include <netloc.h>

static UT_icd node_physical_links_icd = {
    sizeof(netloc_physical_link_t *), NULL, NULL, NULL
};

static UT_icd node_physical_nodes_icd = {
    sizeof(netloc_node_t *), NULL, NULL, NULL
};

static UT_icd node_partitions_icd = { sizeof(int), NULL, NULL, NULL };

netloc_node_t * netloc_node_construct()
{
    netloc_node_t *node = NULL;

    node = (netloc_node_t*)malloc(sizeof(netloc_node_t));
    if( NULL == node ) {
        return NULL;
    }

    node->physical_id[0]  = '\0';
    node->logical_id   = -1;
    node->type    = NETLOC_NODE_TYPE_INVALID;
    utarray_new(node->physical_links, &node_physical_links_icd);
    node->description  = NULL;
    node->userdata     = NULL;
    node->edges        = NULL;
    utarray_new(node->subnodes, &node_physical_nodes_icd);
    node->paths        = NULL;
    node->hostname     = NULL;
    utarray_new(node->partitions, &node_partitions_icd);
    node->hwlocTopo = NULL;

    return node;
}

int netloc_node_destruct(netloc_node_t * node)
{
    // TODO
    return NETLOC_SUCCESS;
}

char * netloc_node_pretty_print(netloc_node_t* node)
{
    char * str = NULL;

    asprintf(&str, " [%23s]/[%d] -- %s (%d links)",
             node->physical_id,
             node->logical_id,
             node->description,
             utarray_len(node->physical_links));

    return str;
}

int netloc_node_is_in_partition(netloc_node_t *node, int partition)
{
    for (int i = 0; i < netloc_get_num_partitions(node); i++) {
        if (netloc_get_partition(node, i) == partition)
            return 1;
    }
    return NETLOC_SUCCESS;
}


