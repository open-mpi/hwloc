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

#include <netloc.h>
#include <private/netloc.h>

#define _GNU_SOURCE
#include <stdio.h>
#include "support.h"


#define STRDUP_IF_NOT_NULL(str) (NULL == str ? NULL : strdup(str))
#define STR_EMPTY_IF_NULL(str) (NULL == str ? "" : str)

#define ASSIGN_NULL_IF_EMPTY(lhs, obj, key) {                           \
    const char * str_val = json_string_value( json_object_get( obj, key) ); \
    if( NULL != str_val && strlen(str_val) > 0 ) {                      \
        lhs    = strdup( str_val );                                     \
    } else {                                                            \
        lhs    = NULL;                                                  \
    }                                                                   \
}

/*******************************************************************/

char * netloc_pretty_print_network_t(netloc_network_t* network)
{
    char * str     = NULL;
    const char * tmp_str = NULL;

    tmp_str = netloc_decode_network_type(network->network_type);

    if( NULL != network->version ) {
        asprintf(&str, "%s-%s (version %s)", tmp_str, network->subnet_id, network->version);
    } else {
        asprintf(&str, "%s-%s", tmp_str, network->subnet_id);
    }

    return str;
}

netloc_network_t * netloc_dt_network_t_construct( )
{
    netloc_network_t *network = NULL;

    network = (netloc_network_t*)malloc(sizeof(netloc_network_t));
    if( NULL == network ) {
        return NULL;
    }

    network->network_type = NETLOC_NETWORK_TYPE_INVALID;
    network->subnet_id    = NULL;
    network->data_uri     = NULL;
    network->node_uri     = NULL;
    network->phy_path_uri = NULL;
    network->path_uri     = NULL;
    network->description  = NULL;
    network->version      = NULL;
    network->userdata     = NULL;

    return network;
}

netloc_network_t * netloc_dt_network_t_dup(netloc_network_t *orig)
{
    netloc_network_t *network = NULL;

    if( NULL == orig ) {
        return NULL;
    }

    network = netloc_dt_network_t_construct();
    if( NULL == network ) {
        return NULL;
    }

    netloc_dt_network_t_copy(orig, network);

    return network;
}

int netloc_dt_network_t_copy(netloc_network_t *from, netloc_network_t *to)
{

    if( NULL == to || NULL == from ) {
        return NETLOC_ERROR;
    }

    to->network_type = from->network_type;

    if( NULL != to->subnet_id ) {
        free(to->subnet_id);
    }
    to->subnet_id    = STRDUP_IF_NOT_NULL(from->subnet_id);

    if( NULL != to->data_uri ) {
        free(to->data_uri);
    }
    to->data_uri     = STRDUP_IF_NOT_NULL(from->data_uri);

    if( NULL != to->node_uri ) {
        free(to->node_uri);
    }
    to->node_uri     = STRDUP_IF_NOT_NULL(from->node_uri);

    if( NULL != to->path_uri ) {
        free(to->path_uri);
    }
    to->path_uri     = STRDUP_IF_NOT_NULL(from->path_uri);

    if( NULL != to->phy_path_uri ) {
        free(to->phy_path_uri);
    }
    to->phy_path_uri     = STRDUP_IF_NOT_NULL(from->phy_path_uri);

    if( NULL != to->description ) {
        free(to->description);
    }
    to->description  = STRDUP_IF_NOT_NULL(from->description);

    if( NULL != to->version ) {
        free(to->version);
    }
    to->version  = STRDUP_IF_NOT_NULL(from->version);

    to->userdata     = from->userdata;

    return NETLOC_SUCCESS;
}

json_t* netloc_dt_network_t_json_encode(netloc_network_t *network)
{
    json_t *json_nw = NULL;

    json_nw = json_object();

    json_object_set_new(json_nw, JSON_NODE_FILE_NETWORK_TYPE, json_integer(network->network_type));
    json_object_set_new(json_nw, JSON_NODE_FILE_SUBNET_ID,    json_string( STR_EMPTY_IF_NULL(network->subnet_id) ));

    json_object_set_new(json_nw, JSON_NODE_FILE_DESCRIPTION,  json_string( STR_EMPTY_IF_NULL(network->description) ));
    json_object_set_new(json_nw, JSON_NODE_FILE_META_VERSION, json_string( STR_EMPTY_IF_NULL(network->version) ));

    return json_nw;
}

