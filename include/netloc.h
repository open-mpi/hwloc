/*
 * Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
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

#ifndef _NETLOC_H_
#define _NETLOC_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for asprintf
#endif

#include <hwloc/autogen/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <scotch.h>

#include <netloc/uthash.h>
#include <netloc/utarray.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * "Import" a few things from hwloc
 */
#define __netloc_attribute_unused __hwloc_attribute_unused
#define __netloc_attribute_malloc __hwloc_attribute_malloc
#define __netloc_attribute_const __hwloc_attribute_const
#define __netloc_attribute_pure __hwloc_attribute_pure
#define __netloc_attribute_deprecated __hwloc_attribute_deprecated
#define __netloc_attribute_may_alias __hwloc_attribute_may_alias
#define NETLOC_DECLSPEC HWLOC_DECLSPEC


/** \defgroup netloc_api Netloc API
 * @{
 */

/**********************************************************************
 * Enumerated types
 **********************************************************************/

/**
 * Definitions for Comparators
 * \sa These are the return values from the following functions:
 *     netloc_dt_network_t_compare, netloc_dt_edge_t_compare, netloc_dt_node_t_compare
 */
typedef enum {
    NETLOC_CMP_SAME    =  0,  /**< Compared as the Same */
    NETLOC_CMP_SIMILAR = -1,  /**< Compared as Similar, but not the Same */
    NETLOC_CMP_DIFF    = -2   /**< Compared as Different */
} netloc_compare_type_t;

/**
 * Enumerated type for the various types of supported networks
 */
typedef enum {
    NETLOC_NETWORK_TYPE_ETHERNET    = 1, /**< Ethernet network */
    NETLOC_NETWORK_TYPE_INFINIBAND  = 2, /**< InfiniBand network */
    NETLOC_NETWORK_TYPE_INVALID     = 3  /**< Invalid network */
} netloc_network_type_t;

/**
 * Enumerated type for the various types of supported topologies
 */
typedef enum {
    NETLOC_TOPOLOGY_TYPE_INVALID = -1, /**< Invalid */
    NETLOC_TOPOLOGY_TYPE_TREE    = 1,  /**< Tree */
} netloc_topology_type_t;


/**
 * Encode the network type
 *
 * \note Only used by netloc readers to encode the network type
 *
 * \param str_val String value to parse
 *
 * \returns  A valid member of the \ref netloc_network_type_t type
 */
static inline netloc_network_type_t netloc_encode_network_type(const char * str_val) {
    if( NULL == str_val ) {
        return NETLOC_NETWORK_TYPE_INVALID;
    }
    else if( 0 == strncmp(str_val, "ETH", strlen(str_val)) ) {
        return NETLOC_NETWORK_TYPE_ETHERNET;
    }
    else if( 0 == strncmp(str_val, "IB", strlen(str_val)) ) {
        return NETLOC_NETWORK_TYPE_INFINIBAND;
    }
    else {
        return NETLOC_NETWORK_TYPE_INVALID;
    }
}
// TODO XXX use it

/**
 * Decode the network type
 *
 * \param net_type A valid member of the \ref netloc_network_type_t type
 *
 * \returns NULL if the type is invalid
 * \returns A string for that \ref netloc_network_type_t type
 */
static inline const char * netloc_decode_network_type(netloc_network_type_t net_type) {
    if( NETLOC_NETWORK_TYPE_ETHERNET == net_type ) {
        return "ETH";
    }
    else if( NETLOC_NETWORK_TYPE_INFINIBAND == net_type ) {
        return "IB";
    }
    else {
        return NULL;
    }
}
// TODO XXX use it

/**
 * Decode the network type into a human readable string
 *
 * \param net_type A valid member of the \ref netloc_network_type_t type
 *
 * \returns A string for that \ref netloc_network_type_t type
 */
static inline const char * netloc_decode_network_type_readable(netloc_network_type_t net_type) {
    if( NETLOC_NETWORK_TYPE_ETHERNET == net_type ) {
        return "Ethernet";
    }
    else if( NETLOC_NETWORK_TYPE_INFINIBAND == net_type ) {
        return "InfiniBand";
    }
    else {
        return "Invalid";
    }
}
// TODO XXX use it


