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
#include <netloc/rename.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
    NETLOC_TOPOLOGY_TYPE_TREE    = 1, /**< Tree */
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


/**
 * Enumerated type for the various types of nodes
 */
typedef enum {
    NETLOC_NODE_TYPE_SWITCH  = 1, /**< Switch node */
    NETLOC_NODE_TYPE_HOST    = 2, /**< Host (a.k.a., network addressable endpoint - e.g., MAC Address) node */
    NETLOC_NODE_TYPE_INVALID = 3  /**< Invalid node */
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
 * \struct netloc_dt_lookup_table_t
 * \brief Lookup Table Type
 *
 * An opaque data structure to represent a collection of data items
 */
struct netloc_dt_lookup_table;
/** \cond IGNORE */
typedef struct netloc_dt_lookup_table * netloc_dt_lookup_table_t;
/** \endcond */

/**
 * \struct netloc_dt_lookup_table_iterator_t
 * \brief Lookup Table Iterator
 *
 * An opaque data structure representing the next location in the lookup table
 */
struct netloc_dt_lookup_table_iterator;
/** \cond IGNORE */
typedef struct netloc_dt_lookup_table_iterator * netloc_dt_lookup_table_iterator_t;
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
    /** Unique Edge ID */
    int edge_uid;

    /** Source: Pointer to neloc_node_t */
    netloc_node_t     *src_node;
    /** Source: Physical ID from netloc_node_t */
    char *             src_node_id;
    /** Source: Node type from netloc_node_t */
    netloc_node_type_t src_node_type;
    /** Source: Port number */
    char *             src_port_id;

    /** Dest: Pointer to neloc_node_t */
    netloc_node_t     *dest_node;
    /** Dest: Physical ID from netloc_node_t */
    char *             dest_node_id;
    /** Dest: Node type from netloc_node_t */
    netloc_node_type_t dest_node_type;
    /** Dest: Port number */
    char *             dest_port_id;

    /** Metadata: Speed */
    char *      speed;
    /** Metadata: Width */
    char *      width;

    /** gbits of the link from speed and width */
    float gbits;

    /** Description information from discovery (if any) */
    char * description;

    /** If it is a merged edge, list of corresponding edges */
    int num_real_edges;
    struct netloc_edge_t **real_edges;

    /** If it is a merged edge, corresponding virtual edge */
    struct netloc_edge_t *virtual_edge;

    int *partitions; /* index in the list from the topology */
    int num_partitions;

    /**
     * Application-given private data pointer.
     * Initialized to NULL, and not used by the netloc library.
     */
    void * userdata;
};
typedef struct netloc_edge_t netloc_edge_t;

/**
 * \brief Netloc Node Type
 *
 * Represents the concept of a node (a.k.a., vertex, endpoint) within a network
 * graph. This could be a server or a network switch. The \ref node_type parameter
 * will distinguish the exact type of node this represents in the graph.
 */
struct netloc_node_t {
    /** Type of the network connection */
    netloc_network_type_t  network_type;

    /** Type of the node */
    netloc_node_type_t     node_type;

    /** Physical ID of the node (must be unique) */
    char *          physical_id;
    unsigned long   physical_id_int;

    /** Logical ID of the node (if any) */
    char *          logical_id;

    /** Internal unique ID: 0 - N */
    int __uid__;

    /** Subnet ID */
    char * subnet_id;

    /** Description information from discovery (if any) */
    char * description;

    /**
     * Application-given private data pointer.
     * Initialized to NULL, and not used by the netloc library.
     */
    void * userdata;

    /** Number of Outgoing edges from this node */
    int num_edges;
    /** Outgoing edges from this node */
    netloc_edge_t **edges;

    /** Number of edge IDs (Internal use only) */
    int num_edge_ids;
    /** Edge IDs (Internal use only) */
    int *edge_ids;

    /** Number of physical paths computed from this node */
    int num_phy_paths;
    /** Lookup table for physical paths from this node */
    netloc_dt_lookup_table_t physical_paths;

    /** Number of logical paths computed from this node */
    int num_log_paths;
    /** Lookup table for logical paths from this node */
    netloc_dt_lookup_table_t logical_paths;

    /** If it is a merged node, list of the corresponding nodes */
    int num_real_nodes;
    struct netloc_node_t **real_nodes;

    /** If it is a merged node, corresponding virtual node*/
    struct netloc_node_t *virtual_node;

