/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

/** \file
 * \brief Macros to help interaction between libtopology and Linux libnuma.
 *
 * Applications that use both Linux libnuma and libtopology may want to
 * include this file so as to ease conversion between their respective types.
 */

#ifndef TOPOLOGY_LINUX_LIBNUMA_H
#define TOPOLOGY_LINUX_LIBNUMA_H

#include <topology.h>
#include <numa.h>
#include <assert.h>


/** \defgroup topology_linux_libnuma_ulongs Helpers for manipulating Linux libnuma unsigned long masks
 * @{
 */


/** \brief Convert libtopology CPU set \p cpuset into the array of unsigned long \p mask
 *
 * \p mask is the array of unsigned long that will be filled.
 * \p maxnode contains the maximal node number that may be stored in \p mask.
 * \p maxnode will be set to the maximal node number that was found, plus one.
 *
 * This function may be used before calling set_mempolicy, mbind, migrate_pages
 * or any other function that takes an array of unsigned long and a maximal
 * node number as input parameter.
 */
static __inline__ void
topo_cpuset_to_linux_libnuma_ulongs(topo_topology_t topology, const topo_cpuset_t *cpuset,
				    unsigned long *mask, unsigned long *maxnode)
{
  unsigned long outmaxnode = -1;
  topo_obj_t node = NULL;
  unsigned nbnodes = topo_get_type_nbobjs(topology, TOPO_OBJ_NODE);
  int i;

  for(i=0; i<*maxnode/TOPO_BITS_PER_LONG; i++)
    mask[i] = 0;

  if (nbnodes) {
    while ((node = topo_get_next_obj_above_cpuset_by_depth(topology, cpuset, TOPO_OBJ_NODE, node)) != NULL) {
      if (node->os_index >= *maxnode)
	break;
      mask[node->os_index/TOPO_BITS_PER_LONG] |= 1 << (node->os_index % TOPO_BITS_PER_LONG);
      outmaxnode = node->os_index;
    }

  } else {
    /* if no numa, libnuma assumes we have a single node */
    if (!topo_cpuset_iszero(cpuset)) {
      mask[0] = 1;
      outmaxnode = 0;
    }
  }

  *maxnode = outmaxnode+1;
}

/** \brief Convert the array of unsigned long \p mask into libtopology CPU set \p cpuset
 *
 * \p mask is a array of unsigned long that will be read.
 * \p maxnode contains the maximal node number that may be read in \p mask.
 *
 * This function may be used after calling get_mempolicy or any other function
 * that takes an array of unsigned long as output parameter (and possibly
 * a maximal node number as input parameter).
 */
static __inline__ void
topo_cpuset_from_linux_libnuma_ulongs(topo_topology_t topology, topo_cpuset_t *cpuset,
				      const unsigned long *mask, unsigned long maxnode)
{
  topo_obj_t node;
  unsigned depth;
  int i;

  topo_cpuset_zero(cpuset);

  depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
  assert(depth != TOPO_TYPE_DEPTH_MULTIPLE);

  if (depth == TOPO_TYPE_DEPTH_UNKNOWN) {
    /* if no numa, libnuma assumes we have a single node */
    if (mask[0] & 1)
      *cpuset = topo_get_system_obj(topology)->cpuset;

  } else {
    for(i=0; i<maxnode; i++)
      if (mask[i/TOPO_BITS_PER_LONG] & (1 << (i% TOPO_BITS_PER_LONG))) {
	node = topo_get_obj_by_depth(topology, depth, i);
	if (node)
	  topo_cpuset_orset(cpuset, &node->cpuset);
      }
  }
}

/** @} */



/** \defgroup topology_linux_libnuma_bitmask Helpers for manipulating Linux libnuma bitmask
 * @{
 */


/** \brief Convert libtopology CPU set \p cpuset into the returned libnuma bitmask
 *
 * The returned bitmask should later be freed with numa_bitmask_free.
 *
 * This function may be used before calling many numa_ functions
 * that use a struct bitmask as an input parameter.
 */