/**
 * Enumerated type for the various types of nodes
 */
typedef enum {
    NETLOC_NODE_TYPE_HOST    = 0, /**< Host (a.k.a., network addressable endpoint - e.g., MAC Address) node */
    NETLOC_NODE_TYPE_SWITCH  = 1, /**< Switch node */
    NETLOC_NODE_TYPE_INVALID = 2  /**< Invalid node */
} netloc_node_type_t;

/**
 * Encode the node type
 *
 * \note Only used by netloc readers to encode the network type
 *
 * \param str_val String value to parse
 *
 * \returns  A valid member of the \ref netloc_node_type_t type
 */
static inline netloc_node_type_t netloc_encode_node_type(const char * str_val) {
    if( NULL == str_val ) {
        return NETLOC_NODE_TYPE_INVALID;
    }
    else if( 0 == strncmp(str_val, "CA", strlen(str_val)) ) {
        return NETLOC_NODE_TYPE_HOST;
    }
    else if( 0 == strncmp(str_val, "SW", strlen(str_val)) ) {
        return NETLOC_NODE_TYPE_SWITCH;
    }
    else {
        return NETLOC_NODE_TYPE_INVALID;
    }
}
// TODO XXX use it

/**
 * Decode the node type
 *
 * \param node_type A valid member of the \ref netloc_node_type_t type
 *
 * \returns NULL if the type is invalid
 * \returns A string for that \ref netloc_node_type_t type
 */
static inline const char * netloc_decode_node_type(netloc_node_type_t node_type) {
    if( NETLOC_NODE_TYPE_SWITCH == node_type ) {
        return "SW";
    }
    else if( NETLOC_NODE_TYPE_HOST == node_type ) {
        return "CA";
    }
    else {
        return NULL;
    }
}
// TODO XXX use it

/**
 * Decode the node type into a human readable string
 *
 * \param node_type A valid member of the \ref netloc_node_type_t type
 *
 * \returns NULL if the type is invalid
 * \returns A string for that \ref netloc_node_type_t type
 */
static inline const char * netloc_decode_node_type_readable(netloc_node_type_t node_type) {
    if( NETLOC_NODE_TYPE_SWITCH == node_type ) {
        return "Switch";
    }
    else if( NETLOC_NODE_TYPE_HOST == node_type ) {
        return "Host";
    }
    else {
        return "Invalid";
    }
}
// TODO XXX use it

/**
 * Return codes
 */
enum {
    NETLOC_SUCCESS         =  0, /**< Success */
    NETLOC_ERROR           = -1, /**< Error: General condition */
    NETLOC_ERROR_NOTDIR    = -2, /**< Error: URI is not a directory */
    NETLOC_ERROR_NOENT     = -3, /**< Error: URI is invalid, no such entry */
    NETLOC_ERROR_EMPTY     = -4, /**< Error: No networks found */
    NETLOC_ERROR_MULTIPLE  = -5, /**< Error: Multiple matching networks found */
    NETLOC_ERROR_NOT_IMPL  = -6, /**< Error: Interface not implemented */
    NETLOC_ERROR_EXISTS    = -7, /**< Error: If the entry already exists when trying to add to a lookup table */
    NETLOC_ERROR_NOT_FOUND = -8, /**< Error: No path found */
    NETLOC_ERROR_MAX       = -9  /**< Error: Enum upper bound marker. No errors less than this number Will not be returned externally. */
};

/**********************************************************************
 *        Structures
 **********************************************************************/

/**
 * \struct netloc_topology_t
 * \brief Netloc Topology Context
 *
 * An opaque data structure used to reference a network topology.
 *
 * \note Must be initialized with \ref netloc_attach()
 */
struct netloc_topology;
/** \cond IGNORE */
typedef struct netloc_topology * netloc_topology_t;
/** \endcond */

/**
 * \brief Netloc Network Type
 *
 * Represents a single network type and subnet.
 */
struct netloc_network_t {
    /** Type of network */
    netloc_network_type_t network_type;

    /** Subnet ID */
    char * subnet_id;

    /** Data URI */
    char * data_uri;

    /** Node URI */
    char * node_uri;