    char *hostname;

    int *partitions; /* index in the list from the topology */
    int num_partitions;

    int topoIdx; /* index to topology->topos */
};


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
 * Copy Constructor for \ref netloc_network_t
 *
 * Allocates memory. User is responsible for calling
 * \ref netloc_dt_network_t_destruct on the returned pointer.
 * Does a shallow copy of the pointers to data.
 *
 * \param network A pointer to the network to duplicate
 *
 * \returns A newly allocated copy of the network.
 */
NETLOC_DECLSPEC netloc_network_t * netloc_dt_network_t_dup(netloc_network_t *network);

/**
 * Copy Function for \ref netloc_network_t
 *
 * Does not allocate memory for 'to'. Does a shallow copy of the pointers to data.
 *
 * \param from A pointer to the network to duplicate
 * \param to A pointer to the network to duplicate into
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_network_t_copy(netloc_network_t *from, netloc_network_t *to);

/**
 * Compare function for \ref netloc_network_t
 *
 * \param a A pointer to one network object for comparison
 * \param b A pointer to the other network object for comparison
 *
 * \returns \ref NETLOC_CMP_SAME if the same
 * \returns \ref NETLOC_CMP_SIMILAR if only the metadata (e.g., version) is different
 * \returns \ref NETLOC_CMP_DIFF if different
 */
NETLOC_DECLSPEC int netloc_dt_network_t_compare(netloc_network_t *a, netloc_network_t *b);


/**
 * Compare function for \ref netloc_edge_t
 *
 * \param a A pointer to one edge object for comparison
 * \param b A pointer to the other edge object for comparison
 *
 * \returns \ref NETLOC_CMP_SAME if the same
 * \returns \ref NETLOC_CMP_DIFF if different
 */
NETLOC_DECLSPEC int netloc_dt_edge_t_compare(netloc_edge_t *a, netloc_edge_t *b);


/**
 * Compare function for netloc_node_t
 *
 * \param a A pointer to one network object for comparison
 * \param b A pointer to the other network object for comparison
 *
 * \returns \ref NETLOC_CMP_SAME if the same
 * \returns \ref NETLOC_CMP_DIFF if different
 */
NETLOC_DECLSPEC int netloc_dt_node_t_compare(netloc_node_t *a, netloc_node_t *b);


/**********************************************************************
 * Datatype Support Functions for Lookup Tables
 **********************************************************************/

/**
 * Constructor for a lookup table iterator
 *
 * User is responsible for calling the \ref netloc_dt_lookup_table_iterator_t_destruct on the handle.
 *
 * \param table The table to reference in this iterator
 *
 * \returns A newly allocated pointer to the lookup table iterator.
 */
NETLOC_DECLSPEC netloc_dt_lookup_table_iterator_t netloc_dt_lookup_table_iterator_t_construct(netloc_dt_lookup_table_t table);

/**
 * Destructor for a lookup table iterator
 *
 * \param hti A valid lookup table iterator handle
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_lookup_table_iterator_t_destruct(netloc_dt_lookup_table_iterator_t hti);


/**********************************************************************
 * Lookup table API Functions
 **********************************************************************/

/**
 * Destroy a lookup table.
 *
 * \note The user is responsible for calling this function if they are ever returned
 * a \ref netloc_dt_lookup_table_t from a function such as \ref netloc_get_all_nodes.
 *
 * \param table The lookup table to destroy
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_destroy(netloc_dt_lookup_table_t table);


/**
 * Access the -used- size of the lookup table
 *
 * \param table A valid pointer to a lookup table
 *
 * \returns The used size of the lookup table
 */
NETLOC_DECLSPEC size_t netloc_lookup_table_size(netloc_dt_lookup_table_t table);


/**
 * Access an entry in the lookup table
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 *
 * \returns NULL if nothing found
 * \returns The pointer associated with this key
 */
NETLOC_DECLSPEC void * netloc_lookup_table_access(netloc_dt_lookup_table_t ht, const char *key);


/**
 * Get the next key and advance the iterator
 *
 * The user should -not- call free() on the string returned.
 *
 * \param hti A valid pointer to a lookup table iterator
 *
 * \returns NULL if error or at end
 * \returns A newly allocated string copy of the key.
 */
NETLOC_DECLSPEC const char * netloc_lookup_table_iterator_next_key(netloc_dt_lookup_table_iterator_t hti);

