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

/*
 * 0 = off, 1 = on
 */
#define DEBUG 0

/*
 * Testing support functions
 */
int test_all_nodes(netloc_topology_t topology);
int test_all_switch_nodes(netloc_topology_t topology);
int test_all_host_nodes(netloc_topology_t topology);

int test_all_edges(netloc_topology_t topology);

int test_get_physical_path(netloc_topology_t topology);
int test_get_logical_path(netloc_topology_t topology);


int main(void) {
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_topology_t topology;
    netloc_network_t *tmp_network = NULL;
    char *search_uri = NULL;

    /*
     * Setup a Network connection
     */
    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    tmp_network->subnet_id    = strdup("fe80:0000:0000:0000");
    search_uri = strdup("file://data/netloc");

    ret = netloc_find_network(search_uri, tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
        return ret;
    }

    /*
     * Attach to the topology context
     */
    printf("Test attach: ");
    fflush(NULL);
    ret = netloc_attach(&topology, *tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }
    printf("Success\n");


    /*
     * Test all_switch_nodes
     */
    printf("Test get_all_switch_nodes: ");
    fflush(NULL);
    ret = test_all_switch_nodes(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");


    /*
     * Test all_host_nodes
     */
    printf("Test get_all_host_nodes: ");
    fflush(NULL);
    ret = test_all_host_nodes(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");


    /*
     * Test all_nodes
     */
    printf("Test get_all_nodes: ");
    fflush(NULL);
    ret = test_all_nodes(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");


    /*
     * Test all_edges_by_node
     */
    printf("Test get_all_edges: ");
    fflush(NULL);
    ret = test_all_edges(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");


    /*
     * Test get_physical_path
     */
    printf("Test get_physical_path: ");
    fflush(NULL);
    ret = test_get_physical_path(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");


    /*
     * Test get_logical_path
     */
    printf("Test get_logical_path: ");
    fflush(NULL);
    ret = test_get_logical_path(topology);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("Success\n");



 cleanup:
    /*
     * Cleanup
     */
    ret = netloc_detach(topology);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_detach returned an error (%d)\n", ret);
        return ret;
    }

    if( NULL != tmp_network) {
        netloc_network_destruct(tmp_network);
        tmp_network = NULL;
    }

    free(search_uri);
    return exit_status;
}


int test_all_nodes(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;

    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_nodes returned %d\n", ret);
        return ret;
    }

    /* Verify the data */
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
#if DEBUG == 1
        printf("Found: %s\n", netloc_node_pretty_print(node));
#endif
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    return NETLOC_SUCCESS;
}

int test_all_switch_nodes(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t switches = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;

    ret = netloc_get_all_switch_nodes(topology, &switches);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_switch_nodes returned %d\n", ret);
        return ret;
    }

    /* Verify the data */
    hti = netloc_dt_lookup_table_iterator_t_construct( switches );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(switches, key);
        if( NETLOC_NODE_TYPE_SWITCH != node->node_type ) {
            fprintf(stderr, "Error: Returned unexpected node: %s\n", netloc_node_pretty_print(node));
            return NETLOC_ERROR;
        }
#if DEBUG == 1
        printf("Found: %s\n", netloc_node_pretty_print(node));
#endif
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(switches);
    free(switches);

    return NETLOC_SUCCESS;
}

int test_all_host_nodes(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t hosts = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;

    ret = netloc_get_all_host_nodes(topology, &hosts);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_host_nodes returned %d\n", ret);
        return ret;
    }

    /* Verify the data */
    hti = netloc_dt_lookup_table_iterator_t_construct( hosts );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(hosts, key);
        if( NETLOC_NODE_TYPE_HOST != node->node_type ) {
            fprintf(stderr, "Error: Returned unexpected node: %s\n", netloc_node_pretty_print(node));
            return NETLOC_ERROR;
        }
#if DEBUG == 1
        printf("Found: %s\n", netloc_node_pretty_print(node));
#endif
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(hosts);
    free(hosts);

    return NETLOC_SUCCESS;
}

int test_all_edges(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;
    int num_edges;
    netloc_edge_t **edges = NULL;

    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_nodes returned %d\n", ret);
        return ret;
    }

    /*
     * For each node, get the edges from it
     */
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(nodes, key);

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
#if DEBUG == 1
        printf("Found: %d edges for host %s\n", num_edges, netloc_node_pretty_print(node));
        for(i = 0; i < num_edges; ++i ) {
            printf("\tEdge: %s\n", netloc_edge_pretty_print(edges[i]));
        }
#endif
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    return NETLOC_SUCCESS;
}

