/*
 * Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2013-2014 Inria.  All rights reserved.
 *
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _NETLOC_MAP_H_
#define _NETLOC_MAP_H_

#include <hwloc/autogen/config.h>
#include <netloc/rename_map.h>

#include <netloc.h>
#include <hwloc.h>

#ifdef __cplusplus
extern "C" {
#elif 0
}
#endif



/** \defgroup netloc_map_api_main Netloc Map API - Main objects.
 * @{
 */

/** A netloc map handle.
 * A map contains servers interconnected by networks.
 */
typedef void * netloc_map_t;

/** A netloc map server handle. */
typedef void * netloc_map_server_t;
/** A netloc map port handle.
 * Servers are interconnected by their ports.
 */
typedef void * netloc_map_port_t;

/** @} */



/** \defgroup netloc_map_api_map Netloc Map API - Building maps.
 * @{
 */

/**
 * Create a map
 *
 * \param map The map object to create.
 *
 * Once created, the map object needs to be attached to hwloc and netloc data directories.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
NETLOC_DECLSPEC int netloc_map_create(netloc_map_t *map);

/**
 * Loading the hwloc data from a directory into a map.
 *
 * \param map The map object to attach the data to.
 * \param data_dir the data directory to read the hwloc information from.
 *
 * Actual failures to read the data will cause an error during netloc_map_build().
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_load_hwloc_data(netloc_map_t map, const char *data_dir);

/**
 * Loading the netloc data from a directory into a map.
 *
 * \param map The map object to attach the data to.
 * \param data_dir the data directory to read the netloc information from.
 *
 * Actual failures to read the data will cause an error during netloc_map_build().
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_load_netloc_data(netloc_map_t map, const char *data_dir);

/**
 * Flags to be passed as a OR'ed set to the netloc_map_build() function
 */
enum netloc_map_build_flags_e {
  NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC = (1<<0) /**< Enable hwloc topology compression if supported. \hideinitializer */
};

/**
 * Build a map that was previously created and where hwloc and netloc data were loaded.
 *
 * Requires the netloc_map_load_hwloc_data() and netloc_map_load_netloc_data()
 * functions have been called on the map object.
 *
 * \param map A netloc map.
 * \param flags Any OR'ed set of ::netloc_map_build_flags_e.
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_build(netloc_map_t map, unsigned long flags);

/**
 * Destroy a map.
 *
 * \param map A netloc map.
 *
 * This function must be called even if netloc_map_build() failed.
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_destroy(netloc_map_t map);

/** @} */



/** \defgroup netloc_map_api_servers_ports Netloc Map API - Manipulating servers and ports
 * @{
 */

