/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <netloc.h>
#include <netloc/dc.h>
#include <private/netloc.h>

#include <stdio.h>

#include "support.h"

/**
 * Encode an edge
 */
json_t* dc_encode_edge(const char * key, void *value);

/**
 * Display a netloc_node_t
 */
static void display_node(netloc_node_t *node, char * prefix);

/**
 * Display a netloc_edge_t
 */
static void display_edge(netloc_edge_t *edge, char * prefix);

/**
 * Display a path
 */
static void display_path(const char * src, const char * dest, int num_edges, netloc_edge_t **edges, char * prefix);

netloc_data_collection_handle_t * netloc_dt_data_collection_handle_t_construct()
{
    netloc_data_collection_handle_t *handle = NULL;

    handle = (netloc_data_collection_handle_t*)malloc(sizeof(netloc_data_collection_handle_t));
    if( NULL == handle ) {
        return NULL;
    }

    handle->network       = NULL;
    handle->is_open       = false;
    handle->is_read_only  = false;
    handle->unique_id_str = NULL;
    handle->data_uri      = NULL;
    handle->filename_nodes = NULL;
    handle->filename_physical_paths = NULL;
    handle->filename_logical_paths = NULL;

    handle->node_list = NULL;

    handle->edges     = NULL;

    handle->node_data = NULL;
    handle->node_data_acc = NULL;
    handle->path_data = NULL;
    handle->path_data_acc = NULL;

    return handle;
}

int netloc_dt_data_collection_handle_t_destruct(netloc_data_collection_handle_t *handle)
{
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *cur_node = NULL;
    netloc_edge_t *cur_edge = NULL;

    if( NULL != handle->network ) {
        netloc_dt_network_t_destruct(handle->network);
        handle->network      = NULL;
    }

    handle->is_open      = false;
    handle->is_read_only = false;

    free(handle->unique_id_str);
    free(handle->data_uri);
    free(handle->filename_nodes);
    free(handle->filename_physical_paths);
    free(handle->filename_logical_paths);

    if( NULL != handle->node_list ) {
        // Make sure to free all of the nodes pointed to in the lookup table
        hti = netloc_dt_lookup_table_iterator_t_construct(handle->node_list);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            cur_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == cur_node ) {
                break;
            }
            netloc_dt_node_t_destruct(cur_node);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);

        netloc_lookup_table_destroy(handle->node_list);
        free(handle->node_list);
    }

    if( NULL != handle->edges ) {
        // Make sure to free all of the edges pointed to in the lookup table
        hti = netloc_dt_lookup_table_iterator_t_construct(handle->edges);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            cur_edge = (netloc_edge_t*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == cur_edge ) {
                break;
            }
            netloc_dt_edge_t_destruct(cur_edge);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);

        netloc_lookup_table_destroy(handle->edges);
        free(handle->edges);
    }

    if( NULL != handle->node_data ) {
        json_decref(handle->node_data);
        // Implied decref of handle->node_data_acc
    }

    if( NULL != handle->path_data ) {
        json_decref(handle->path_data);
        // Implied decref of handle->path_data_acc
    }

    free( handle );

    return NETLOC_SUCCESS;
}


netloc_data_collection_handle_t * netloc_dc_create(netloc_network_t *network, char * dir)
{
    netloc_data_collection_handle_t *handle = NULL;

    /*
     * Sanity Checks
     */
    if( NULL == network ) {
        fprintf(stderr, "Error: Null network provided\n");
        return NULL;
    }

    if( NETLOC_NETWORK_TYPE_INVALID == network->network_type ) {
        fprintf(stderr, "Error: Invalid network type provided\n");
        return NULL;
    }

    if( NULL == network->subnet_id ) {
        fprintf(stderr, "Error: Null subnet provided\n");
        return NULL;
    }

    /*
     * Setup the handle to the data files
     */
    handle = netloc_dt_data_collection_handle_t_construct();

    handle->network       = netloc_dt_network_t_dup(network);

    asprintf(&handle->unique_id_str, "%s-%s",
             netloc_decode_network_type(network->network_type),
             network->subnet_id);

    /*
     * Setup the data filenames
     */
    if( NULL == dir ) {
        dir = strdup("");
    }
    asprintf(&handle->filename_nodes, "%s/%s-nodes.ndat", dir, handle->unique_id_str);
    asprintf(&handle->filename_physical_paths, "%s/%s-phy-paths.ndat", dir, handle->unique_id_str);
    asprintf(&handle->filename_logical_paths, "%s/%s-log-paths.ndat", dir, handle->unique_id_str);
    asprintf(&handle->data_uri, "file://%s", dir);

    handle->is_open       = true;
    handle->is_read_only  = false;

    /*
     * Set metadata and network data in the JSON files
     */
    handle->node_data = json_object();
    json_object_set_new(handle->node_data, JSON_NODE_FILE_NETWORK_INFO, netloc_dt_network_t_json_encode(handle->network));
    handle->node_data_acc = json_object();

    handle->path_data = json_object();
    json_object_set_new(handle->path_data, JSON_NODE_FILE_NETWORK_INFO, netloc_dt_network_t_json_encode(handle->network));
    handle->path_data_acc = json_object();

    handle->phy_path_data = json_object();
    json_object_set_new(handle->phy_path_data, JSON_NODE_FILE_NETWORK_INFO, netloc_dt_network_t_json_encode(handle->network));
    handle->phy_path_data_acc = json_object();

    return handle;
}

