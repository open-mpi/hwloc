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
    network->refcount     = 1;
    network->userdata     = NULL;

    return network;
}

int netloc_dt_network_t_destruct(netloc_network_t * network)
{
    if( NULL == network )
        return NETLOC_SUCCESS;

    if (--network->refcount <= 0)
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



netloc_edge_t * netloc_dt_edge_t_construct()
{
    static int cur_uid = 0;

    netloc_edge_t *edge = NULL;

    edge = (netloc_edge_t*)malloc(sizeof(netloc_edge_t));
    if( NULL == edge ) {
        return NULL;
    }

    edge->id = cur_uid;
    cur_uid++;

    edge->dest = NULL;
    edge->node = NULL;

    utarray_new(edge->physical_links, &ut_ptr_icd);

    edge->total_gbits = 0;

    utarray_new(edge->partitions, &ut_int_icd);

    utarray_new(edge->subnode_edges, &ut_ptr_icd);

    edge->userdata = NULL;

    return edge;
}

char * netloc_pretty_print_edge_t(netloc_edge_t* edge)
{
    // TODO XXX
    return "TODO";
}

char * netloc_pretty_print_link_t(netloc_physical_link_t* link)
{
    char * str = NULL;
    const char * tmp_src_str = NULL;
    const char * tmp_dest_str = NULL;

    tmp_src_str = netloc_decode_node_type(link->src->type);
    tmp_dest_str = netloc_decode_node_type(link->dest->type);

    asprintf(&str, "%3d (%s) [%23s] %d [<- %s / %s (%f) ->] (%s) [%23s] %d",
             link->id,
             tmp_src_str,
             link->src->physical_id,
             link->ports[0],
             link->speed,
             link->width,
             link->gbits,
             tmp_dest_str,
             link->dest->physical_id,
             link->ports[1]);

    return str;
}

int netloc_dt_edge_t_destruct(netloc_edge_t * edge)
{
    // TODO
    return NETLOC_SUCCESS;
}


/*******************************************************************/

static UT_icd node_physical_links_icd = {
    sizeof(netloc_physical_link_t *), NULL, NULL, NULL
};
static UT_icd node_physical_nodes_icd = {
    sizeof(netloc_node_t *), NULL, NULL, NULL
};
static UT_icd node_partitions_icd = { sizeof(int), NULL, NULL, NULL };

netloc_node_t * netloc_dt_node_t_construct()
{
    netloc_node_t *node = NULL;

    node = (netloc_node_t*)malloc(sizeof(netloc_node_t));
    if( NULL == node ) {
        return NULL;
    }

    node->physical_id[0]  = '\0';
    node->logical_id   = -1;
    node->type    = NETLOC_NODE_TYPE_INVALID;
    utarray_new(node->physical_links, &node_physical_nodes_icd);
    node->description  = NULL;
    node->userdata     = NULL;
    node->edges        = NULL;
    utarray_new(node->subnodes, &node_physical_links_icd);
    node->paths        = NULL;
    node->hostname     = NULL;
    utarray_new(node->partitions, &node_partitions_icd);
    node->hwlocTopoIdx = -1;

    return node;
}

char * netloc_pretty_print_node_t(netloc_node_t* node)
{
    char * str = NULL;

    asprintf(&str, " [%23s]/[%d] -- %s (%d links)",
             node->physical_id,
             node->logical_id,
             node->description,
             utarray_len(node->physical_links));

    return str;
}

int netloc_dt_node_t_destruct(netloc_node_t * node)
{
    // TODO
    return NETLOC_SUCCESS;
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

netloc_physical_link_t * netloc_dt_physical_link_t_construct()
{
    static int cur_uid = 0;
    static UT_icd int_icd = {sizeof(int), NULL, NULL, NULL };


    netloc_physical_link_t *physical_link = NULL;

    physical_link = (netloc_physical_link_t*)
        malloc(sizeof(netloc_physical_link_t));
    if( NULL == physical_link ) {
        return NULL;
    }

    physical_link->id = cur_uid;
    cur_uid++;

    physical_link->src = NULL;
    physical_link->dest = NULL;

    physical_link->ports[0] = -1;
    physical_link->ports[1] = -1;

    physical_link->width = NULL;
    physical_link->speed = NULL;

    physical_link->edge = NULL;
    physical_link->other_way = NULL;

    utarray_new(physical_link->partitions, &ut_int_icd);

    physical_link->gbits = -1;

    physical_link->description = NULL;

    return physical_link;
}