/**
 * Returns map ports that are close to a hwloc topology and object.
 *
 * \param map A netloc map.
 *
 * \param htopo A hwloc topology that was previously returned by netloc.
 *
 * \param hobj A optional hwloc object inside the hwloc topology.
 * If \p hobj is \c NULL, all ports of that map server match.
 * If \p hobj is a I/O device, the matching ports that are returned are connected to that device.
 * Otherwise, the matching ports are connected to a I/O device close to \p hobj.
 *
 * \param ports The array where the corresponding map ports will be stored.
 * The caller must be preallocate a number of slots that is given in \p *nrp on input.
 *
 * \param nrp A pointer to the number of ports.
 * On input, specifies how many ports can be stored in the \p ports array.
 * On output, specifies how many were actually stored.
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_hwloc2port(netloc_map_t map,
					  hwloc_topology_t htopo, hwloc_obj_t hobj,
					  netloc_map_port_t *ports, unsigned *nrp);

/**
 * Return the map port corresponding to a network edge and/or node.
 *
 * \param map A netloc map.
 * \param ntopo A network topology.
 * \param nnode A network node where to look for ports. Cannot be \c NULL if nedge is \c NULL.
 * \param nedge A network edge where to look for ports. Cannot be \c NULL if nnode is \c NULL.
 * \param port The corresponding port returned on success.
 *
 * \note On input, one (and only one) of nedge and nnode may be NULL. If both are non-NULL, they should match.
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_netloc2port(netloc_map_t map,
					   netloc_topology_t ntopo, netloc_node_t *nnode, netloc_edge_t *nedge,
					   netloc_map_port_t *port);

/**
 * Return the network node and edge from a port.
 *
 * \param port A port to look for edge and node.
 * \param ntopo A netloc topology returned on success.
 * \param nnode The corresponding network node returned on success, if not \c NULL.
 * \param nedge The corresponding network edge returned on success, if not \c NULL.
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_port2netloc(netloc_map_port_t port,
					   netloc_topology_t *ntopo, netloc_node_t **nnode, netloc_edge_t **nedge);

/**
 * Return the hwloc topology and object from a port.
 *
 * \param port A port to look for hwloc topology and object.
 * \param htopop The corresponding hwloc topology is returned on success.
 * \param hobjp The corresponding hwloc object is returned on success, if not \c NULL.
 *
 * A reference will be taken on the returned hwloc topology, it should be released later with netloc_map_put_hwloc().
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_port2hwloc(netloc_map_port_t port,
					  hwloc_topology_t *htopop, hwloc_obj_t *hobjp);

/**
 * Return the hwloc topology from a map server.
 *
 * \param server A map server.
 * \param topology The corresponding hwloc topology is returned on success.
 *
 * A reference will be taken on the returned hwloc topology, it should be released later with netloc_map_put_hwloc().
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_server2hwloc(netloc_map_server_t server, hwloc_topology_t *topology);

/**
 * Return a map server from a hwloc topology.
 *
 * \param map A netloc map.
 *
 * \param topology A hwloc topology.
 * It must have been previously obtained from this netloc map.
 * It cannot be another topology loaded by another piece of software.
 *
 * \param server The corresponding map server returned on success.
 *
 * \note Server should not be freed by the caller.
 *
 * \note Another way to find a server is to extract the hostname from a hwloc topology
 * with hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "HostName") and
 * pass it to netloc_map_name2server().
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_hwloc2server(netloc_map_t map, hwloc_topology_t topology, netloc_map_server_t *server);

/**
 * Release a hwloc topology pointer that we got above.
 *
 * \param map A netloc map.
 *
 * \param topology A hwloc topology previously obtained with netloc_map_port2hwloc()
 * or netloc_map_server2hwloc().
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_put_hwloc(netloc_map_t map, hwloc_topology_t topology);

/* FIXME: uniformize the following get() calls:
 * - get_subnets: the caller should free the array, not its contents (no internal array exists).
 * - get_servers: the caller should alloc/free the array, not its contents (no internal array exists).
 *   this one may be huge, but it's only useful for debugging.
 * - get_ports: the caller should not alloc/free anything (we return a copy of the internal array).
 */

/**
 * Get an array of subnets existing in a netloc map.
 *
 * \param map A netloc map.
 * \param nr The number of network topologies returned in \p *topos on success.
 * \param topos The array of network topologies returned on success.
 *
 * \note the caller should free the array, not its contents.
 *
 * \returns 0 on success
 * \return -1 on error
 */
NETLOC_DECLSPEC int netloc_map_get_subnets(netloc_map_t map, unsigned *nr, netloc_topology_t **topos);

/**
 * Get the number of servers.
 *
 * \param map A netloc map.
 *
 * \returns The number of servers in the map.
 */
NETLOC_DECLSPEC int netloc_map_get_nbservers(netloc_map_t map);

/**
 * Fill the input array with a range of servers.
 *
 * \param map A netloc map.
 * \param first The index of the first server to return.
 * \param nr The number of servers to return.
 * \param servers A preallocated array that is filled with servers on success.
 *
 * \note Servers must be allocated (and freed) by the caller.
 *
 * \note This function is not performance-optimized, it may be slow when first is high.
 *
 * \returns 0 on success
 * \return -1 on error, for instance if \p first and/or \p nr is too high.
 */
NETLOC_DECLSPEC int netloc_map_get_servers(netloc_map_t map,
					   unsigned first, unsigned nr,
					   netloc_map_server_t servers[]);