    /** Physical Path URI */
    char * phy_path_uri;

    /** Path URI */
    char * path_uri;

    /** Description information from discovery (if any) */
    char * description;

    /** Metadata about network information */
    char * version;

    int refcount;

    /**
     * Application-given private data pointer.
     * Initialized to NULL, and not used by the netloc library.
     */
    void * userdata;
};
typedef struct netloc_network_t netloc_network_t;


/* Predefine the netloc_node_t structure so we can use it in the netloc_edge_t */
struct netloc_node_t;
typedef struct netloc_node_t netloc_node_t;
struct netloc_node2_t;
typedef struct netloc_node2_t netloc_node2_t;


/**
 * \brief Netloc Edge Type
 *
 * Represents the concept of a directed edge within a network graph.
 *
 * \note We do not point to the netloc_node_t structure directly to
 * simplify the representation, and allow the information to more easily
 * be entered into the data store without circular references.
 * \todo JJH Is the note above still true?
 */
struct netloc_edge_t {
    UT_hash_handle hh;       /* makes this structure hashable */

    netloc_node_t *dest;

    int id;

    /** Pointers to the parent node */
    netloc_node_t *node;

    /* Pointer to physical_links */
    UT_array *physical_links;

    /** total gbits of the links */
    float total_gbits;

    UT_array *partitions; /* index in the list from the topology */

    UT_array *subnode_edges; /* for edges going to virtual nodes */

    struct netloc_edge_t *other_way;

    /**
     * Application-given private data pointer.
     * Initialized to NULL, and not used by the netloc library.
     */
    void * userdata;
};
typedef struct netloc_edge_t netloc_edge_t;

struct netloc_physical_link_t {
    UT_hash_handle hh;       /* makes this structure hashable */

    int id; // TODO long long
    netloc_node_t *src;
    netloc_node_t *dest;
    int ports[2];
    char *width;
    char *speed;

    netloc_edge_t *edge;

    int other_way_id;
    struct netloc_physical_link_t *other_way;

    UT_array *partitions; /* index in the list from the topology */

    /** gbits of the link from speed and width */
    float gbits;

    /** Description information from discovery (if any) */
    char *description;
};
typedef struct netloc_physical_link_t netloc_physical_link_t;

struct netloc_path_t {
    UT_hash_handle hh;       /* makes this structure hashable */

    char dest_id[20];

    UT_array *links;
};
typedef struct netloc_path_t netloc_path_t;

/**
 * \brief Netloc Node Type
 *
 * Represents the concept of a node (a.k.a., vertex, endpoint) within a network
 * graph. This could be a server or a network switch. The \ref node_type parameter
 * will distinguish the exact type of node this represents in the graph.
 */
struct netloc_node_t {
    UT_hash_handle hh;       /* makes this structure hashable */

    /** Physical ID of the node */
    char physical_id[20];

    /** Logical ID of the node (if any) */
    int logical_id;

    /** Type of the node */
    netloc_node_type_t type;

    /* Pointer to physical_links */
    UT_array *physical_links;

    /** Description information from discovery (if any) */
    char *description;

    /**
     * Application-given private data pointer.
     * Initialized to NULL, and not used by the netloc library.
     */
    void * userdata;

    /** Outgoing edges from this node */
    netloc_edge_t *edges;

    UT_array *subnodes; /* the group of nodes for the virtual nodes */

    netloc_path_t *paths;

    char *hostname;

    UT_array *partitions; /* index in the list from the topology */

    int topoIdx;  /* index in the list from the topology */
};


typedef enum {
    NETLOC_ARCH_TREE    =  0,  /* Fat tree */
} netloc_arch_type_t;
typedef struct {
    UT_hash_handle hh;       /* makes this structure hashable */
    char id[20];
    int idx; /* Rank in the scotch_arch */
} netloc_arch_host_t;
typedef struct {
    int num_levels;
    int *degrees;
    int *throughput;
    int num_cores;
    netloc_arch_host_t *hosts;
} netloc_arch_tree_t;
typedef struct {
    netloc_arch_type_t type;
    union {
        netloc_arch_tree_t *tree;
    } arch;
} netloc_arch_t;
/**********************************************************************
 * Topology API Functions
 **********************************************************************/
