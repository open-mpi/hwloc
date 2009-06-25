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
 * \brief High-level libtopology traversal helpers.
 */

#ifndef TOPOLOGY_HELPER_H
#define TOPOLOGY_HELPER_H

#ifndef TOPOLOGY_H
#error Please include the main topology.h instead
#endif



/** \defgroup topology_helper_types Object Type Helpers
 * @{
 */

/** \brief Returns the depth of objects of type \p type or below
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns the depth of the first "present" object typically found
 * inside \p type.
 */
static __inline__ unsigned
topo_get_type_or_below_depth (topo_topology_t topology, topo_obj_type_t type)
{
  unsigned depth = topo_get_type_depth(topology, type);
  unsigned minorder = topo_get_type_order(type);

  if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
    return depth;

  /* find the highest existing level with type order >= */
  for(depth = topo_get_type_depth(topology, TOPO_OBJ_PROC); ; depth--)
    if (topo_get_type_order(topo_get_depth_type(topology, depth)) < minorder)
      return depth+1;

  /* Shouldn't ever happen, as there is always a SYSTEM level with lower order and known depth.  */
  abort();
}

/** \brief Returns the depth of objects of type \p type or above
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns the depth of the first "present" object typically
 * containing \p type.
 */
static __inline__ unsigned
topo_get_type_or_above_depth (topo_topology_t topology, topo_obj_type_t type)
{
  unsigned depth = topo_get_type_depth(topology, type);
  unsigned maxorder = topo_get_type_order(type);

  if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
    return depth;

  /* find the lowest existing level with type order <= */
  for(depth = 0; ; depth++)
    if (topo_get_type_order(topo_get_depth_type(topology, depth)) > maxorder)
      return depth-1;

  /* Shouldn't ever happen, as there is always a PROC level with higher order and known depth.  */
  abort();
}

/** \brief Returns the width of level type \p type
 *
 * If no object for that type exists, 0 is returned.
 * If there are several levels with objects of that type, -1 is returned.
 */
static __inline__ int
topo_get_type_nbobjs (topo_topology_t topology, topo_obj_type_t type)
{
	unsigned depth = topo_get_type_depth(topology, type);
	if (depth == TOPO_TYPE_DEPTH_UNKNOWN)
		return 0;
	if (depth == TOPO_TYPE_DEPTH_MULTIPLE)
		return -1; /* FIXME: agregate nbobjs from different levels? */
	return topo_get_depth_nbobjs(topology, depth);
}

/** @} */



/** \defgroup topology_helper_traversal_basic Basic Traversal Helpers
 * @{
 */

/** \brief Returns the top-object of the topology-tree. Its type is ::TOPO_OBJ_SYSTEM. */
static __inline__ topo_obj_t
topo_get_system_obj (topo_topology_t topology)
{
  return topo_get_obj_by_depth (topology, 0, 0);
}

/** \brief Returns the topology object at index \p index with type \p type
 *
 * If no object for that type exists, \c NULL is returned.
 * If there are several levels with objects of that type, \c NULL is returned
 * and ther caller may fallback to topo_get_obj_by_depth().
 */
static __inline__ topo_obj_t
topo_get_obj (topo_topology_t topology, topo_obj_type_t type, unsigned index)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN)
    return NULL;
  if (depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_obj_by_depth(topology, depth, index);
}

/** \brief Returns the next object at depth \p depth.
 *
 * If \p prev is \c NULL, return the first object at depth \p depth.
 */
static __inline__ topo_obj_t
topo_get_next_obj_by_depth (topo_topology_t topology, unsigned depth, topo_obj_t prev)
{
  if (!prev)
    return topo_get_obj_by_depth (topology, depth, 0);
  if (prev->depth != depth)
    return NULL;
  return prev->next_cousin;
}

/** \brief Returns the next object of type \p type.
 *
 * If \p prev is \c NULL, return the first object at type \p type.
 * If there are multiple or no depth for given type, return \c NULL and let the caller
 * fallback to topo_get_next_obj_by_depth().
 */
static __inline__ topo_obj_t
topo_get_next_obj (topo_topology_t topology, topo_obj_type_t type,
		   topo_obj_t prev)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_next_obj_by_depth (topology, depth, prev);
}

/** \brief Return the next child.
 *
 * If \p prev is \c NULL, return the first child.
 */
static __inline__ topo_obj_t
topo_get_next_child (topo_topology_t topology, topo_obj_t father, topo_obj_t prev)
{
  if (!prev)
    return father->first_child;
  if (prev->father != father)
    return NULL;
  return prev->next_sibling;
}

/** \brief Returns the common father object to objects lvl1 and lvl2 */
static __inline__ topo_obj_t
topo_get_common_ancestor_obj (topo_obj_t obj1, topo_obj_t obj2)
{
  while (obj1->depth > obj2->depth)
    obj1 = obj1->father;
  while (obj2->depth > obj1->depth)
    obj2 = obj2->father;
  while (obj1 != obj2) {
    obj1 = obj1->father;
    obj2 = obj2->father;
  }
  return obj1;
}