netloc_network_t * netloc_dt_network_t_json_decode(json_t *json_nw)
{
    netloc_network_t *network = NULL;

    network = netloc_dt_network_t_construct();
    if( NULL == network ) {
        return NULL;
    }

    network->network_type = (netloc_network_type_t)json_integer_value( json_object_get( json_nw, JSON_NODE_FILE_NETWORK_TYPE) );

    ASSIGN_NULL_IF_EMPTY( network->subnet_id,   json_nw, JSON_NODE_FILE_SUBNET_ID );
    ASSIGN_NULL_IF_EMPTY( network->description, json_nw, JSON_NODE_FILE_DESCRIPTION );
    ASSIGN_NULL_IF_EMPTY( network->version,     json_nw, JSON_NODE_FILE_META_VERSION );

    return network;
}

int netloc_dt_network_t_destruct(netloc_network_t * network)
{
    if( NULL == network )
        return NETLOC_SUCCESS;

    if( NULL != network->subnet_id ) {
        free(network->subnet_id);
        network->subnet_id = NULL;
    }

    if( NULL != network->data_uri ) {
        free(network->data_uri);
        network->data_uri = NULL;
    }

    if( NULL != network->node_uri ) {
        free(network->node_uri);
        network->node_uri = NULL;
    }

    if( NULL != network->path_uri ) {
        free(network->path_uri);
        network->path_uri = NULL;
    }

    if( NULL != network->phy_path_uri ) {
        free(network->phy_path_uri);
        network->phy_path_uri = NULL;
    }

    if( NULL != network->description ) {
        free(network->description);
        network->description = NULL;
    }

    if( NULL != network->version ) {
        free(network->version);
        network->version = NULL;
    }

    if( NULL != network->userdata ) {
        network->userdata = NULL;
    }

    free(network);
    network = NULL;

    return NETLOC_SUCCESS;
}


int netloc_dt_network_t_compare(netloc_network_t *a, netloc_network_t *b)
{
    /* Check: Network Type */
    if(a->network_type != b->network_type) {
        return NETLOC_CMP_DIFF;
    }

    /* Check: Subnet ID */
    if( (NULL == a->subnet_id && NULL != b->subnet_id) ||
        (NULL != a->subnet_id && NULL == b->subnet_id) ) {
        return NETLOC_CMP_DIFF;
    }
    if( NULL != a->subnet_id && NULL != b->subnet_id ) {
        if( 0 != strncmp(a->subnet_id, b->subnet_id, strlen(a->subnet_id)) ) {
            return NETLOC_CMP_DIFF;
        }
    }

    /* Check: Metadata */
    if( (NULL == a->version && NULL != b->version) ||
        (NULL != a->version && NULL == b->version) ) {
        return NETLOC_CMP_SIMILAR;
    }
    if( NULL != a->version && NULL != b->version ) {
        if( 0 != strncmp(a->version, b->version, strlen(a->version)) ) {
            return NETLOC_CMP_SIMILAR;
        }
    }

    return NETLOC_CMP_SAME;
}


/*******************************************************************/


netloc_edge_t * netloc_dt_edge_t_construct()
{
    static int cur_uid = NETLOC_EDGE_UID_START;

    netloc_edge_t *edge = NULL;

    edge = (netloc_edge_t*)malloc(sizeof(netloc_edge_t));
    if( NULL == edge ) {
        return NULL;
    }

    edge->edge_uid = cur_uid;
    cur_uid++;

    edge->src_node       = NULL;
    edge->src_node_id    = NULL;
    edge->src_node_type  = NETLOC_NODE_TYPE_INVALID;
    edge->src_port_id    = NULL;

    edge->dest_node      = NULL;
    edge->dest_node_id   = NULL;
    edge->dest_node_type = NETLOC_NODE_TYPE_INVALID;
    edge->dest_port_id   = NULL;

    edge->speed = NULL;
    edge->width = NULL;
    edge->gbits = 0;

    edge->description = NULL;

    edge->num_real_edges = 0;
    edge->real_edges = NULL;

    edge->virtual_edge = NULL;

    edge->partitions = NULL;
    edge->num_partitions = 0;

    edge->userdata = NULL;

    return edge;
}