int netloc_dc_close(netloc_data_collection_handle_t *handle)
{
    int ret;

    /*
     * Sanity Checks
     */
    if( NULL == handle ) {
        fprintf(stderr, "Error: Null handle provided\n");
        return NETLOC_ERROR;
    }

    /*
     * If read only, then just close the fd
     */
    if( handle->is_read_only ) {
        handle->is_open = false;
        return NETLOC_SUCCESS;
    }


    /******************** Node and Edge Data **************************/

    json_object_set_new(handle->node_data, JSON_NODE_FILE_NODE_INFO, handle->node_data_acc);

    /*
     * Add the edge lookup table to the node data
     */
    json_object_set_new(handle->node_data, JSON_NODE_FILE_EDGE_INFO, netloc_dt_lookup_table_t_json_encode(handle->edges, &dc_encode_edge));
    //netloc_lookup_table_pretty_print(handle->edges);

    /*
     * If creating a new file, then write out the data (Node)
     */
    ret = json_dump_file(handle->node_data, handle->filename_nodes, JSON_COMPACT);
    if( 0 != ret ) {
        fprintf(stderr, "Error: Failed to write out node JSON file!\n");
        return NETLOC_ERROR;
    }

    json_decref(handle->node_data);
    handle->node_data = NULL;


    /******************** Physical Path Data **************************/
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *cur_node = NULL;

    /*
     * Add path entries to the JSON file (physical path)
     */
    hti = netloc_dt_lookup_table_iterator_t_construct(handle->node_list);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        cur_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == cur_node ) {
            break;
        }

        json_object_set_new(handle->phy_path_data_acc,
                            cur_node->physical_id,
                            netloc_dt_node_t_json_encode_paths(cur_node, cur_node->physical_paths));
    }
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    json_object_set_new(handle->phy_path_data, JSON_NODE_FILE_PATH_INFO, handle->phy_path_data_acc);

    /*
     * Write out path data
     */
    ret = json_dump_file(handle->phy_path_data, handle->filename_physical_paths, JSON_COMPACT);
    if( 0 != ret ) {
        fprintf(stderr, "Error: Failed to write out physical path JSON file!\n");
        return NETLOC_ERROR;
    }

    json_decref(handle->phy_path_data);
    handle->phy_path_data = NULL;


    /******************** Logical Path Data **************************/

    /*
     * Add path entries to the JSON file (logical path)
     */
    hti = netloc_dt_lookup_table_iterator_t_construct(handle->node_list);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        cur_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == cur_node ) {
            break;
        }

        json_object_set_new(handle->path_data_acc,
                            cur_node->physical_id,
                            netloc_dt_node_t_json_encode_paths(cur_node, cur_node->logical_paths));
    }
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    json_object_set_new(handle->path_data, JSON_NODE_FILE_PATH_INFO, handle->path_data_acc);

    /*
     * Write out path data
     */
    ret = json_dump_file(handle->path_data, handle->filename_logical_paths, JSON_COMPACT);
    if( 0 != ret ) {
        fprintf(stderr, "Error: Failed to write out logical path JSON file!\n");
        return NETLOC_ERROR;
    }

    json_decref(handle->path_data);
    handle->path_data = NULL;

    /*
     * Mark file as closed
     */
    handle->is_open = false;

    return NETLOC_SUCCESS;
}

netloc_network_t * netloc_dc_handle_get_network(netloc_data_collection_handle_t *handle)
{
    if( NULL == handle ) {
        fprintf(stderr, "Error: Null handle provided\n");
        return NULL;
    }

    return handle->network;
}