/**
 * Return the map ports from the server.
 *
 * \param server A map server.
 * \param nr The number of ports returned in \p ports on success.
 * \param ports The array of map server ports returned on success.
 *
 * \note The caller should not free the array.
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_get_server_ports(netloc_map_server_t server,
						unsigned *nr, netloc_map_port_t **ports);

/**
 * Return the map server from a port.
 *
 * \param port A map server port.
 * \param server The corresponding map server returned on success.
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_port2server(netloc_map_port_t port,
					   netloc_map_server_t *server);

/**
 * Return the map of a server
 *
 * \param server A server object
 * \param map The corresponding map which the server is part of.
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_server2map(netloc_map_server_t server, netloc_map_t *map);

/**
 * Return the name of a server.
 *
 * \param server A server object.
 * \param name The name associated with that server.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
NETLOC_DECLSPEC int netloc_map_server2name(netloc_map_server_t server, const char **name);

/**
 * Return the map server object from a name.
 *
 * \param map A netloc map.
 * \param name The name of the server.
 * \param server The corresponding server object returned on success.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
NETLOC_DECLSPEC int netloc_map_name2server(netloc_map_t map,
					   const char *name, netloc_map_server_t *server);

/** @} */



/** \defgroup netloc_map_api_paths Netloc Map API - Finding paths within a map
 * @{
 */

/** A netloc map edge. */
struct netloc_map_edge_s {
  /** A netloc map edge type. */
  enum netloc_map_edge_type_e {
    NETLOC_MAP_EDGE_TYPE_NETLOC, /**< The edge is a regular network edge. */
    NETLOC_MAP_EDGE_TYPE_HWLOC_PARENT, /**< The edge is a hwloc edge from child to parent. */
    NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL, /**< The edhe is a horizontal hwloc edge. */
    NETLOC_MAP_EDGE_TYPE_HWLOC_CHILD, /**< The edge is a hwloc edge from parent to child. */
    NETLOC_MAP_EDGE_TYPE_HWLOC_PCI /**< The edge is a hwloc edge between a PCI and a regular object. */
  } type;
  union {
    struct {
      /** A regular network edge. */
      netloc_edge_t *edge;
      /** The netloc topology corresponding to the edge. */
      netloc_topology_t topology;
    } netloc;
    struct {
      /** The source object of a hwloc edge. */
      hwloc_obj_t src_obj;
      /** The target object of a hwloc edge. */
      hwloc_obj_t dest_obj;
      /* The weigth of the hwloc edge.
       *
       * 1 if there's a direct connection between them.
       * Otherwise the amount of edges between them on the shortest path across the hwloc tree.
       * Usually latency increases with this weigth while bandwidth decreases.
       * For PCI, the weight usually includes an edge from a NUMA node to a hostbridge,
       * some edges between PCI bridges/devices, and an edge from a PCI device to a OS device.
       */
      unsigned weight;
    } hwloc;
  };
};

/** Flags to be given as a OR'ed set to netloc_map_paths_build().
 *
 * \note By default only horizontal hwloc edges are reported, for instance cross-NUMA links.
 */
enum netloc_map_paths_flag_e {
  NETLOC_MAP_PATHS_FLAG_IO = (1UL << 0), /**< Want edges between I/O objects such as PCI NICs and normal hwloc objects */
  NETLOC_MAP_PATHS_FLAG_VERTICAL = (1UL << 1) /**< Want edges between normal hwloc object child and parent, for instance from a core to a NUMA node */
};

/** A netloc map path handle. */
typedef void * netloc_map_paths_t;