char * netloc_pretty_print_edge_t(netloc_edge_t* edge)
{
    char * str = NULL;
    const char * tmp_src_str = NULL;
    const char * tmp_dest_str = NULL;

    tmp_src_str = netloc_decode_node_type(edge->src_node_type);
    tmp_dest_str = netloc_decode_node_type(edge->dest_node_type);

    asprintf(&str, "%3d (%s) [%23s] %2s [<- %s / %s (%f) ->] (%s) [%23s] %2s",
             edge->edge_uid,
             tmp_src_str,
             edge->src_node_id,
             edge->src_port_id,
             edge->speed,
             edge->width,
             edge->gbits,
             tmp_dest_str,
             edge->dest_node_id,
             edge->dest_port_id);

    return str;
}

netloc_edge_t * netloc_dt_edge_t_dup(netloc_edge_t *orig)
{
    netloc_edge_t * edge = NULL;

    if( NULL == orig ) {
        return NULL;
    }

    edge = netloc_dt_edge_t_construct();
    if( NULL == edge ) {
        return NULL;
    }

    netloc_dt_edge_t_copy(orig, edge);

    return edge;
}

int netloc_dt_edge_t_copy(netloc_edge_t *from, netloc_edge_t *to)
{
    if( NULL == to || NULL == from ) {
        return NETLOC_ERROR;
    }

    /* Note: Does not copy the edge UID */

    to->src_node  = from->src_node;

    if( NULL != to->src_node_id ) {
        free(to->src_node_id);
    }
    to->src_node_id    = STRDUP_IF_NOT_NULL(from->src_node_id);

    to->src_node_type  = from->src_node_type;

    if( NULL != to->src_port_id ) {
        free(to->src_port_id);
    }
    to->src_port_id    = STRDUP_IF_NOT_NULL(from->src_port_id);


    to->dest_node  = from->dest_node;

    if( NULL != to->dest_node_id ) {
        free(to->dest_node_id);
    }
    to->dest_node_id   = STRDUP_IF_NOT_NULL(from->dest_node_id);

    to->dest_node_type = from->dest_node_type;

    if( NULL != to->dest_port_id ) {
        free(to->dest_port_id );
    }
    to->dest_port_id   = STRDUP_IF_NOT_NULL(from->dest_port_id);


    if( NULL != to->speed ) {
        free(to->speed);
    }
    to->speed = STRDUP_IF_NOT_NULL(from->speed);

    if( NULL != to->width ) {
        free(to->width);
    }
    to->width = STRDUP_IF_NOT_NULL(from->width);

    to->gbits = from->gbits;

    if( NULL != to->description ) {
        free(to->description);
    }
    to->description = STRDUP_IF_NOT_NULL(from->description);

    to->userdata = from->userdata;

    return NETLOC_SUCCESS;
}

json_t* netloc_dt_edge_t_json_encode(netloc_edge_t *edge)
{
    json_t *json_edge = NULL;

    json_edge = json_object();

    json_object_set_new(json_edge, JSON_NODE_FILE_EDGE_UID,  json_integer(edge->edge_uid));

    json_object_set_new(json_edge, JSON_NODE_FILE_SRC_ID,    json_string( STR_EMPTY_IF_NULL(edge->src_node_id)));
    json_object_set_new(json_edge, JSON_NODE_FILE_SRC_TYPE,  json_integer(edge->src_node_type));
    json_object_set_new(json_edge, JSON_NODE_FILE_SRC_PORT,  json_string( STR_EMPTY_IF_NULL(edge->src_port_id)));

    json_object_set_new(json_edge, JSON_NODE_FILE_DEST_ID,   json_string( STR_EMPTY_IF_NULL(edge->dest_node_id)));
    json_object_set_new(json_edge, JSON_NODE_FILE_DEST_TYPE, json_integer(edge->dest_node_type));
    json_object_set_new(json_edge, JSON_NODE_FILE_DEST_PORT, json_string( STR_EMPTY_IF_NULL(edge->dest_port_id)));

    json_object_set_new(json_edge, JSON_NODE_FILE_EDGE_SPEED, json_string( STR_EMPTY_IF_NULL(edge->speed)));
    json_object_set_new(json_edge, JSON_NODE_FILE_EDGE_WIDTH, json_string( STR_EMPTY_IF_NULL(edge->width)));
    json_object_set_new(json_edge, JSON_NODE_FILE_EDGE_GBITS, json_real(edge->gbits));

    json_object_set_new(json_edge, JSON_NODE_FILE_DESCRIPTION,  json_string( STR_EMPTY_IF_NULL(edge->description) ));

    return json_edge;
}