char * netloc_dc_handle_get_unique_id_str(netloc_data_collection_handle_t *handle)
{
    if( NULL == handle ) {
        fprintf(stderr, "Error: Null handle provided\n");
        return NULL;
    }

    return handle->unique_id_str;
}

char * netloc_dc_handle_get_unique_id_str_filename(char *filename)
{
    if( NULL == filename ) {
        fprintf(stderr, "Error: Null filename provided\n");
        return NULL;
    }

    // JJH TODO

    return NULL;
}

int netloc_dc_append_node(netloc_data_collection_handle_t *handle, netloc_node_t *node)
{
    char *key = NULL;
    netloc_node_t *cur_node = NULL;
    unsigned long key_int;

    /*
     * Setup the table for the first node
     */
    if(NULL == handle->node_list) {
        handle->node_list = calloc(1, sizeof(*handle->node_list));
        netloc_lookup_table_init(handle->node_list, 1, 0);
    }

    /*
     * Check to see if we have seen this node before
     */
    SUPPORT_CONVERT_ADDR_TO_INT(node->physical_id, handle->network->network_type, key_int);
    asprintf(&key, "%s", node->physical_id);
    cur_node = netloc_lookup_table_access_with_int(handle->node_list, key, key_int);

    if( NULL != cur_node ) {
        if( NETLOC_NODE_TYPE_INVALID == cur_node->node_type ) {
            // JJH: We should be able to use 'replace' instead of 'remove' and 'append'
            //      Need to double check ordering.
            netloc_lookup_table_remove_with_int(handle->node_list, key, key_int);
        } else {
            fprintf(stderr, "Warning: A version of this node has already been added to the data set!\n");
            fprintf(stderr, "Warning: Support for updating nodes is not yet available\n");
            // JJH: Delete the old cached version ? replace it?
            //json_object_del(handle->node_data_acc, node->physical_id);
        }
    }
    free(key);

    /*
     * Add the node to our list
     */
    if( NULL == cur_node ) {
        cur_node = netloc_dt_node_t_dup(node);

        cur_node->__uid__ = 0;

        cur_node->physical_id_int = key_int;
        asprintf(&key, "%s", node->physical_id);
        netloc_lookup_table_append_with_int(handle->node_list, key, key_int, cur_node);

        /*
         * Encode the data: Physical ID is the key
         */
        json_object_set_new(handle->node_data_acc, node->physical_id, netloc_dt_node_t_json_encode(node));
    }
    else if( NETLOC_NODE_TYPE_INVALID == cur_node->node_type ) {
        netloc_dt_node_t_copy(node, cur_node);

        cur_node->__uid__ = 0;

        cur_node->physical_id_int = key_int;
        asprintf(&key, "%s", node->physical_id);
        netloc_lookup_table_append_with_int(handle->node_list, key, key_int, cur_node);

        /*
         * Encode the data: Physical ID is the key
         */
        json_object_set_new(handle->node_data_acc, node->physical_id, netloc_dt_node_t_json_encode(node));
    }
    else {
        /*
         * Replace the node?
         * The code below will fix the cache (node_data_acc), but the handle->node_list needs to also be updated.
         */
        //json_object_del(handle->node_data_acc, node->physical_id);
        //json_object_set_new(handle->node_data_acc, node->physical_id, netloc_dt_node_t_json_encode(node));
        return NETLOC_ERROR_NOT_IMPL;
    }

    return NETLOC_SUCCESS;
}