/**
 * Build the list of netloc map paths between two hwloc objects in two hwloc topologies.
 *
 * \param map A netloc map.
 * \param srctopo The hwloc topology of the source server.
 * \param srcobj The source hwloc object within the source topology.
 * \param dsttopo The hwloc topology of the destination server.
 * \param dstobj The destination hwloc object within the destination topology.
 * \param flags A OR'ed set of ::netloc_map_paths_flag_e.
 * \param paths The paths handle returned on success. It must be freed with netloc_map_paths_destroy() after use.
 * \param nr The number of paths contained in the \p paths handle that is returned on success.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
/* FIXME: add an optional subnet */
/* FIXME: make srcobj/dstobj optional. paths would only contain network edges. */
NETLOC_DECLSPEC int netloc_map_paths_build(netloc_map_t map,
					   hwloc_topology_t srctopo, hwloc_obj_t srcobj,
					   hwloc_topology_t dsttopo, hwloc_obj_t dstobj,
					   unsigned long flags,
					   netloc_map_paths_t *paths, unsigned *nr);

/**
 * Get a single path from a previously built netloc map paths handle.
 *
 * \param paths A paths handle previously returned by netloc_map_paths_build().
 *
 * \param idx The index of the path to return.
 *
 * \param edges The array of map edges returned on success.
 * It must not be modified or freed by the caller.
 * It is only valid until netloc_map_paths_destroy() is called.

 * \param nr_edges The number of edges returned in the \p edges array on success.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
/* FIXME: also return a subnet pointer.
 * would be NULL if we ever have path across multiple subnets with gateways */
NETLOC_DECLSPEC int netloc_map_paths_get(netloc_map_paths_t paths, unsigned idx,
					 struct netloc_map_edge_s **edges, unsigned *nr_edges);

/* FIXME: get the distance (minimal path length, using PCI/NUMA/Eth/IB values */

/**
 * Destroy a previously built netloc map paths handle.
 *
 * \param paths A paths handle previously returned by netloc_map_paths_build().
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_paths_destroy(netloc_map_paths_t paths);

/** @} */



/** \defgroup netloc_map_api_misc Netloc Map API - Misc
 * @{
 */

/**
 * Find the neighbors of the specified node out to a given depth in the network.
 *
 * \todo Brice FIXME: get neighbor nodes at a given distance, within any or a single subnet
 * \todo Brice FIXME: get neighbor nodes with enough cores, within any or a single subnet
 * \todo Brice This interface is temporary, for debugging
 *
 * \param map A map object.
 * \param hostname The hostname of the node to start from.
 * \param depth The depth into the network to search.
 *
 * \returns 0 on success
 * \returns -1 on error
 */
NETLOC_DECLSPEC int netloc_map_find_neighbors(netloc_map_t map,
					       const char *hostname, unsigned depth);

/**
 * Display the map to stdout (for debugging purposes only).
 *
 * \param map A map object.
 *
 * \returns 0 on success
 */
NETLOC_DECLSPEC int netloc_map_dump(netloc_map_t map);

#ifdef __cplusplus
}
#endif

/** @} */



/*
 * Ideas
 *
 * do users come with a predefined list of nodes (1), or are we going to tell them which nodes to use (2) ?
 *
 * If (1), one may want
 * * all distances/routes between all pairs of nodes within my set
 *   - with an algorithm that is faster than computing for each pair independently
 * * there could be a call computing everything and then a handle to retrieve the distance/route
 *   of a given pair?
 *
 * If (2), one  may want
 * * a set of nodes that are very close to each other
 *   - with or without a source node for this small neighborhood?
 *
 * For all of them, we may want variants with nodes only, and with cores
 * * give me 8 nodes or give enough nodes for 56 cores
 *
 * Distances can be expressed as
 * 3) a number of network hops, with optional link attributes
 * 4) a number of intra-node links
 *   On current architecture, only the number of inter-NUMA links matters.
 *   If the network ever goes to QPI without PCI, we may have to specify whether
 *   there are PCI bridges on the path. So keep the model flexible to add new types of links.
 *   If PCIe ever becomes a networks, we may to specify whether there is
 *
 * (4) may be negligible against (3), so it's not clear  whether we'll want a
 * full distance description (such as 1x NUMA + 1x PCI bridge + 2x network links.
 * (4) will be useful once you chose the nodes, and now need some cores on these,
 * which means we need a distance from cores to a subnet?
 */

#endif /* _NETLOC_MAP_H_ */