netloc_edge_t* netloc_dt_edge_t_json_decode(json_t *json_edge)
{
    netloc_edge_t *edge = NULL;

    edge = netloc_dt_edge_t_construct();
    if( NULL == edge ) {
        return NULL;
    }

    edge->edge_uid = json_integer_value( json_object_get( json_edge, JSON_NODE_FILE_EDGE_UID));

    ASSIGN_NULL_IF_EMPTY( edge->src_node_id, json_edge, JSON_NODE_FILE_SRC_ID);
    edge->src_node_type = (netloc_node_type_t)json_integer_value( json_object_get( json_edge, JSON_NODE_FILE_SRC_TYPE));
    ASSIGN_NULL_IF_EMPTY( edge->src_port_id, json_edge, JSON_NODE_FILE_SRC_PORT);

    ASSIGN_NULL_IF_EMPTY( edge->dest_node_id, json_edge, JSON_NODE_FILE_DEST_ID);
    edge->dest_node_type = (netloc_node_type_t)json_integer_value( json_object_get( json_edge, JSON_NODE_FILE_DEST_TYPE));
    ASSIGN_NULL_IF_EMPTY( edge->dest_port_id, json_edge, JSON_NODE_FILE_DEST_PORT);

    ASSIGN_NULL_IF_EMPTY( edge->speed, json_edge, JSON_NODE_FILE_EDGE_SPEED);
    ASSIGN_NULL_IF_EMPTY( edge->width, json_edge, JSON_NODE_FILE_EDGE_WIDTH);
    edge->gbits = json_real_value( json_object_get( json_edge, JSON_NODE_FILE_EDGE_GBITS));

    ASSIGN_NULL_IF_EMPTY( edge->description, json_edge, JSON_NODE_FILE_DESCRIPTION );

    return edge;
}

int netloc_dt_edge_t_destruct(netloc_edge_t * edge)
{
    if( NULL == edge ) {
        return NETLOC_SUCCESS;
    }

    // Do not free - just nullify the reference
    edge->src_node = NULL;

    if( NULL != edge->src_node_id ) {
        free(edge->src_node_id);
        edge->src_node_id = NULL;
    }

    if( NULL != edge->src_port_id ) {
        free(edge->src_port_id);
        edge->src_port_id = NULL;
    }

    // Do not free - just nullify the reference
    edge->dest_node = NULL;

    if( NULL != edge->dest_node_id ) {
        free(edge->dest_node_id);
        edge->dest_node_id = NULL;
    }

    if( NULL != edge->dest_port_id ) {
        free(edge->dest_port_id);
        edge->dest_port_id = NULL;
    }

    if( NULL != edge->speed ) {
        free(edge->speed);
        edge->speed = NULL;
    }

    if( NULL != edge->width ) {
        free(edge->width);
        edge->width = NULL;
    }

    if( NULL != edge->description ) {
        free(edge->description);
        edge->description = NULL;
    }

    if( NULL != edge->userdata ) {
        edge->userdata = NULL;
    }

    free(edge);
    edge = NULL;
    return NETLOC_SUCCESS;
}

int netloc_dt_edge_t_compare(netloc_edge_t *a, netloc_edge_t *b)
{
    if( 0 != strncmp(a->src_node_id, b->src_node_id, strlen(a->src_node_id)) ) {
        return NETLOC_CMP_DIFF;
    }

    if( 0 != strncmp(a->src_port_id, b->src_port_id, strlen(a->src_port_id)) ) {
        return NETLOC_CMP_DIFF;
    }

    if( 0 != strncmp(a->dest_node_id, b->dest_node_id, strlen(a->dest_node_id)) ) {
        return NETLOC_CMP_DIFF;
    }

    if( 0 != strncmp(a->dest_port_id, b->dest_port_id, strlen(a->dest_port_id)) ) {
        return NETLOC_CMP_DIFF;
    }

    return NETLOC_CMP_SAME;
}


/*******************************************************************/


