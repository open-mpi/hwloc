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
 *
 * This program is meant to mirror the inline examples in netloc.doxy
 */
#include "netloc.h"

int main(void) {
    char **search_uris = NULL;
    int num_uris = 1, ret;
    netloc_network_t *tmp_network = NULL;

    netloc_topology_t topology;

    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;


    // Specify where to search for network data
    search_uris = (char**)malloc(sizeof(char*) * num_uris );
    search_uris[0] = strdup("file://data/netloc");

    // Find a specific InfiniBand network
    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    tmp_network->subnet_id    = strdup("fe80:0000:0000:0000");

    // Search for the specific network
    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: network not found!\n");
        netloc_network_destruct(tmp_network);
        return NETLOC_ERROR;
    }

    printf("\tFound Network: %s\n", netloc_network_pretty_print(tmp_network));

    // Attach to the network
    ret = netloc_attach(&topology, *tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }

    // Query the network topology

    // Access all of the nodes in the topology
    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_nodes returned %d\n", ret);
        return ret;
    }

    // Display all of the nodes found
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        // Access the data by key (could also access by entry in the example)
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(nodes, key);
        if( NETLOC_NODE_TYPE_INVALID == node->node_type ) {
            fprintf(stderr, "Error: Returned unexpected node: %s\n", netloc_node_pretty_print(node));
            return NETLOC_ERROR;
        }

        printf("Found: %s\n", netloc_node_pretty_print(node));
    }

    /* Cleanup the lookup table objects */
    if( NULL != hti ) {
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;
    }
    if( NULL != nodes ) {
        netloc_lookup_table_destroy(nodes);
        free(nodes);
        nodes = NULL;
    }

    // Detach from the network
    ret = netloc_detach(topology);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_detach returned an error (%d)\n", ret);
        return ret;
    }

    /*
     * Cleanup
     */
    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return NETLOC_SUCCESS;
}
