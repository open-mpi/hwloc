/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <netloc.h>
#include "support.h"

int netloc_attach(struct netloc_topology ** topology_ptr, netloc_network_t network)
{
    struct netloc_topology *topology = NULL;

    /*
     * Allocate Memory
     */
    topology = (struct netloc_topology *)malloc(sizeof(struct netloc_topology) * 1);
    if( NULL == topology ) {
        return NETLOC_ERROR;
    }

    /*
     * Initialize the structure
     */
    topology->network    = netloc_dt_network_t_dup(&network);

    topology->nodes_loaded   = false;
    topology->num_nodes      = 0;
    topology->nodes          = NULL;
    topology->edges          = NULL;
    topology->partitions     = NULL;
    topology->num_partitions = 0;
    topology->topos          = NULL;
    topology->num_topos      = 0;

    /*
     * Make the pointer live
     */
    (*topology_ptr) = topology;

    return NETLOC_SUCCESS;
}

int netloc_detach(struct netloc_topology * topology)
{
    int i;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_edge_t *cur_edge = NULL;

    /*
     * Sanity Check
     */
    if( NULL == topology ) {
        fprintf(stderr, "Error: Detaching from a NULL pointer\n");
        return NETLOC_ERROR;
    }

    /*
     * Free Memory
     */
    netloc_dt_network_t_destruct(topology->network);

    if( NULL != topology->edges ) {
        hti = netloc_dt_lookup_table_iterator_t_construct(topology->edges);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            cur_edge = (netloc_edge_t*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == cur_edge ) {
                break;
            }
            netloc_dt_edge_t_destruct(cur_edge);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        netloc_lookup_table_destroy(topology->edges);
        free(topology->edges);
        topology->edges = NULL;
    }

    if( NULL != topology->nodes ) {
        for(i = 0; i < topology->num_nodes; ++i ) {
            if( NULL != topology->nodes[i] ) {
                netloc_dt_node_t_destruct(topology->nodes[i]);
                topology->nodes[i] = NULL;
            }
        }
        free(topology->nodes);
        topology->nodes = NULL;
    }

    free(topology);

    return NETLOC_SUCCESS;
}

int netloc_refresh(struct netloc_topology *topology)
{
    return NETLOC_ERROR_NOT_IMPL;
}