netloc_node_t * netloc_dt_node_t_construct()
{
    netloc_node_t *node = NULL;

    node = (netloc_node_t*)malloc(sizeof(netloc_node_t));
    if( NULL == node ) {
        return NULL;
    }

    node->network_type = NETLOC_NETWORK_TYPE_INVALID;
    node->node_type    = NETLOC_NODE_TYPE_INVALID;
    node->physical_id  = NULL;
    node->physical_id_int = 0;
    node->logical_id   = NULL;
    node->subnet_id    = NULL;
    node->description  = NULL;
    node->userdata     = NULL;
    node->num_edges    = 0;
    node->edges        = NULL;

    node->num_edge_ids = 0;
    node->edge_ids     = NULL;

    node->num_phy_paths = 0;
    node->physical_paths = calloc(1, sizeof(*node->physical_paths));

    node->num_log_paths = 0;
    node->logical_paths = calloc(1, sizeof(*node->logical_paths));

    node->num_real_nodes = 0;
    node->real_nodes = NULL;

    node->virtual_node = NULL;

    node->hostname = NULL;

    node->partitions = NULL;
    node->num_partitions = 0;

    node->topoIdx = -1;

    return node;
}

char * netloc_pretty_print_node_t(netloc_node_t* node)
{
    char * str = NULL;
    const char * tmp_nw_type = NULL;
    const char * tmp_node_type = NULL;

    tmp_nw_type = netloc_decode_network_type(node->network_type);
    tmp_node_type = netloc_decode_node_type(node->node_type);

    asprintf(&str, "(%s-\t %s) [%23s][%lu]/[%15s] %s -- %s (%d/%d edges)",
             tmp_nw_type,
             tmp_node_type,
             node->physical_id,
             node->physical_id_int,
             node->logical_id,
             node->subnet_id,
             node->description,
             node->num_edges,
             node->num_edge_ids);

    return str;
}

netloc_node_t * netloc_dt_node_t_dup(netloc_node_t *orig)
{
    netloc_node_t * node = NULL;

    if( NULL == orig ) {
        return NULL;
    }

    node = netloc_dt_node_t_construct();
    if( NULL == node ) {
        return NULL;
    }

    netloc_dt_node_t_copy(orig, node);

    return node;
}

int netloc_dt_node_t_copy(netloc_node_t *from, netloc_node_t *to)
{
    int i;

    to->network_type = from->network_type;
    to->node_type    = from->node_type;

    if( NULL != to->physical_id ) {
        free(to->physical_id );
    }
    to->physical_id  = STRDUP_IF_NOT_NULL(from->physical_id);

    to->physical_id_int = from->physical_id_int;

    if( NULL != to->logical_id ) {
        free(to->logical_id);
    }
    to->logical_id   = STRDUP_IF_NOT_NULL(from->logical_id);

    if( NULL != to->subnet_id ) {
        free(to->subnet_id);
    }
    to->subnet_id    = STRDUP_IF_NOT_NULL(from->subnet_id);

    if( NULL != to->description ) {
        free(to->description);
    }
    to->description  = STRDUP_IF_NOT_NULL(from->description);

    to->userdata     = from->userdata;


    if( NULL != to->edges ) {
        for(i = 0; i < to->num_edges; ++i) {
            // Nothing to deallocate here, since we just have pointers
            to->edges[i] = NULL;
        }
        free(to->edges);
    }
    to->num_edges = from->num_edges;
    to->edges = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*)*to->num_edges);
    for(i = 0; i < to->num_edges; ++i) {
        // Copy the pointer to the edge
        to->edges[i] = from->edges[i];
    }


    if( NULL != to->edge_ids ) {
        free(to->edge_ids);
    }
    to->num_edge_ids = from->num_edge_ids;
    to->edge_ids = (int*)malloc(sizeof(int) * to->num_edge_ids);
    for(i = 0; i < to->num_edge_ids; ++i) {
        to->edge_ids[i] = from->edge_ids[i];
    }


    if( NULL != to->physical_paths ) {
        netloc_lookup_table_destroy(to->physical_paths);
        free(to->physical_paths);
    }
    to->num_phy_paths = from->num_phy_paths;
    to->physical_paths = calloc(1, sizeof(*to->physical_paths));
    // Note: This does -not- do a deep copy of the keys
    netloc_dt_lookup_table_t_copy(from->physical_paths, to->physical_paths);


    if( NULL != to->logical_paths ) {
        netloc_lookup_table_destroy(to->logical_paths);
        free(to->logical_paths);
    }
    to->num_log_paths = from->num_log_paths;
    to->logical_paths = calloc(1, sizeof(*to->logical_paths));
    // Note: This does -not- do a deep copy of the keys
    netloc_dt_lookup_table_t_copy(from->logical_paths, to->logical_paths);

    return NETLOC_SUCCESS;
}

