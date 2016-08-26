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
    netloc_node_t *node = NULL;

    int num_edges;
    netloc_edge_t **edges = NULL;

    netloc_node_t *src_node = NULL;

    char * phy_id = NULL;
    char * tmp_str = NULL;

    phy_id = strdup("0002:c903:0002:a0eb");

    /*
     * Setup a Network connection
     */
    network = netloc_network_construct();
    network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    network->subnet_id    = strdup("fe80:0000:0000:0000");
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
    netloc_network_destruct(network);
    network = NULL;
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
     * Get all of the edges
     */
    src_node = netloc_get_node_by_physical_id(topology, phy_id);
    if( NULL == src_node ) {
        fprintf(stderr, "Error: Failed to in the node associated with the physical id %s\n", phy_id);
        return NETLOC_ERROR;
    }

    ret = netloc_get_all_edges(topology, src_node, &num_edges, &edges);
    if( NETLOC_SUCCESS != ret ) {
        tmp_str = netloc_node_pretty_print(node);
        fprintf(stderr, "Error: get_all_edges_by_id returned %d for node %s\n", ret, tmp_str);
        free(tmp_str);
        return ret;
    }

    /*
     * Display the data
     */
    tmp_str = netloc_node_pretty_print(src_node);
    printf("Neighbors of %s\n", tmp_str);
    free(tmp_str);
    for(i = 0; i < num_edges; ++i ) {
        node = netloc_get_node_by_physical_id(topology, edges[i]->dest_node_id);
        printf("%s ( from port %3s to port %3s, type: %6s) has %3d directed edge(s)\n",
               node->physical_id,
               edges[i]->src_port_id,
               edges[i]->dest_port_id,
               (NETLOC_NODE_TYPE_SWITCH == node->node_type ? "switch" : "host"),
               node->num_edges);
    }

    /* Cleanup */
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

    free(phy_id);
    free(search_uri);
    return exit_status;
}