/** \brief Returns true if _obj_ is inside the subtree beginning
    with \p subtree_root. */
static __inline__ int
topo_obj_is_in_subtree (topo_obj_t obj, topo_obj_t subtree_root)
{
  return topo_cpuset_isincluded(&obj->cpuset, &subtree_root->cpuset);
}

/** @} */



/** \defgroup topology_helper_find_includeds Finding similar Objects Included in a CPU set
 * @{
 */

/** \brief Return the next object at depth \p depth included in CPU set \p set.
 *
 * If \p prev is \c NULL, return the first object at depth \p depth included in \p set.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object in \p set.
 */
static __inline__ topo_obj_t
topo_get_next_obj_below_cpuset_by_depth (topo_topology_t topology, const topo_cpuset_t *set,
					 unsigned depth, topo_obj_t prev)
{
  topo_obj_t next = topo_get_next_obj_by_depth(topology, depth, prev);
  while (next && !topo_cpuset_isincluded(&next->cpuset, set))
    next = next->next_cousin;
  return next;
}

/** \brief Return the next object of type \p type included in CPU set \p set.
 *
 * If there are multiple or no depth for given type, return \c NULL and let the caller
 * fallback to topo_get_next_obj_below_cpuset_by_depth().
 */
static __inline__ topo_obj_t
topo_get_next_obj_below_cpuset (topo_topology_t topology, const topo_cpuset_t *set,
				topo_obj_type_t type, topo_obj_t prev)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_next_obj_below_cpuset_by_depth(topology, set, depth, prev);
}

/** \brief Return the \p index -th object at depth \p depth included in CPU set \p set.
 */
static __inline__ topo_obj_t
topo_get_obj_below_cpuset_by_depth (topo_topology_t topology, const topo_cpuset_t *set,
				    unsigned depth, unsigned index)
{
  int count = 0;
  topo_obj_t obj = topo_get_obj_by_depth (topology, depth, 0);
  while (obj) {
    if (topo_cpuset_isincluded(&obj->cpuset, set)) {
      if (count == index)
	return obj;
      count++;
    }
    obj = obj->next_cousin;
  }
  return NULL;
}

/** \brief Return the \p index -th object of type \p type included in CPU set \p set.
 *
 * If there are multiple or no depth for given type, return \c NULL and let the caller
 * fallback to topo_get_obj_below_cpuset_by_depth().
 */
static __inline__ topo_obj_t
topo_get_obj_below_cpuset (topo_topology_t topology, const topo_cpuset_t *set,
			   topo_obj_type_t type, unsigned index)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_obj_below_cpuset_by_depth(topology, set, depth, index);
}

/** \brief Return the number of objects at depth \p depth included in CPU set \p set. */
static __inline__ unsigned
topo_get_nbobjs_below_cpuset_by_depth (topo_topology_t topology, const topo_cpuset_t *set,
				       unsigned depth)
{
  topo_obj_t obj = topo_get_obj_by_depth (topology, depth, 0);
  int count = 0;
  while (obj) {
    if (topo_cpuset_isincluded(&obj->cpuset, set))
      count++;
    obj = obj->next_cousin;
  }
  return count;
}

/** \brief Return the number of objects of type \p type included in CPU set \p set.
 *
 * If no object for that type exists below CPU set \p set, 0 is returned.
 * If there are several levels with objects of that type below CPU set \p set, -1 is returned.
 */
static __inline__ int
topo_get_nbobjs_below_cpuset (topo_topology_t topology, const topo_cpuset_t *set,
			      topo_obj_type_t type)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN)
    return 0;
  if (depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return -1; /* FIXME: agregate nbobjs from different levels? */
  return topo_get_nbobjs_below_cpuset_by_depth(topology, set, depth);
}

/** @} */



/** \defgroup topology_helper_find_covering Finding a single Object covering at least CPU set
 * @{
 */

/** \brief Get the child covering at least CPU set \p set.
 *
 * \return \c NULL if no child matches.
 */
static inline topo_obj_t
topo_get_cpuset_covering_child (topo_topology_t topology, const topo_cpuset_t *set,
				topo_obj_t father)
{
  topo_obj_t child = father->first_child;
  while (child) {
    if (topo_cpuset_isincluded(set, &child->cpuset))
      return child;
    child = child->next_sibling;
  }
  return NULL;
}

/** \brief Get the lowest object covering at least CPU set \p set
 *
 * \return \c NULL if no object matches.
 */
static inline topo_obj_t
topo_get_cpuset_covering_obj (topo_topology_t topology, const topo_cpuset_t *set)
{
  struct topo_obj *current = topo_get_system_obj(topology);

  if (!topo_cpuset_isincluded(set, &current->cpuset))
    return NULL;

  while (1) {
    topo_obj_t child = topo_get_cpuset_covering_child(topology, set, current);
    if (!child)
      return current;
    current = child;
  }
}


/** @} */



/** \defgroup topology_helper_find_coverings Finding a set of similar Objects covering at least a CPU set
 * @{
 */

