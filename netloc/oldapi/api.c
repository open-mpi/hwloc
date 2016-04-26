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

/**
 * Find all of the nodes that match the netloc_node_type_t
 *
 * \param topology A valid pointer to a topology handle
 * \param nodes A lookup table of nodes that match
 * \param nt The netloc_node_type_t to compare against (NULL if anything can match)
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR otherwise
 */
static int find_matching_nodes(struct netloc_topology * topology, struct netloc_dt_lookup_table ** nodes, netloc_node_type_t *nt);


/*********************************************************************/

netloc_network_t* netloc_access_network_ref(struct netloc_topology * topology)
{
    if( NULL == topology ) {
        return NULL;
    }
    return topology->network;
}

int netloc_get_all_nodes(struct netloc_topology * topology, struct netloc_dt_lookup_table ** nodes)
{
    return find_matching_nodes(topology, nodes, NULL);
}

int netloc_get_all_switch_nodes(struct netloc_topology * topology, struct netloc_dt_lookup_table ** switches)
{
    netloc_node_type_t nt = NETLOC_NODE_TYPE_SWITCH;
    return find_matching_nodes(topology, switches, &nt);
}

int netloc_get_all_host_nodes(struct netloc_topology * topology, struct netloc_dt_lookup_table ** hosts)
{
    netloc_node_type_t nt = NETLOC_NODE_TYPE_HOST;
    return find_matching_nodes(topology, hosts, &nt);
}

netloc_node_t * netloc_get_node_by_physical_id(struct netloc_topology * topology, const char * phy_id)
{
    int i, ret;

    if( NULL == phy_id ) {
        return NULL;
    }

    /*
     * Lazy load the node information
     */
    if( !topology->nodes_loaded ) {
        ret = support_load_json(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to load the topology\n");
            return NULL;
        }
    }

    for(i = 0; i < topology->num_nodes; ++i) {
        if( 0 == strncmp(topology->nodes[i]->physical_id, phy_id, strlen(topology->nodes[i]->physical_id)) ) {
            return topology->nodes[i];
        }
    }

    return NULL;
}

int netloc_get_all_edges(struct netloc_topology * topology, netloc_node_t *node, int *num_edges, netloc_edge_t ***edges)
{
    int ret;

    /*
     * Lazy load the node information
     */
    if( !topology->nodes_loaded ) {
        ret = support_load_json(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to load the topology\n");
            return ret;
        }
    }

    // Note that we are just adjusting pointers, -not- copying data
    (*num_edges) = node->num_edges;
    (*edges)     = node->edges;

    return NETLOC_SUCCESS;
}

int netloc_get_path(const netloc_topology_t topology,
                    netloc_node_t *src_node,
                    netloc_node_t *dest_node,
                    int *num_edges,
                    netloc_edge_t ***path,
                    bool is_logical)
{
    int ret;

    /*
     * Lazy load the node information
     */
    if( !topology->nodes_loaded ) {
        ret = support_load_json(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to load the topology\n");
            return ret;
        }
    }

    (*num_edges) = 0;
    if( is_logical ) {
        (*path) = (netloc_edge_t**)netloc_lookup_table_access(src_node->logical_paths, dest_node->physical_id);
    } else {
        (*path) = (netloc_edge_t**)netloc_lookup_table_access(src_node->physical_paths, dest_node->physical_id);
    }
    if( NULL == (*path) ) {
        return NETLOC_ERROR_NOT_FOUND;
    }

    /* Count the edges */
    for((*num_edges) = 0; NULL != (*path)[(*num_edges)]; (*num_edges) += 1) {
        ;
    }

    return NETLOC_SUCCESS;
}


/*********************************************************************
 * Support Functions
 *********************************************************************/
static int find_matching_nodes(struct netloc_topology * topology, struct netloc_dt_lookup_table ** nodes, netloc_node_type_t *nt)
{
    int ret;
    int i;

    (*nodes) = NULL;

    /*
     * Lazy load the node information
     */
    if( !topology->nodes_loaded ) {
        ret = support_load_json(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to load the topology\n");
            return ret;
        }
    }

    /*
     * Search the nodes to find all of the nodes, and add them to the hash
     */
    for(i = 0; i < topology->num_nodes; ++i) {
        if( NULL == nt || (*nt) == topology->nodes[i]->node_type ) {
            if( NULL == (*nodes) ) {
	        (*nodes) = calloc(1, sizeof(**nodes));
                netloc_lookup_table_init((*nodes), 0, 0);
            }

            netloc_lookup_table_append( (*nodes), topology->nodes[i]->physical_id, topology->nodes[i]);
        }
    }

    return NETLOC_SUCCESS;
}
