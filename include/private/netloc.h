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



#define NETLOC_EDGE_UID_START    0
#define NETLOC_EDGE_UID_NULL    -1
#define NETLOC_EDGE_UID_INVALID -2


/**********************************************************************
 *        Lookup table functionality
 **********************************************************************/
/**
 * Lookup table entry
 */
struct netloc_lookup_table_entry_t {
    /** Key */
    const char *key;
    /** Value pointer */
    void *value;
    /** Lookup key */
    unsigned long __key__;
};
typedef struct netloc_lookup_table_entry_t netloc_lookup_table_entry_t;

/**
 * Lookup table
 */
struct netloc_dt_lookup_table {
    /** Table entries array */
    netloc_lookup_table_entry_t **ht_entries;
    /** Number of entries in the lookup table */
    size_t   ht_size;
    /** Number of filled entried in the lookup table */
    size_t   ht_used_size;
    /** Flags */
    unsigned long flags;
};


/**
 * Lookup table iterator
 */
struct netloc_dt_lookup_table_iterator {
    /** A pointer to the lookup table */
    struct netloc_dt_lookup_table *htp;
    /** The current location in the table */
    size_t loc;
    /** Flag if we reached the end */
    bool at_end;
};


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
    int num_nodes;
    netloc_node_t **nodes;

    /** Partition List */
    int num_partitions;
    char **partitions;

    /** Hwloc topology List */
    char **topos;
    int num_topos;

    /** Lookup table for all edge information */
    struct netloc_dt_lookup_table *edges;

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
 * Datatype Support Functions for Lookup Tables
 **********************************************************************/
/**
 * Copy a lookup table and all entries
 *
 * Note that the pointers are copied for each entry. The user is responsible for reference counting.
 *
 * \param from A pointer to the lookup table to duplicate
 * \param to A pointer to the lookup table to duplicate into
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_dt_lookup_table_t_copy(netloc_dt_lookup_table_t from, netloc_dt_lookup_table_t to);


/**********************************************************************
 * Lookup table API Functions
 **********************************************************************/
enum netloc_lookup_table_init_flag_e {
    /* Don't duplicate keys inside the table, assume the given string
     * will remain valid and unchanged during the entire table life.
     */
    NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY = (1UL<<0)
};

/**
 * Initialize the lookup table
 *
 * The lookup table must have been memset'ed to 0 before calling this function.
 *
 * \param table The lookup table to initialize
 * \param size Initial table size (will automaticly expand as necessary)
 * \param flags Flags to tune the table, OR'ed set of netloc_lookup_table_init_flag_e.
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_init(netloc_dt_lookup_table_t table, size_t size, unsigned long flags);

/**
 * Access the -allocated- size of the lookup table
 *
 * \param table A valid pointer to a lookup table
 *
 * Returns
 *   The allocated size of the lookup table
 */
NETLOC_DECLSPEC size_t netloc_lookup_table_size_alloc(netloc_dt_lookup_table_t table);

/**
 * Append an entry to the lookup table
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param value The pointer to associate with this key
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
*/
NETLOC_DECLSPEC int netloc_lookup_table_append(netloc_dt_lookup_table_t ht, const char *key, void *value);

/**
 * Append an entry to the lookup table while specifying the integer key to use
 * (instead of calculating it)
 *
 * Warning: This interface is only used internally at the moment.
 * Warning: In order for this interface to work, you must use all of the
 *          *_with_int() methods when interacting with this loopup table.
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param key_int The unique integer key used to find the data
 * \param value The pointer to associate with this key
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_append_with_int(netloc_dt_lookup_table_t ht, const char *key, unsigned long key_int, void *value);

/**
 * Access an entry to the lookup table while specifying the integer key to use
 * (instead of calculating it)
 *
 * \warning This interface is only used internally at the moment.
 * In order for this interface to work, you must use all of the
 * *_with_int() methods when interacting with this loopup table.
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param key_int The unique integer key used to find the data
 *
 * Returns
 *   NULL if nothing found
 *   The pointer stored from a prior call to netloc_lookup_table_append
 */
NETLOC_DECLSPEC void * netloc_lookup_table_access_with_int(netloc_dt_lookup_table_t ht, const char *key, unsigned long key_int);


/**
 * Replace an entry in the lookup table with the provided value
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param value The pointer to associate with this key
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_replace(netloc_dt_lookup_table_t ht, const char *key, void *value);

/**
 * Replace an entry in the lookup table with the provided value
 * while specifying the integer key to use (instead of calculating it)
 *
 * Warning: This interface is only used internally at the moment.
 * Warning: In order for this interface to work, you must use all of the
 *          *_with_int() methods when interacting with this loopup table.
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param key_int The unique integer key used to find the data
 * \param value The pointer to associate with this key
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_replace_with_int(netloc_dt_lookup_table_t ht, const char *key, unsigned long key_int, void *value);


/**
 * Remove an entry from the lookup table.
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_remove(netloc_dt_lookup_table_t ht, const char *key);

/**
 * Remove an entry from the lookup table.
 * while specifying the integer key to use (instead of calculating it)
 *
 * Warning: This interface is only used internally at the moment.
 * Warning: In order for this interface to work, you must use all of the
 *          *_with_int() methods when interacting with this loopup table.
 *
 * \param ht A valid pointer to a lookup table
 * \param key The key used to find the data
 * \param key_int The unique integer key used to find the data
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR on error
 */