/** \brief Iterate through same-depth objects covering at least CPU set \p set
 *
 * If object \p prev is \c NULL, return the first object at depth \p depth
 * covering at least part of CPU set \p set.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object covering at least another part of \p set.
 */
static __inline__ topo_obj_t
topo_get_next_obj_above_cpuset_by_depth(topo_topology_t topology, const topo_cpuset_t *set,
					unsigned depth, topo_obj_t prev)
{
  topo_obj_t next = topo_get_next_obj_by_depth(topology, depth, prev);
  while (next && !topo_cpuset_intersects(set, &next->cpuset))
    next = next->next_cousin;
  return next;
}

/** \brief Iterate through same-type objects covering at least CPU set \p set
 *
 * If object \p prev is \c NULL, return the first object of type \p type
 * covering at least part of CPU set \p set.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object of type \p type covering at least another part of \p set.
 *
 * If there are no or multiple depths for type \p type, \c NULL is returned.
 * The caller may fallback to topo_get_next_obj_above_cpuset_by_depth()
 * for each depth.
 */
static __inline__ topo_obj_t
topo_get_next_obj_above_cpuset(topo_topology_t topology, const topo_cpuset_t *set,
			       topo_obj_type_t type, topo_obj_t prev)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_next_obj_above_cpuset_by_depth(topology, set, depth, prev);
}

/** @} */



/** \defgroup topology_helper_find_cache Cache-specific Finding Helpers
 * @{
 */

/** \brief Get the first cache covering a cpuset \p set
 *
 * \return \c NULL if no cache matches
 */
static __inline__ topo_obj_t
topo_get_cpuset_covering_cache (topo_topology_t topology, const topo_cpuset_t *set)
{
  topo_obj_t current = topo_get_cpuset_covering_obj(topology, set);
  while (current) {
    if (current->type == TOPO_OBJ_CACHE)
      return current;
    current = current->father;
  }
  return NULL;
}

/** \brief Get the first cache shared between an object and somebody else
 *
 * \return \c NULL if no cache matches
 */
static __inline__ topo_obj_t
topo_get_shared_cache_above (topo_topology_t topology, topo_obj_t obj)
{
  topo_obj_t current = obj->father;
  while (current) {
    if (!topo_cpuset_isequal(&current->cpuset, &obj->cpuset)
        && current->type == TOPO_OBJ_CACHE)
      return current;
    current = current->father;
  }
  return NULL;
}

/** @} */



/** \defgroup topology_helper_traversal Advanced Traversal Helpers
 * @{
 */

/** \brief Get the set of highest objects covering exactly a given cpuset \p set
 *
 * \return the number of objects returned in \p objs.
 */
extern int topo_get_cpuset_objs (topo_topology_t topology, const topo_cpuset_t *set,
				 topo_obj_t * __topo_restrict objs, int max);

/** \brief Do a depth-first traversal of the topology to find and sort
 *
 *  all objects that are at the same depth than \p src.
 *  Report in \p objs up to \p max physically closest ones to \p src.
 *
 *  \return the number of objects returned in \p objs.
 */
/* TODO: rather provide an iterator? Provide a way to know how much should be allocated? By returning the total number of objects instead? */
extern int topo_get_closest_objs (topo_topology_t topology, topo_obj_t src, topo_obj_t * __topo_restrict objs, int max);

/** @} */

/** \defgroup topology_helper_binding Binding Helpers
 * @{
 */

/** \brief Distribute \p n items over the topology under \p root
 *
 * Array \p cpuset will be filled with \p n cpusets distributed linearly over
 * the topology under \p root .
 *
 * This is typically useful when an application wants to distribute \p n
 * threads over a machine, giving each of them as much private cache as
 * possible and keeping them locally in number order.
 *
 * The caller may typicall want to additionally call topo_cpuset_singlify()
 * before binding a thread, so that it doesn't move at all.
 */
static __inline__ void
topo_distribute(topo_topology_t topology, topo_obj_t root, topo_cpuset_t *cpuset, int n)
{
  int i;
  int chunk_size, complete_chunks;
  topo_cpuset_t *cpusetp;

  if (!root->arity || n == 1) {
    /* Got to the bottom, we can't split any more, put everything there.  */
    for (i=0; i<n; i++)
      cpuset[i] = root->cpuset;
    return;
  }

  /* Divide n in root->arity chunks.  */
  chunk_size = (n + root->arity - 1) / root->arity;
  complete_chunks = n % root->arity;
  if (!complete_chunks)
    complete_chunks = root->arity;

  /* Allocate complete chunks first.  */
  for (cpusetp = cpuset, i = 0;
       i < complete_chunks;
       i ++, cpusetp += chunk_size)
    topo_distribute(topology, root->children[i], cpusetp, chunk_size);

  /* Now allocate not-so-complete chunks.  */
  for ( ;
       i < root->arity;
       i++, cpusetp += chunk_size-1)
    topo_distribute(topology, root->children[i], cpusetp, chunk_size-1);
}


#endif /* TOPOLOGY_HELPER_H */