json_t* netloc_dt_node_t_json_encode(netloc_node_t *node)
{
    json_t *json_node = NULL;
    json_t *json_edges = NULL;
    int i;

    json_node = json_object();

    json_object_set_new(json_node, JSON_NODE_FILE_NETWORK_TYPE, json_integer(node->network_type));
    json_object_set_new(json_node, JSON_NODE_FILE_NODE_TYPE,    json_integer(node->node_type));

    json_object_set_new(json_node, JSON_NODE_FILE_PHY_ID,       json_string( STR_EMPTY_IF_NULL(node->physical_id) ));
    json_object_set_new(json_node, JSON_NODE_FILE_LOG_ID,       json_string( STR_EMPTY_IF_NULL(node->logical_id) ));
    // JJH: Subnet ID is duplication from network data...
    json_object_set_new(json_node, JSON_NODE_FILE_SUBNET_ID,    json_string( STR_EMPTY_IF_NULL(node->subnet_id) ));

    json_object_set_new(json_node, JSON_NODE_FILE_DESCRIPTION,  json_string( STR_EMPTY_IF_NULL(node->description) ));

    // Do not encode the edges, just the edge_id's
    // We will re-point the edges when we read back in the data set
    json_edges = json_array();
    for(i = 0; i < node->num_edge_ids; ++i) {
        json_array_append_new(json_edges, json_integer(node->edge_ids[i]));
    }
    json_object_set_new(json_node, JSON_NODE_FILE_EDGE_ID_LIST, json_edges);

    /** Do not encode the Logical Paths here. **/

    return json_node;
}

netloc_node_t* netloc_dt_node_t_json_decode(struct netloc_dt_lookup_table *edge_table, json_t *json_node)
{
    netloc_node_t *node = NULL;
    int i;
    json_t *edge_list = NULL;
    char * key = NULL;

    node = netloc_dt_node_t_construct();
    if( NULL == node ) {
        return NULL;
    }

    node->network_type = (netloc_network_type_t)json_integer_value( json_object_get( json_node, JSON_NODE_FILE_NETWORK_TYPE));
    node->node_type    = (netloc_node_type_t)json_integer_value( json_object_get( json_node, JSON_NODE_FILE_NODE_TYPE));

    ASSIGN_NULL_IF_EMPTY( node->physical_id, json_node, JSON_NODE_FILE_PHY_ID);
    SUPPORT_CONVERT_ADDR_TO_INT(node->physical_id, node->network_type, node->physical_id_int);

    ASSIGN_NULL_IF_EMPTY( node->logical_id, json_node, JSON_NODE_FILE_LOG_ID);
    ASSIGN_NULL_IF_EMPTY( node->subnet_id, json_node, JSON_NODE_FILE_SUBNET_ID);

    ASSIGN_NULL_IF_EMPTY( node->description, json_node, JSON_NODE_FILE_DESCRIPTION );

    /*
     * While reading in the edge_id list, create pointers to the data in the 'edges' array
     * These pointers are for ease of use. The user could lookup the data from the
     * lookup table each time they access an edge, but the direct pointer is better.
     * We just need to be a bit more careful.
     */
    edge_list = json_object_get(json_node, JSON_NODE_FILE_EDGE_ID_LIST);
    node->num_edge_ids = json_array_size(edge_list);
    node->edge_ids = (int*)malloc(sizeof(int) * node->num_edge_ids);
    if( NULL == node->edge_ids ) {
        netloc_dt_node_t_destruct(node);
        return NULL;
    }

    node->num_edges = node->num_edge_ids;
    node->edges = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*) * node->num_edges);
    if( NULL == node->edges ) {
        netloc_dt_node_t_destruct(node);
        return NULL;
    }

    for(i = 0; i < node->num_edge_ids; ++i) {
        node->edge_ids[i] = json_integer_value( json_array_get(edge_list, i));
        asprintf(&key, "%d", node->edge_ids[i]);
        node->edges[i] = (netloc_edge_t*)netloc_lookup_table_access(edge_table, key);
        if( NULL == node->edges[i] ) {
            printf("Error: Failed to find edge UID %d for the following node\n",node->edge_ids[i]);
            printf("Error: \t%s\n", netloc_pretty_print_node_t(node));
            netloc_dt_node_t_destruct(node);
            return NULL;
        }

        // Update the edge->src_node pointer to reference ourself
        // Note: We cannot fill in the dest_node since that node may not exist yet.
        //       We can only do that after all nodes have been read.
        node->edges[i]->src_node = node;

        free(key);
    }

    /** Do not decode the Logical Paths here. **/

    return node;
}