NETLOC_DECLSPEC int netloc_lookup_table_remove_with_int(netloc_dt_lookup_table_t ht, const char *key, unsigned long key_int);

/**
 * Pretty print the lookup table to stdout (Debugging Support)
 *
 * \param ht A valid pointer to a lookup table
 */
NETLOC_DECLSPEC void netloc_lookup_table_pretty_print(netloc_dt_lookup_table_t ht);

/**
 * Get the next key and advance the iterator
 *
 * \param hti A valid pointer to a lookup table iterator
 *
 * Returns
 *   internal lookup key, 0 if error
 */
NETLOC_DECLSPEC unsigned long netloc_lookup_table_iterator_next_key_int(netloc_dt_lookup_table_iterator_t hti);


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

/**
 * JSON Decode the data
 *
 * This will -not- decode the path information
 * User is responsible for calling _destruct on the returned pointer.
 *
 * \param json_node A point to a valid json object representing the node information
 *
 * Returns
 *   A newly allocated node type filled in with the stored information
 */
NETLOC_DECLSPEC netloc_node_t* netloc_dt_node_t_json_decode(netloc_dt_lookup_table_t edge_table, json_t *json_node);

/**
 * JSON Encode the paths in the data structure
 *
 * This -only- encodes the path information specified.
 *
 * \param node A pointer to the node to process
 * \param paths A pointer to the paths data to process
 *
 * Returns
 *   A valid json object representing the paths information for this node
 */
NETLOC_DECLSPEC json_t* netloc_dt_node_t_json_encode_paths(netloc_node_t *node, netloc_dt_lookup_table_t paths);

/**
 * JSON Decode the paths in the data structure
 *
 * This will -only- decode the path information
 * User is responsible for calling _destruct on the returned pointer.
 *
 * \param json_all_paths A point to a valid json object representing the path information for a node
 *
 * Returns
 *   A newly allocated lookup table for the path information stored in the json object
 */
NETLOC_DECLSPEC netloc_dt_lookup_table_t netloc_dt_node_t_json_decode_paths(netloc_dt_lookup_table_t edge_table, json_t *json_all_paths);

/*************************************************/

/**
 * JSON Encode the data
 *
 * \param table A pointer to a lookup table
 * \param (*func) A function to encode the individual values
 *
 * Returns
 *   A valid json object representing the lookup table information
 */
NETLOC_DECLSPEC json_t* netloc_dt_lookup_table_t_json_encode(netloc_dt_lookup_table_t table,
                                                             json_t* (*func)(const char * key, void *value));

/**
 * JSON Decode the data
 *
 * User is responsible for calling _destruct on the returned pointer.
 *
 * \param json_lt A pointer to a valid json object representing the lookup table information
 * \param (*func) A function to decode the individual values
 *
 * Returns
 *   A newly allocated lookup table type filled in with the stored information
 */
NETLOC_DECLSPEC netloc_dt_lookup_table_t netloc_dt_lookup_table_t_json_decode(json_t *json_lt,
                                                                               void * (*func)(const char *key, json_t* json_obj));

/*************************************************/

/**********************************************************************
 *        Expandable list
 **********************************************************************/

typedef struct {
    int size;
    int max_size;
    void **array;
} netloc_explist_t;

netloc_explist_t *netloc_explist_init(int size);
void netloc_explist_add(netloc_explist_t *list, void *elem);
void netloc_explist_cat(netloc_explist_t *list1, netloc_explist_t *list2);
void netloc_explist_set(netloc_explist_t *list, int idx, void *elem);
int netloc_explist_get_size(netloc_explist_t *list);
void *netloc_explist_get(netloc_explist_t *list, int idx);
void netloc_explist_destroy(netloc_explist_t *list);
void **netloc_explist_get_array_and_destroy(netloc_explist_t *list);

/**********************************************************************
 *        Analysis functionality
 **********************************************************************/

typedef struct netloc_analysis_data_t {
    int level;
} netloc_analysis_data;

typedef struct netloc_tree_data_t {
    int num_levels;
    int *num_nodes_by_level;
    netloc_node_t ***nodes_by_level;
    int *num_edges_by_level;
    netloc_node_t ***edges_by_level;
} netloc_tree_data;

typedef struct netloc_topology_analysis_t {
    netloc_topology_t topology;
    netloc_topology_type_t type;
    void *data;
} netloc_topology_analysis;

NETLOC_DECLSPEC int netloc_partition_analyse(netloc_topology_analysis *analysis,
        netloc_topology_t topology, int simpify, char *partition, int levels,
        int hwloc);
NETLOC_DECLSPEC netloc_tree_data *netloc_get_tree_data(netloc_topology_analysis *analysis);

#endif // _NETLOC_PRIVATE_H_