int netloc_dc_append_edge_to_node(netloc_data_collection_handle_t *handle, netloc_node_t *node, netloc_edge_t *edge)
{
    char *key = NULL;
    netloc_edge_t *found_edge = NULL;
    netloc_node_t *found_node = NULL;
    unsigned long key_int;
    bool is_cached = false;

    /*
     * Setup the table for the first edge
     */
    if( NULL == handle->edges ) {
        handle->edges = calloc(1, sizeof(*handle->edges));
        netloc_lookup_table_init(handle->edges, 1, 0);
    }

    /*
     * Setup the table for the first node
     */
    if(NULL == handle->node_list) {
        handle->node_list = calloc(1, sizeof(*handle->node_list));
        netloc_lookup_table_init(handle->node_list, 1, 0);
    }

    /*
     * Check to see if we have seen this edge before
     */
    asprintf(&key, "%d", edge->edge_uid);
    found_edge = (netloc_edge_t*)netloc_lookup_table_access(handle->edges, key);
    free(key);
    // JJH: Should we be checking the contents of the edge, not just the key?

    /*
     * If not add it to the edges lookup table
     */
    if( NULL == found_edge ) {
        found_edge = netloc_dt_edge_t_dup(edge);

        asprintf(&key, "%d", found_edge->edge_uid);
        netloc_lookup_table_append(handle->edges, key, found_edge);
        free(key);
    }

    /*
     * Update the edge links
     * if the node endpoint is not in the list, then add a stub
     */
    if( NULL == edge->src_node_id ) {
        return NETLOC_ERROR_NOT_FOUND;
    }
    SUPPORT_CONVERT_ADDR_TO_INT(edge->src_node_id, handle->network->network_type, key_int);
    asprintf(&key, "%s", edge->src_node_id);
    found_node = netloc_lookup_table_access_with_int(handle->node_list, key, key_int);

    if( NULL == found_node ) {
        found_node = netloc_dt_node_t_construct();

        found_node->physical_id = strdup(edge->src_node_id);
        found_node->physical_id_int = key_int;
        found_node->node_type = NETLOC_NODE_TYPE_INVALID;

        netloc_lookup_table_append_with_int(handle->node_list, key, key_int, found_node);
    } else {
        is_cached = true;
    }
    found_edge->src_node = found_node;
    free(key);

    if( NULL == edge->dest_node_id ) {
        return NETLOC_ERROR_NOT_FOUND;
    }
    asprintf(&key, "%s", edge->dest_node_id);
    SUPPORT_CONVERT_ADDR_TO_INT(edge->dest_node_id, handle->network->network_type, key_int);
    found_node = netloc_lookup_table_access_with_int(handle->node_list, key, key_int);

    if( NULL == found_node ) {
        found_node = netloc_dt_node_t_construct();

        found_node->physical_id = strdup(edge->dest_node_id);
        found_node->physical_id_int = key_int;
        found_node->node_type = NETLOC_NODE_TYPE_INVALID;

        netloc_lookup_table_append_with_int(handle->node_list, key, key_int, found_node);
    }
    found_edge->dest_node = found_node;
    free(key);

    /*
     * Add the edge index to the node passed to us
     */
    node->num_edge_ids++;
    node->edge_ids = (int*)realloc(node->edge_ids, sizeof(int) * node->num_edge_ids);
    node->edge_ids[node->num_edge_ids -1] = found_edge->edge_uid;

    node->num_edges++;
    node->edges = (netloc_edge_t**)realloc(node->edges, sizeof(netloc_edge_t*) * node->num_edges);
    node->edges[node->num_edges -1] = found_edge;

    /*
     * Update the cached version of this node
     */
    if( is_cached ) {
        json_object_del(handle->node_data_acc, node->physical_id);
        json_object_set_new(handle->node_data_acc, node->physical_id, netloc_dt_node_t_json_encode(node));
    }

    return NETLOC_SUCCESS;
}

int netloc_dc_append_path(netloc_data_collection_handle_t *handle,
                          const char * src_node_id,
                          const char * dest_node_id,
                          int num_edges, netloc_edge_t **edges,
                          bool is_logical)
{
    int i;
    netloc_node_t *node = NULL;
    int *edge_ids = NULL;
    unsigned long key_int;

    /*
     * Find the source Node
     */
    SUPPORT_CONVERT_ADDR_TO_INT(src_node_id, handle->network->network_type, key_int);
    node = netloc_lookup_table_access_with_int( handle->node_list, src_node_id, key_int );

    if( NULL == node ) {
        fprintf(stderr, "Error: node not found in the list (id = %s)\n", src_node_id);
        return NETLOC_ERROR;
    }

    /*
     * Copy the edge id's
     * Note: The 'path' storage is not a set of edge pointers, but edge ids.
     * It is assumed that the user will push paths into the data collection, but
     * not have to retrieve that path from the dc_handle.
     */
    edge_ids = (int*)malloc(sizeof(int)* (num_edges+1));
    if( NULL == edge_ids ) {
        return NETLOC_ERROR;
    }

    for(i = 0; i < num_edges; ++i) {
        if( NULL == edges[i] ) {
            edge_ids[i] = NETLOC_EDGE_UID_INVALID;
        } else {
            edge_ids[i] = edges[i]->edge_uid;
        }
    }
    // Null terminated array
    edge_ids[num_edges] = NETLOC_EDGE_UID_NULL;


    /*
     * Add the entry to the hash
     */
    if( is_logical ) {
        if( NULL == node->logical_paths ) {
	    node->logical_paths = calloc(1, sizeof(*node->logical_paths));
            netloc_lookup_table_init(node->logical_paths, 1, 0);
            node->num_log_paths = 0;
        }
        node->num_log_paths += 1;
        netloc_lookup_table_append( node->logical_paths, dest_node_id, edge_ids);
    } else {
        if( NULL == node->physical_paths ) {
	    node->physical_paths = calloc(1, sizeof(*node->physical_paths));
            netloc_lookup_table_init(node->physical_paths, 1, 0);
            node->num_log_paths = 0;
        }
        node->num_log_paths += 1;
        netloc_lookup_table_append( node->physical_paths, dest_node_id, edge_ids);
    }


    return NETLOC_SUCCESS;
}

