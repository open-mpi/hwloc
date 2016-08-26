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
 * This program prints all of the nodes in all of the network topologies discovered.
 */
#include "netloc.h"

int main(void) {
    int i, num_uris = 1;
    char **search_uris = NULL;
    int ret, exit_status = NETLOC_SUCCESS;
    int num_all_networks = 0;
    netloc_network_t **all_networks = NULL;

    netloc_topology_t topology;

    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    netloc_node_t *node = NULL;

    /*
     * Where to search for network topology information.
     * Information generated from a netloc reader.
     */
    search_uris = (char**)malloc(sizeof(char*) * num_uris );
    if( NULL == search_uris ) {
        return NETLOC_ERROR;
    }
    search_uris[0] = strdup("file://data/netloc");

    /*
     * Find all of the networks in the specified serach URI locations
     */
    ret = netloc_foreach_network((const char * const *) search_uris,
                                 num_uris,
                                 NULL, // Callback function (NULL = include all networks)
                                 NULL, // Callback function data
                                 &num_all_networks,
                                 &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_foreach_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * For each of those networks access the detailed topology
     */
    for(i = 0; i < num_all_networks; ++i ) {
        // Pretty print the network for debugging purposes
        printf("\tIncluded Network: %s\n", netloc_network_pretty_print(all_networks[i]) );

        /*
         * Attach to the network
         */
        ret = netloc_attach(&topology, *(all_networks[i]));
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
            return ret;
        }

        /*
         * Access all of the nodes in the topology
         */
        ret = netloc_get_all_nodes(topology, &nodes);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: get_all_nodes returned %d\n", ret);
            return ret;
        }

        // Display all of the nodes found
        hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            node = netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == node ) {
                break;
            }
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

        /*
         * Detach from the network
         */
        ret = netloc_detach(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: netloc_detach returned an error (%d)\n", ret);
            return ret;
        }
    }

    /*
     * Cleanup
     */
 cleanup:
    if( NULL != hti ) {
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;
    }
    if( NULL != nodes ) {
        netloc_lookup_table_destroy(nodes);
        free(nodes);
        nodes = NULL;
    }

    if( NULL != all_networks ) {
        for(i = 0; i < num_all_networks; ++i ) {
            netloc_network_destruct(all_networks[i]);
            all_networks[i] = NULL;
        }
        free(all_networks);
        all_networks = NULL;
    }

    if( NULL != search_uris ) {
        for(i = 0; i < num_uris; ++i) {
            free(search_uris[i]);
            search_uris[i] = NULL;
        }
        free(search_uris);
        search_uris = NULL;
    }

    return NETLOC_SUCCESS;
}