/**
 * Get the next entry and advance the iterator
 *
 * Similar to \ref netloc_lookup_table_iterator_next_key except the caller is
 * given the next value directly. So they do not need to call the
 * \ref netloc_lookup_table_access function to access the value.
 *
 * \param hti A valid pointer to a lookup table iterator
 *
 * \returns NULL if error or at end
 * \returns The pointer associated with this key
 */
NETLOC_DECLSPEC void * netloc_lookup_table_iterator_next_entry(netloc_dt_lookup_table_iterator_t hti);

/**
 * Check if we are at the end of the iterator
 *
 * \param hti A valid pointer to a lookup table iterator
 *
 * \returns true if at the end of the data, false otherwise
 */
NETLOC_DECLSPEC bool netloc_lookup_table_iterator_at_end(netloc_dt_lookup_table_iterator_t hti);

/**
 * Reset the iterator back to the start
 *
 * \param hti A valid pointer to a lookup table iterator
 *
 */
NETLOC_DECLSPEC void netloc_lookup_table_iterator_reset(netloc_dt_lookup_table_iterator_t hti);



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

/**
 * Pretty print the edge (Debugging Support)
 *
 * The user is responsible for calling free() on the string returned.
 *
 * \param edge A valid pointer to an edge
 *
 * \returns A newly allocated string representation of the edge.
 */
NETLOC_DECLSPEC char * netloc_pretty_print_edge_t(netloc_edge_t* edge);

/**
 * Pretty print the node (Debugging Support)
 *
 * The user is responsible for calling free() on the string returned.
 *
 * \param node A valid pointer to a node
 *
 * \returns A newly allocated string representation of the node.
 */
NETLOC_DECLSPEC char * netloc_pretty_print_node_t(netloc_node_t* node);

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

/**
 * Find all available networks in the specified URIs
 *
 * User is responsible for calling the destructor for each element of the networks array
 * paramater, then free() on the entire array.
 *
 * \param search_uris Array of URIs. file:// syntax is the only supported mechanism
 *                    at the moment. Array is searched for .dat files. All uris will
 *                    be searched. If NULL is supplied then the default search path
 *                    will be used (currently the CWD).
 * \param num_uris Size of the search_uris array.
 * \param (*func) A callback function triggered for each network found the user
 *                is provided an opportunity to decide if it should be included
 *                in the "networks" array or not. "net" is a handle to the
 *                network information (includes uri where it was found).
 *                If the callback returns non-zero then the entry is added to
 *                the networks array. If the callback returns 0 then the entry
 *                is not added to the networks array. If NULL is supplied as
 *                an argument for this function pointer then all networks are
 *                included in the array.
 * \param funcdata User specified data pointer to be passed to the callback function.
 * \param num_networks Size of the networks array.
 * \param networks An array of networks discovered.
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR otherwise
 */
NETLOC_DECLSPEC int netloc_foreach_network(const char * const * search_uris,
                                             int num_uris,
                                             int (*func)(const netloc_network_t *network, void *funcdata),
                                             void *funcdata,
                                             int *num_networks,
                                             netloc_network_t ***networks);


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

/**
 * Refresh the data associated with the topology.
 *
 * \warning This interface is not currently implemented.
 *
 * \param topology A valid pointer to a \ref netloc_topology_t handle created
 * from a prior call to \ref netloc_attach.
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_refresh(netloc_topology_t topology);


/**********************************************************************
 * Query API Functions
 **********************************************************************/
/**
 * Access a reference to the \ref netloc_network_t associated with the \ref netloc_topology_t
 *
 * The user should -not- call \ref netloc_dt_network_t_destruct on the reference returned.
 *
 * \param topology A valid pointer to a topology handle
 *
 * \returns A reference to the \ref netloc_network_t associtated with the topology
 * \returns NULL on error.
 */
NETLOC_DECLSPEC netloc_network_t* netloc_access_network_ref(netloc_topology_t topology);