int netloc_dc_append_edge_to_node_by_id(netloc_data_collection_handle_t *handle, char * phy_id, netloc_edge_t *edge)
{
    netloc_node_t *node = NULL;

    node = netloc_dc_get_node_by_physical_id(handle, phy_id);
    if( NULL == node ) {
        return NETLOC_ERROR_NOT_FOUND;
    }

    return netloc_dc_append_edge_to_node(handle, node, edge);
}


netloc_node_t * netloc_dc_get_node_by_physical_id(netloc_data_collection_handle_t *handle, char * phy_id)
{
    unsigned long key_int;

    if( NULL == phy_id ) {
        return NULL;
    }

    SUPPORT_CONVERT_ADDR_TO_INT(phy_id, handle->network->network_type, key_int);
    return netloc_lookup_table_access_with_int( handle->node_list, phy_id, key_int );
}


void netloc_dc_pretty_print(netloc_data_collection_handle_t *handle)
{
    int p;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    const char * key = NULL;
    netloc_edge_t **path = NULL;
    int path_len;
    struct netloc_dt_lookup_table_iterator *htin = NULL;
    netloc_node_t *cur_node = NULL;

    htin = netloc_dt_lookup_table_iterator_t_construct(handle->node_list);
    while( !netloc_lookup_table_iterator_at_end(htin) ) {
        cur_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(htin);
        if( NULL == cur_node ) {
            break;
        }

        display_node(cur_node, "");

        printf("Physical Paths\n");
        printf("--------------\n");
        // Display all of the paths from this node to other nodes (if any)
        hti = netloc_dt_lookup_table_iterator_t_construct(cur_node->physical_paths);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            key = netloc_lookup_table_iterator_next_key(hti);
            if( NULL == key ) {
                break;
            }
            path = (netloc_edge_t**)netloc_lookup_table_access(cur_node->physical_paths, key);
            path_len = 0;
            for(p = 0; NULL != path[p]; ++p) {
                ++path_len;
            }
            display_path(cur_node->physical_id,
                         key, path_len, path, "\t");
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;

        printf("Logical Paths\n");
        printf("--------------\n");
        // Display all of the paths from this node to other nodes (if any)
        hti = netloc_dt_lookup_table_iterator_t_construct(cur_node->logical_paths);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            key = netloc_lookup_table_iterator_next_key(hti);
            if( NULL == key ) {
                break;
            }
            path = (netloc_edge_t**)netloc_lookup_table_access(cur_node->logical_paths, key);
            path_len = 0;
            for(p = 0; NULL != path[p]; ++p) {
                ++path_len;
            }
            display_path(cur_node->physical_id,
                         key, path_len, path, "\t");
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;
    }
    netloc_dt_lookup_table_iterator_t_destruct(htin);
}

/*************************************************************
 * Support Functionality
 *************************************************************/
static void display_node(netloc_node_t *node, char * prefix)
{
    int i;

    printf("%s%s\n",
           prefix,
           netloc_pretty_print_node_t(node) );

    for(i = 0; i < node->num_edges; ++i ) {
        display_edge(node->edges[i], "\t");
    }
}

static void display_edge(netloc_edge_t *edge, char * prefix)
{
    printf("%s%s\n",
           prefix,
           netloc_pretty_print_edge_t(edge));
}

static void display_path(const char * src, const char * dest, int num_edges, netloc_edge_t **edges, char * prefix)
{
    int i;

    printf("%sPath (%s) \t (%s)\n",
           prefix,
           src, dest);

    for(i = 0; i < num_edges; ++i) {
        if(i == 0 ) {
            printf("\t\t[%s] <--> [%s]",
                   edges[i]->src_node_id, edges[i]->dest_node_id);
        } else {
            printf(" <--> [%s]",
                   edges[i]->dest_node_id);
        }
    }
    printf("\n");
}

json_t* dc_encode_edge(const char * key, void *value)
{
    return netloc_dt_edge_t_json_encode((netloc_edge_t*)value);
}
