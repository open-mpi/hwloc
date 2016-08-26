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
 * This program searches for a specific node in a specific network.
 */
#include "netloc.h"

int main(void) {
    char **search_uris = NULL;
    int num_uris = 1, ret;
    netloc_network_t *tmp_network = NULL;

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

    netloc_topology_t topology;

    // Attach to the network
    ret = netloc_attach(&topology, *tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }

    // Query the network topology
    // Find a specific node by its physical ID (GUID in the case of InfiniBand)
    netloc_node_t *node = NULL;
    char * phy_id = strdup("000b:8cff:ff00:4939");
    node = netloc_get_node_by_physical_id(topology, phy_id);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_get_node_by_physical_id returned %d\n", ret);
        return ret;
    }

    if( NULL == node ) {
        printf("Did not find a node with the physical ID \"%s\"\n", phy_id);
    } else {
        printf("Found: %s\n", netloc_node_pretty_print(node));
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
    free(phy_id);
    netloc_network_destruct(tmp_network);
    return NETLOC_SUCCESS;
}