/**
 * Get all nodes in the network topology
 *
 * The user is responsible for calling the lookup table destructor on the nodes
 * table (\ref netloc_lookup_table_destroy).
 * The user should -not- call the netloc_node_t's destructor on the elements in
 * the lookup table. That interface (netloc_dt_node_t_destruct) is not publicly exposed.
 *
 * \param topology A valid pointer to a topology handle
 * \param nodes A lookup table of the nodes requested
 *              Keys in the table are the \ref netloc_node_t::physical_id's of the \ref netloc_node_t objects
 *              The values are pointers to \ref netloc_node_t objects
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_get_all_nodes(netloc_topology_t topology, netloc_dt_lookup_table_t *nodes);

/**
 * Get only switch nodes in the network topology
 *
 * The user is responsible for calling the lookup table destructor on the nodes
 * table (\ref netloc_lookup_table_destroy).
 * The user should -not- call the netloc_node_t's destructor on the elements in
 * the lookup table. That interface (netloc_dt_node_t_destruct) is not publicly exposed.
 *
 * \param topology A valid pointer to a topology handle
 * \param nodes A lookup table of the nodes requested
 *              Keys in the table are the \ref netloc_node_t::physical_id's of the \ref netloc_node_t objects
 *              The values are pointers to \ref netloc_node_t objects
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_get_all_switch_nodes(netloc_topology_t topology, netloc_dt_lookup_table_t *nodes);

/**
 * Get only host nodes in the network topology
 *
 * The user is responsible for calling the lookup table destructor on the nodes
 * table (\ref netloc_lookup_table_destroy).
 * The user should -not- call the netloc_node_t's destructor on the elements in
 * the lookup table. That interface (netloc_dt_node_t_destruct) is not publicly exposed.
 *
 * \param topology A valid pointer to a topology handle
 * \param nodes A lookup table of the nodes requested
 *              Keys in the table are the \ref netloc_node_t::physical_id's of the \ref netloc_node_t objects
 *              The values are pointers to \ref netloc_node_t objects
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_get_all_host_nodes(netloc_topology_t topology, netloc_dt_lookup_table_t *nodes);

/**
 * Get all of the edges from the specified node in the network topology.
 * There should be one edge for every active port on this node.
 *
 * The user should not free the array, neither its elements.
 *
 * \param topology A valid pointer to a topology handle
 * \param node A valid pointer to a \ref netloc_node_t from which to get the edges.
 * \param num_edges The number of edges in the edges array.
 * \param edges An array of \ref netloc_edge_t objects
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_get_all_edges(netloc_topology_t topology,
                                         netloc_node_t *node,
                                         int *num_edges,
                                         netloc_edge_t ***edges);

/**
 * Access the \ref netloc_node_t pointer given a physical identifier (e.g., MAC address, GUID)
 *
 * The user should -not- call the destructor on the returned value.
 *
 * \param topology A valid pointer to a topology handle
 * \param phy_id The physical identifier to search for (e.g., MAC address, GUID)
 *
 * \returns A pointer to the \ref netloc_node_t with the specified physical identifier
 * \returns NULL if the phy_id is not found.
 */
NETLOC_DECLSPEC netloc_node_t * netloc_get_node_by_physical_id(netloc_topology_t topology, const char * phy_id);

/**
 * Get the "path" from the source to the destination as an ordered array of \ref netloc_edge_t objects
 *
 * The user is responsible for calling free() on the allocated array, but -not- the elements in the array.
 *
 * \warning A large API change is in the works for v1.0 that will change how we represent path data.
 *
 * \param topology A valid pointer to a topology handle
 * \param src_node A valid pointer to the source node
 * \param dst_node A valid pointer to the destination node
 * \param num_edges The number of edges in the path array.
 * \param path An ordered array of \ref netloc_edge_t objects from the source to the destination
 * \param is_logical If the path should represent the logical or the physical path information.
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_get_path(const netloc_topology_t topology,
                                    netloc_node_t *src_node,
                                    netloc_node_t *dst_node,
                                    int *num_edges,
                                    netloc_edge_t ***path,
                                    bool is_logical);


/**********************************************************************
 * Export API Functions
 **********************************************************************/
/**
 * Exports the network topology to a GraphML formatted file.
 *
 * \param topology A valid pointer to a topology handle
 * \param filename The filename to write the data to
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_topology_export_graphml(netloc_topology_t topology, const char * filename);

/**
 * Exports the network topology to a GEXF formatted file.
 *
 * \param topology A valid pointer to a topology handle
 * \param filename The filename to write the data to
 *
 * \returns NETLOC_SUCCESS on success
 * \returns NETLOC_ERROR upon an error.
 */
NETLOC_DECLSPEC int netloc_topology_export_gexf(netloc_topology_t topology, const char * filename);


#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif // _NETLOC_H_
