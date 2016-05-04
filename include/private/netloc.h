/*
 * Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2015-2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _NETLOC_PRIVATE_H_
#define _NETLOC_PRIVATE_H_

#include <netloc.h>
#include <jansson.h>
#include <netloc/uthash.h>
#include <netloc/utarray.h>



#define NETLOC_EDGE_UID_START    0
#define NETLOC_EDGE_UID_NULL    -1
#define NETLOC_EDGE_UID_INVALID -2


/**********************************************************************
 *        Topology object
 **********************************************************************/
/**
 * Topology state used by the API functions.
 */
struct netloc_topology {
    /** Copy of the network structure */
    netloc_network_t *network;

    /** Lazy load the node list */
    bool nodes_loaded;

    /** Node List */
    netloc_node_t *nodes;

    netloc_physical_link_t *physical_links;

    /** Partition List */
    UT_array *partitions;

    /** Hwloc topology List */
    UT_array *topos;

    /** Type of the graph */
    netloc_topology_type_t type;
};


/**********************************************************************
 *        Datatype support functionality
 **********************************************************************/
/**
 * Constructor for netloc_edge_t
 *
 * User is responsible for calling the destructor on the handle.
 *
 * Returns
 *   A newly allocated pointer to the edge information.
 */
NETLOC_DECLSPEC netloc_edge_t * netloc_dt_edge_t_construct(void);

/**
 * Destructor for netloc_edge_t
 *
 * \param edge A valid edge handle
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_edge_t_destruct(netloc_edge_t *edge);

/**
 * Copy Constructor for netloc_edge_t
 *
 * Allocates memory. User is responsible for calling _destruct on the returned pointer.
 * Does a shallow copy of the pointers to data.
 *
 * \param edge A pointer to the edge to duplicate
 *
 * Returns
 *   A newly allocated copy of the edge.
 */
NETLOC_DECLSPEC netloc_edge_t * netloc_dt_edge_t_dup(netloc_edge_t *edge);

/**
 * Copy Function for netloc_edge_t
 *
 * Does not allocate memory for 'to'. Does a shallow copy of the pointers to data.
 *
 * \param from A pointer to the edge to duplicate
 * \param to A pointer to the edge to duplicate into
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_edge_t_copy(netloc_edge_t *from, netloc_edge_t *to);

/*************************************************/

/**
 * Constructor for netloc_node_t
 *
 * User is responsible for calling the destructor on the handle.
 *
 * Returns
 *   A newly allocated pointer to the network information.
 */
NETLOC_DECLSPEC netloc_node_t * netloc_dt_node_t_construct(void);

/**
 * Destructor for netloc_node_t
 *
 * \param node A valid node handle
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_node_t_destruct(netloc_node_t *node);

/**
 * Copy Constructor for netloc_node_t
 *
 * Allocates memory. User is responsible for calling _destruct on the returned pointer.
 * Does a shallow copy of the pointers to data.
 *
 * \param node A pointer to the node to duplicate
 *
 * Returns
 *   A newly allocated copy of the node.
 */
NETLOC_DECLSPEC netloc_node_t * netloc_dt_node_t_dup(netloc_node_t *node);

/**
 * Copy Function for netloc_node_t
 *
 * Does not allocate memory for 'to'. Does a shallow copy of the pointers to data.
 *
 * \param from A pointer to the node to duplicate
 * \param to A pointer to the node to duplicate into
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_node_t_copy(netloc_node_t *from, netloc_node_t *to);


/*************************************************/

/**
 * Convert a string MAC address (':' separated) into a whole number value
 *
 * \param mac String MAC address
 *
 * Returns
 *  whole number encoding of that value
 */
NETLOC_DECLSPEC unsigned long netloc_dt_convert_mac_str_to_int(const char * mac);

/**
 * Convert a value encoding a MAC address into a string representation (':' separated)
 *
 * Caller is responsible for free'ing the pointer returned.
 *
 * \param value encoded value MAC address
 *
 * Returns
 *  NULL on error
 *  otherwise string representation.
 */
NETLOC_DECLSPEC char * netloc_dt_convert_mac_int_to_str(const unsigned long value);

/**
 * Convert a string GUID address (':' separated) into a whole number value
 *
 * \param guid String GUID address
 *
 * Returns
 *  whole number encoding of that value
 */
NETLOC_DECLSPEC unsigned long netloc_dt_convert_guid_str_to_int(const char * guid);

/**
 * Convert a value encoding a GUID address into a string representation (':' separated)
 *
 * Caller is responsible for free'ing the pointer returned.
 *
 * \param value encoded value GUID address
 *
 * Returns
 *  NULL on error
 *  otherwise string representation.
 */
NETLOC_DECLSPEC char * netloc_dt_convert_guid_int_to_str(const unsigned long value);


/**********************************************************************
 *        JSON Encode/Decode functionality
 **********************************************************************/
/**
 * JSON Encode the data
 *
 * \param network A pointer to the network to process
 *
 * Returns
 *   A valid json object representing the network information
 */
NETLOC_DECLSPEC json_t* netloc_dt_network_t_json_encode(netloc_network_t *network);