/**
 * Attach to the specified network, and allocate a topology handle.
 *
 * User is responsible for calling \ref netloc_detach on the topology handle.
 * The network parameter information is deep copied into the topology handle, so the
 * user may destruct the network handle after calling this function and/or reuse
 * the network handle.
 *
 * \param topology A pointer to a netloc_topology_t handle.
 * \param network The \ref netloc_network_t handle from a prior call to either:
 *                - \ref netloc_find_network()
 *                - \ref netloc_foreach_network()
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_attach(netloc_topology_t * topology, netloc_network_t network);

/**
 * Detach from a topology handle
 *
 * \param topology A valid pointer to a \ref netloc_topology_t handle created
 * from a prior call to \ref netloc_attach.
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_detach(netloc_topology_t topology);

/**********************************************************************
 * Datatype Support Functions
 **********************************************************************/
/**
 * Constructor for \ref netloc_network_t
 *
 * User is responsible for calling the destructor on the handle.
 *
 * \returns A newly allocated pointer to the network information.
 */
NETLOC_DECLSPEC netloc_network_t * netloc_dt_network_t_construct(void);

/**
 * Destructor for \ref netloc_network_t
 *
 * \param network A valid network handle, or \c NULL.
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_network_t_destruct(netloc_network_t *network);


/**
 * Find a specific network at the URI specified.
 *
 * \param network_topo_uri URI to search for the specified network.
 * \param network  Netloc network handle (IN/OUT)
 *                 A network handle with the data structure fields set to
 *                 specify the search. For example, the user can set 'IB'
 *                 and nothing else, if they do not know the subnet or any
 *                 of the other necessary information. If the method
 *                 returns success then the network handle will be filled
 *                 out with the rest of the information found. If the method
 *                 returns some error then the network handle is not modified.
 *
 * \returns NETLOC_SUCCESS if exactly one network matches the specification,
 *                  and updates the network handle.
 * \returns NETLOC_ERROR_MULTIPLE if more than one network matches the spec.
 * \returns NETLOC_ERROR_EMPTY if no networks match the specification.
 * \returns NETLOC_ERROR_NOENT if the directory does not exist.
 * \returns NETLOC_ERROR_NOTDIR if the data_dir is not a directory.
 * \returns NETLOC_ERROR if something else is wrong.
 */
NETLOC_DECLSPEC int netloc_find_network(const char * network_topo_uri,
                                        netloc_network_t* network);

/**********************************************************************
 * Network Metadata API Functions
 **********************************************************************/
/**
 * Pretty print the network (Debugging Support)
 *
 * The user is responsible for calling free() on the string returned.
 *
 * \param network A valid pointer to a network
 *
 * \returns A newly allocated string representation of the network.
 */
NETLOC_DECLSPEC char * netloc_pretty_print_network_t(netloc_network_t* network);

NETLOC_DECLSPEC int netloc_dt_network_t_copy(netloc_network_t *from, netloc_network_t *to);
NETLOC_DECLSPEC int netloc_dt_network_t_compare(netloc_network_t *a, netloc_network_t *b);
NETLOC_DECLSPEC netloc_network_t * netloc_dt_network_t_dup(netloc_network_t *network);

netloc_network_t* netloc_access_network_ref(struct netloc_topology * topology);

int netloc_arch_build(netloc_arch_t *arch);
int slurm_get_current_nodes(int *pnum_nodes, char ***pnodes);
int slurm_get_proc_number(int *pnum_ppn);
int pbs_get_current_nodes(int *pnum_nodes, char ***pnodes);
int pbs_get_proc_number(int *pnum_ppn);
int netloc_edge_is_in_partition(netloc_edge_t *edge, int partition);
int netloc_node_is_in_partition(netloc_node_t *node, int partition);
int netloc_topology_find_partition_idx(netloc_topology_t topology, char *partition_name);
int hwloc_get_core_number(int *pnum_cores);
int netloc_get_current_nodes(int *pnum_nodes, char ***pnodes);
int netloc_arch_find_current_hosts(netloc_arch_t *arch, char **nodelist,
        int num_nodes, netloc_arch_host_t ***phost_list);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif // _NETLOC_H_
