/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include "netloc.h"

#include <stdlib.h>

int main(void) {
    int ret, exit_status = NETLOC_SUCCESS;

    int i;
    netloc_topology_t topology;
    netloc_network_t *network = NULL;
    char *search_uri = NULL;

    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;

    int num_edges;
    netloc_edge_t **edges = NULL;


    /*
     * Setup a Network connection
     */
    network = netloc_network_construct();
    network->network_type = NETLOC_NETWORK_TYPE_ETHERNET;
    network->subnet_id    = strdup("unknown");
    search_uri = strdup("file://data/netloc");

    ret = netloc_find_network(search_uri, network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
        return ret;
    }

    /*
     * Attach to the topology context
     */
    printf("Test attach: ");
    fflush(NULL);
    ret = netloc_attach(&topology, *network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }
    printf("Success\n");


    /*
     * Get all the verticies
     */
    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_nodes returned %d\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Display the data
     */
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(nodes, key);
        if( NETLOC_NODE_TYPE_INVALID == node->node_type ) {
            fprintf(stderr, "Error: Returned unexpected node: %s\n", netloc_node_pretty_print(node));
            return NETLOC_ERROR;
        }

        /*
         * Get all of the edges
         */
        ret = netloc_get_all_edges(topology, node, &num_edges, &edges);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: get_all_edges_by_id returned %d for node %s\n", ret, netloc_node_pretty_print(node));
            return ret;
        }

        /*
         * Verify the edges
         */
        printf("Found: %d edges for host %s\n", num_edges, netloc_node_pretty_print(node));
        for(i = 0; i < num_edges; ++i ) {
            printf("\tEdge: %s\n", netloc_edge_pretty_print(edges[i]));
        }
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

 cleanup:
    /*
     * Cleanup
     */
    ret = netloc_detach(topology);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_detach returned an error (%d)\n", ret);
        return ret;
    }

    free(search_uri);
    return exit_status;
}