/**
 * JSON Decode the data
 *
 * User is responsible for calling _destruct on the returned pointer.
 *
 * \param json_nw A point to a valid json object representing the network information
 *
 * Returns
 *   A newly allocated network type filled in with the stored information
 */
NETLOC_DECLSPEC netloc_network_t* netloc_dt_network_t_json_decode(json_t *json_nw);

/*************************************************/

/**
 * JSON Encode the data
 *
 * \param edge A pointer to the edge to process
 *
 * Returns
 *   A valid json object representing the edge information
 */
NETLOC_DECLSPEC json_t* netloc_dt_edge_t_json_encode(netloc_edge_t *edge);

/**
 * JSON Decode the data
 *
 * User is responsible for calling _destruct on the returned pointer.
 *
 * \param json_edge A point to a valid json object representing the edge information
 *
 * Returns
 *   A newly allocated edge type filled in with the stored information
 */
NETLOC_DECLSPEC netloc_edge_t* netloc_dt_edge_t_json_decode(json_t *json_edge);

/*************************************************/

/**
 * JSON Encode the data
 *
 * This will -not- encode the path information
 *
 * \param node A pointer to the node to process
 *
 * Returns
 *   A valid json object representing the node information
 */
NETLOC_DECLSPEC json_t* netloc_dt_node_t_json_encode(netloc_node_t *node);

NETLOC_DECLSPEC netloc_physical_link_t * netloc_dt_physical_link_t_construct(void);

/**********************************************************************
 *        Expandable list
 **********************************************************************/

typedef struct {
    int size;
    int max_size;
    void **array;
} netloc_explist_t;

netloc_explist_t *netloc_explist_init(int size);
void netloc_explist_push(netloc_explist_t *list, void *elem);
void *netloc_explist_pop(netloc_explist_t *list);
void netloc_explist_cat(netloc_explist_t *list1, netloc_explist_t *list2);
void netloc_explist_set(netloc_explist_t *list, int idx, void *elem);
int netloc_explist_get_size(netloc_explist_t *list);
void *netloc_explist_get(netloc_explist_t *list, int idx);
void netloc_explist_destroy(netloc_explist_t *list);
void **netloc_explist_get_array_and_destroy(netloc_explist_t *list);

/**********************************************************************
 *        Analysis functionality
 **********************************************************************/

/* Access functions XXX */
#define netloc_get_num_partitions(object) \
    utarray_len((object)->partitions)
#define netloc_get_partition(object,i) \
    (*(int *)utarray_eltptr((object)->partitions, (i)))
#define netloc_edge_get_num_links(edge) \
    utarray_len((edge)->physical_links)
#define netloc_edge_get_link(edge,i) \
    (*(netloc_physical_link_t **)utarray_eltptr((edge)->physical_links, (i)))

#define netloc_node_get_num_subnodes(node) \
    utarray_len((node)->subnodes)
#define netloc_node_get_subnode(node,i) \
    (*(netloc_node_t **)utarray_eltptr((node)->subnodes, (i)))

#define netloc_node_get_num_edges(node) \
    utarray_len((node)->edges)
#define netloc_node_get_edge(node,i) \
    (*(netloc_edge_t **)utarray_eltptr((node)->edges, (i)))

#define netloc_node_iter_edges(node,edge,_tmp) \
    HASH_ITER(hh, node->edges, edge, _tmp)

#define netloc_edge_get_num_subedges(edge) \
    utarray_len((edge)->subnode_edges)
#define netloc_edge_get_subedge(edge,i) \
    (*(netloc_edge_t **)utarray_eltptr((edge)->subnode_edges, (i)))


// XXX TODO choose between get_num and get_object, and iter_ibject
#define netloc_topology_iter_partitions(topology,partition) \
    for ((partition) = (char **)utarray_front(topology->partitions); \
            (partition) != NULL; \
            (partition) = (char **)utarray_next(topology->partitions, partition))

#define netloc_topology_iter_hwloctopos(topology,hwloctopo) \
    for ((hwloctopo) = (char **)utarray_front(topology->topos); \
            (hwloctopo) != NULL; \
            (hwloctopo) = (char **)utarray_next(topology->topos, hwloctopo))

#define netloc_path_iter_links(path,link) \
    for ((link) = (netloc_physical_link_t **)utarray_front(path->links); \
            (link) != NULL; \
            (link) = (netloc_physical_link_t **)utarray_next(path->links, link))

#define netloc_topology_find_node(topology,node_id,node) \
    HASH_FIND_STR(topology->nodes, node_id, node)

#define netloc_topology_iter_nodes(topology,node,_tmp) \
    HASH_ITER(hh, topology->nodes, node, _tmp)

#define netloc_node_iter_paths(node,path,_tmp) \
    HASH_ITER(hh, node->paths, path, _tmp)

#define netloc_topology_num_nodes(topology) \
    HASH_COUNT(topology->nodes)

#define netloc_node_is_host(node) \
    (node->type == NETLOC_NODE_TYPE_HOST)

#define netloc_node_is_switch(node) \
    (node->type == NETLOC_NODE_TYPE_SWITCH)

#define netloc_node_iter_paths(node, path,_tmp) \
    HASH_ITER(hh, node->paths, path, _tmp)

int support_load_datafile(struct netloc_topology * topology);

#endif // _NETLOC_PRIVATE_H_