// NOTE: This assumes that the "paths" is a NULL terminated array of
//       edge_ids. Which it will be when called from the data
//       collector. Otherwise, it will be netloc_edge_t pointers.
json_t* netloc_dt_node_t_json_encode_paths(netloc_node_t *node, struct netloc_dt_lookup_table *paths)
{
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    const char * key = NULL;
    int *path = NULL;

    int i;

    json_t *json_all_paths = NULL;
    json_t *json_edges = NULL;

    json_all_paths = json_object();

    // All paths from this source
    hti = netloc_dt_lookup_table_iterator_t_construct(paths);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        // Path is a NULL terminated array of edge_ids to that destination.
        path = (int*)netloc_lookup_table_access(paths, key);
        json_edges = json_array();
        for(i = 0; NETLOC_EDGE_UID_NULL != path[i]; ++i) {
            json_array_append_new(json_edges, json_integer(path[i]));
        }

        // Add path to all paths
        // Key = Destination
        json_object_set_new(json_all_paths, key, json_edges);
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti);

    return json_all_paths;
}

struct netloc_dt_lookup_table * netloc_dt_node_t_json_decode_paths(struct netloc_dt_lookup_table *edge_table, json_t *json_all_paths)
{
    struct netloc_dt_lookup_table * ht = NULL;
    size_t size = 0;

    const char * key1 = NULL;
    json_t     * value1 = NULL;

    size_t i, j, num_edges = 0;
    netloc_edge_t **edges = NULL;
    int *edge_ids = NULL;

    char *edge_key = NULL;

    ht = calloc(1, sizeof(*ht));

    size = json_object_size(json_all_paths);

    netloc_lookup_table_init(ht, size, 0);

    if( 0 == size ) {
        return ht;
    }

    json_object_foreach(json_all_paths, key1, value1) {
        num_edges = json_array_size(value1);

        /*
         * Only edge_id's are stored in the JSON file, so we need a temporary
         * structure to hold those ids before we translate them to real edge
         * pointers.
         */
        edge_ids = (int*)malloc(sizeof(int) * (num_edges));
        if( NULL == edge_ids ) {
            netloc_lookup_table_destroy(ht);
            free(ht);
            return NULL;
        }

        for(i = 0; i < num_edges; ++i ) {
            edge_ids[i] = json_integer_value( json_array_get(value1, i) );
        }

        /*
         * Now that we have the edge ids, find the cooresponding pointer in the
         * edge_table
         */
        edges = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*) * (num_edges+1));
        if( NULL == edges ) {
            free(edge_ids);
            edge_ids = NULL;
            netloc_lookup_table_destroy(ht);
            free(ht);
            return NULL;
        }

        for(i = 0; i < num_edges; ++i ) {
            asprintf(&edge_key, "%d", edge_ids[i]);
            edges[i] = (netloc_edge_t*)netloc_lookup_table_access(edge_table, edge_key);
            if( NULL == edges[i] ) {
                printf("Error: Failed to find edge UID %d while decoding the path:\n", edge_ids[i]);
                printf("Error: \t");
                for(j = 0; j < num_edges; ++j) {
                    printf("%3d,", edge_ids[j]);
                }
                printf("\n");

                free(edge_ids);
                edge_ids = NULL;
                free(edges);
                edges = NULL;
                netloc_lookup_table_destroy(ht);
                free(ht);
                return NULL;
            }
            free(edge_key);
            edge_key = NULL;
        }
        // Null terminated array
        edges[num_edges] = NULL;

        /*
         * Add the array of edge pointers to the table with the
         * key = destination
         */
        netloc_lookup_table_append(ht, key1, edges);
        edges = NULL; // Do --not-- free the memory.

        if( NULL != edge_ids ) {
            free(edge_ids);
            edge_ids = NULL;
        }
    }

    if( NULL != edge_ids ) {
        free(edge_ids);
        edge_ids = NULL;
    }

    if( NULL != edge_key ) {
        free(edge_key);
        edge_key = NULL;
    }

    return ht;
}