int test_get_physical_path(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti_src = NULL;
    netloc_dt_lookup_table_iterator_t hti_dest = NULL;
    const char * src_key = NULL;
    const char * dest_key = NULL;
    netloc_node_t *src_node = NULL;
    netloc_node_t *dest_node = NULL;
    int num_edges;
    netloc_edge_t **edges = NULL;

    ret = netloc_get_all_host_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_host_nodes returned %d\n", ret);
        return ret;
    }

    /*
     * For each node, get the physical path for the node
     */
    hti_src = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti_src) ) {
        src_key = netloc_lookup_table_iterator_next_key(hti_src);
        if( NULL == src_key ) {
            break;
        }

        src_node = (netloc_node_t*)netloc_lookup_table_access(nodes, src_key);

        /*
         * From this source to all destinations
         */
        hti_dest = netloc_dt_lookup_table_iterator_t_construct( nodes );
        while( !netloc_lookup_table_iterator_at_end(hti_dest) ) {
            dest_key = netloc_lookup_table_iterator_next_key(hti_dest);
            if( NULL == dest_key ) {
                break;
            }

            /*
             * Skip self reference
             */
            dest_node = (netloc_node_t*)netloc_lookup_table_access(nodes, dest_key);
            if( NETLOC_CMP_SAME == netloc_dt_node_t_compare(src_node, dest_node) ) {
                continue;
            }

            /*
             * Get the physical path, if any
             */
            ret = netloc_get_path(topology, src_node, dest_node, &num_edges, &edges, false);
            if( NETLOC_ERROR_NOT_FOUND == ret ) {
                fprintf(stderr, "Warning: No physical path information between these two nodes\n\tSrc  node %s\n\tDest node %s\n",
                        netloc_node_pretty_print(src_node),
                        netloc_node_pretty_print(dest_node));
                continue;
            }
            else if( NETLOC_SUCCESS != ret ) {
                fprintf(stderr, "Error: get_physical_path returned %d\nError: Src  node %s\nError: Dest node %s\n",
                        ret,
                        netloc_node_pretty_print(src_node),
                        netloc_node_pretty_print(dest_node));
                return ret;
            }

            /*
             * Verify the edges
             */
#if DEBUG == 1
            printf("Path Between: %d edges\n", num_edges);
            printf("\tSrc : %s\n", netloc_node_pretty_print(src_node));
            printf("\tDest: %s\n", netloc_node_pretty_print(dest_node));
            for(i = 0; i < num_edges; ++i ) {
                printf("\t\tEdge: %s\n", netloc_edge_pretty_print(edges[i]));
            }
#endif
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti_dest);
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti_src);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    return NETLOC_SUCCESS;
}

int test_get_logical_path(netloc_topology_t topology)
{
    int ret;
    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti_src = NULL;
    netloc_dt_lookup_table_iterator_t hti_dest = NULL;
    const char * src_key = NULL;
    const char * dest_key = NULL;
    netloc_node_t *src_node = NULL;
    netloc_node_t *dest_node = NULL;
    int num_edges;
    netloc_edge_t **edges = NULL;

    ret = netloc_get_all_host_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: get_all_host_nodes returned %d\n", ret);
        return ret;
    }

    /*
     * For each node, get the physical path for the node
     */
    hti_src = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti_src) ) {
        src_key = netloc_lookup_table_iterator_next_key(hti_src);
        if( NULL == src_key ) {
            break;
        }

        src_node = (netloc_node_t*)netloc_lookup_table_access(nodes, src_key);

        /*
         * From this source to all destinations
         */
        hti_dest = netloc_dt_lookup_table_iterator_t_construct( nodes );
        while( !netloc_lookup_table_iterator_at_end(hti_dest) ) {
            dest_key = netloc_lookup_table_iterator_next_key(hti_dest);
            if( NULL == dest_key ) {
                break;
            }

            /*
             * Skip self reference
             */
            dest_node = (netloc_node_t*)netloc_lookup_table_access(nodes, dest_key);
            if( NETLOC_CMP_SAME == netloc_dt_node_t_compare(src_node, dest_node) ) {
                continue;
            }

            /*
             * Get the logical path, if any
             */
            ret = netloc_get_path(topology, src_node, dest_node, &num_edges, &edges, true);
            if( NETLOC_ERROR_NOT_FOUND == ret ) {
                fprintf(stderr, "Warning: No path information between these two nodes\n\tSrc  node %s\n\tDest node %s\n",
                        netloc_node_pretty_print(src_node),
                        netloc_node_pretty_print(dest_node));
                continue;
            }
            else if( NETLOC_SUCCESS != ret ) {
                fprintf(stderr, "Error: get_logical_path returned %d\nError: Src  node %s\nError: Dest node %s\n",
                        ret,
                        netloc_node_pretty_print(src_node),
                        netloc_node_pretty_print(dest_node));
                return ret;
            }

            /*
             * Verify the edges
             */
#if DEBUG == 1
            printf("Path Between: %d edges\n", num_edges);
            printf("\tSrc : %s\n", netloc_node_pretty_print(src_node));
            printf("\tDest: %s\n", netloc_node_pretty_print(dest_node));
            for(i = 0; i < num_edges; ++i ) {
                printf("\t\tEdge: %s\n", netloc_edge_pretty_print(edges[i]));
            }
#endif
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti_dest);
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti_src);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    return NETLOC_SUCCESS;
}