static __inline__ struct bitmask *
topo_cpuset_to_linux_libnuma_bitmask(topo_topology_t topology, const topo_cpuset_t *cpuset)
{
  struct bitmask *bitmask;
  topo_obj_t node = NULL;
  unsigned nbnodes = topo_get_type_nbobjs(topology, TOPO_OBJ_NODE);

  if (nbnodes) {
    bitmask = numa_bitmask_alloc(nbnodes);
    if (!bitmask)
      return NULL;
    while ((node = topo_get_next_obj_above_cpuset(topology, cpuset, TOPO_OBJ_NODE, node)) != NULL)
      numa_bitmask_setbit(bitmask, node->os_index);

  } else {
    /* if no numa, libnuma assumes we have a single node */
    bitmask = numa_bitmask_alloc(1);
    if (!bitmask)
      return NULL;
    if (!topo_cpuset_iszero(cpuset))
      numa_bitmask_setbit(bitmask, 0);
  }

  return bitmask;
}

/** \brief Convert libnuma bitmask \p bitmask into libtopology CPU set \p cpuset
 *
 * This function may be used after calling many numa_ functions
 * that use a struct bitmask as an output parameter.
 */
static __inline__ void
topo_cpuset_from_linux_libnuma_bitmask(topo_topology_t topology, topo_cpuset_t *cpuset,
				       const struct bitmask *bitmask)
{
  topo_obj_t node;
  unsigned depth;
  int i;

  topo_cpuset_zero(cpuset);

  depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
  assert(depth != TOPO_TYPE_DEPTH_MULTIPLE);

  if (depth == TOPO_TYPE_DEPTH_UNKNOWN) {
    /* if no numa, libnuma assumes we have a single node */
    if (numa_bitmask_isbitset(bitmask, 0))
      *cpuset = topo_get_system_obj(topology)->cpuset;

  } else {
    for(i=0; i<NUMA_NUM_NODES; i++)
      if (numa_bitmask_isbitset(bitmask, i)) {
	node = topo_get_obj_by_depth(topology, depth, i);
	if (node)
	  topo_cpuset_orset(cpuset, &node->cpuset);
      }
  }
}

/** @} */



#ifdef NUMA_VERSION1_COMPATIBILITY
/** \defgroup topology_linux_libnuma_nodemask Helpers for manipulating Linux libnuma nodemask_t
 * @{
 */


/** \brief Convert libtopology CPU set \p cpuset into libnuma nodemask \p nodemask
 *
 * This function may be used before calling some old libnuma functions
 * that use a nodemask_t as an input parameter.
 */
static __inline__ void
topo_cpuset_to_linux_libnuma_nodemask(topo_topology_t topology, const topo_cpuset_t *cpuset,
				      nodemask_t *nodemask)
{
  topo_obj_t node = NULL;
  unsigned nbnodes = topo_get_type_nbobjs(topology, TOPO_OBJ_NODE);

  nodemask_zero(nodemask);
  if (nbnodes) {
    while ((node = topo_get_next_obj_above_cpuset(topology, cpuset, TOPO_OBJ_NODE, node)) != NULL)
      nodemask_set(nodemask, node->os_index);

  } else {
    /* if no numa, libnuma assumes we have a single node */
    if (!topo_cpuset_iszero(cpuset))
      nodemask_set(nodemask, 0);
  }
}

/** \brief Convert libnuma nodemask \p nodemask into libtopology CPU set \p cpuset
 *
 * This function may be used before calling some old libnuma functions
 * that use a nodemask_t as an output parameter.
 */
static __inline__ void
topo_cpuset_from_linux_libnuma_nodemask(topo_topology_t topology, topo_cpuset_t *cpuset,
					const nodemask_t *nodemask)
{
  topo_obj_t node;
  unsigned depth;
  int i;

  topo_cpuset_zero(cpuset);

  depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
  assert(depth != TOPO_TYPE_DEPTH_MULTIPLE);

  if (depth == TOPO_TYPE_DEPTH_UNKNOWN) {
    /* if no numa, libnuma assumes we have a single node */
    if (nodemask_isset(nodemask, 0))
      *cpuset = topo_get_system_obj(topology)->cpuset;

  } else {
    for(i=0; i<NUMA_NUM_NODES; i++)
      if (nodemask_isset(nodemask, i)) {
	node = topo_get_obj_by_depth(topology, depth, i);
	if (node)
	  topo_cpuset_orset(cpuset, &node->cpuset);
      }
  }
}

/** @} */
#endif /* NUMA_VERSION1_COMPATIBILITY */


#endif /* TOPOLOGY_LINUX_NUMA_H */