int netloc_dt_node_t_destruct(netloc_node_t * node)
{
    int i;
    void **path = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL;

    node->physical_id_int = 0;

    if( NULL != node->physical_id ) {
        free(node->physical_id);
        node->physical_id = NULL;
    }

    if( NULL != node->logical_id ) {
        free(node->logical_id);
        node->logical_id = NULL;
    }

    if( NULL != node->subnet_id ) {
        free( node->subnet_id );
        node->subnet_id = NULL;
    }

    if( NULL != node->description ) {
        free(node->description);
        node->description = NULL;
    }

    if( NULL != node->userdata ) {
        node->userdata = NULL;
    }

    if( NULL != node->edges ) {
        for(i = 0; i < node->num_edges; ++i ) {
            // Do not deallocate the edges here, since we are just holding pointers
            // into the lookup table.
            node->edges[i] = NULL;
        }

        free(node->edges);
        node->edges = NULL;
        node->num_edges = 0;
    }

    if( NULL != node->edge_ids ) {
        free(node->edge_ids);
        node->edge_ids = NULL;
        node->num_edge_ids = 0;
    }

    if( NULL != node->physical_paths ) {
        // Destruct all of the pointer arrays in the table
        hti = netloc_dt_lookup_table_iterator_t_construct(node->physical_paths);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            path = (void*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == path ) {
                break;
            }
            // Path is a NULL terminated array of edge_ids to that destination.
            free(path);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);

        netloc_lookup_table_destroy(node->physical_paths);
        free(node->physical_paths);
        node->physical_paths = NULL;
        node->num_phy_paths = 0;
    }

    if( NULL != node->logical_paths ) {
        // Destruct all of the pointer arrays in the table
        hti = netloc_dt_lookup_table_iterator_t_construct(node->logical_paths);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            path = (void*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == path ) {
                break;
            }
            // Path is a NULL terminated array of edge_ids to that destination.
            free(path);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);

        netloc_lookup_table_destroy(node->logical_paths);
        free(node->logical_paths);
        node->logical_paths = NULL;
        node->num_log_paths = 0;
    }

    free(node);
    node = NULL;

    return NETLOC_SUCCESS;
}

int netloc_dt_node_t_compare(netloc_node_t *a, netloc_node_t *b)
{
    if( 0 != strncmp(a->physical_id, b->physical_id, strlen(a->physical_id)) ) {
        return NETLOC_CMP_DIFF;
    }

    return NETLOC_CMP_SAME;
}

/**************************************************/
unsigned long netloc_dt_convert_mac_str_to_int(const char * mac)
{
    unsigned long value;
    unsigned char addr[6];

    sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);

    value = 0;
    value = value | addr[0];
    value = value << 8;
    value = value | addr[1];
    value = value << 8;
    value = value | addr[2];
    value = value << 8;
    value = value | addr[3];
    value = value << 8;
    value = value | addr[4];
    value = value << 8;
    value = value | addr[5];

    return value;
}

char * netloc_dt_convert_mac_int_to_str(const unsigned long value)
{
    unsigned char addr[6];
    unsigned long rem;
    char * tmp_str = NULL;

    rem = value;

    addr[0] = rem >> 40;
    rem = rem << 8;
    addr[1] = rem >> 40;
    rem = rem << 8;
    addr[2] = rem >> 40;
    rem = rem << 8;
    addr[3] = rem >> 40;
    rem = rem << 8;
    addr[4] = rem >> 40;
    rem = rem << 8;
    addr[5] = rem >> 40;

    asprintf(&tmp_str, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
             addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    return tmp_str;
}

unsigned long netloc_dt_convert_guid_str_to_int(const char * guid)
{
    unsigned long value;
    short unsigned int addr[4];

    sscanf(guid, "%hX:%hX:%hX:%hX",
           &addr[0], &addr[1], &addr[2], &addr[3]);

    value = 0;
    value = value | addr[0];
    value = value << 16;
    value = value | addr[1];
    value = value << 16;
    value = value | addr[2];
    value = value << 16;
    value = value | addr[3];

    return value;
}

char * netloc_dt_convert_guid_int_to_str(const unsigned long value)
{
    short unsigned int addr[4];
    unsigned long rem;
    char * tmp_str = NULL;

    rem = value;

    addr[0] = rem >> 48;
    rem = rem << 16;
    addr[1] = rem >> 48;
    rem = rem << 16;
    addr[2] = rem >> 48;
    rem = rem << 16;
    addr[3] = rem >> 48;

    asprintf(&tmp_str, "%04hX:%04hX:%04hX:%04hX",
             addr[0], addr[1], addr[2], addr[3]);

    return tmp_str;
}
