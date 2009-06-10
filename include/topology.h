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
 * \brief The libtopology API.
 */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <stdio.h>


/*
 * Cpuset bitmask definitions
 */

#include <topology/cpuset.h>



/** \defgroup topology_info Topology and Topology Info
 * @{
 */

struct topo_topology;
/** \brief Topology context
 *
 * To be initialized with topo_topology_init() and built with topo_topology_load().
 */
typedef struct topo_topology * topo_topology_t;

/** \brief Global information about a Topology context,
 *
 * To be filled with topo_topology_get_info().
 */
struct topo_topology_info {
  /** \brief topology size */
  unsigned depth;

  /** \brief machine specifics */
  char *dmi_board_vendor;
  char *dmi_board_name;

  /** \brief size of huge pages */
  unsigned long huge_page_size_kB;

  /** \brief set if the topology is different from the actual underlying machine */
  int is_fake;
};


/** @} */

/** \defgroup topology_objects Topology Objects
 * @{
 */

/** \brief Type of topology Object.
 *
 * Do not rely on the ordering of the values as new ones may be defined in the
 * future!  Use the value returned by topo_get_obj_type_order() instead.
 */
enum topo_obj_type_e {
  TOPO_OBJ_SYSTEM,	/**< \brief Whole system (may be a cluster of machines) */
  TOPO_OBJ_MACHINE,	/**< \brief Machine */
  TOPO_OBJ_NODE,	/**< \brief NUMA node */
  TOPO_OBJ_SOCKET,	/**< \brief Socket, physical package, or chip */
  TOPO_OBJ_CACHE,	/**< \brief Cache */
  TOPO_OBJ_CORE,	/**< \brief Core */
  TOPO_OBJ_PROC,	/**< \brief (Logical) Processor (e.g. a thread in an SMT core) */

  TOPO_OBJ_MISC,	/**< \brief Miscellaneous object that may be needed under special circumstances (like arbitrary OS aggregation)  */
};
typedef enum topo_obj_type_e topo_obj_type_t;
/** \brief Maximal value of an Object Type */
#define TOPO_OBJ_TYPE_MAX (TOPO_OBJ_MISC+1)

/** \brief Convert an object type into a number that permits to compare them
 *
 * Types shouldn't be compared as they are, since newer ones may be added in
 * the future.  This function returns an integer value that can be used
 * instead.
 *
 * \note topo_get_obj_type_order(TOPO_OBJ_SYSTEM) will always be the lowest
 * value, and topo_get_obj_type_order(TOPO_OBJ_PROC) will always be the highest
 * value.
 */
int topo_get_obj_type_order(topo_obj_type_t type);

/** \brief converse of topo_get_obj_type_oder()
 *
 * This is the converse of topo_get_obj_type_order().
 */
topo_obj_type_t topo_get_obj_order_type(int order);

/** \brief Structure of a topology Object */
struct topo_obj {
  /* physical information */
  enum topo_obj_type_e type;		/**< \brief Type of object */
  signed physical_index;		/**< \brief OS-provided physical index number */

  /** \brief Object-specific Attributes */
  union topo_obj_attr_u {
    /** \brief Cache-specific Object Attributes */
    struct topo_cache_attr_u {
      unsigned long memory_kB;		  /**< \brief Size of cache */
      unsigned depth;			  /**< \brief Depth of cache */
    } cache;

    /** \brief Node-specific Object Attributes */
    struct topo_memory_attr_u {
      unsigned long memory_kB;		  /**< \brief Size of memory node */
      unsigned long huge_page_free;	  /**< \brief Number of available huge pages */
    } node;
    struct topo_memory_attr_u machine;	/**< \brief Machine-specific Object Attributes */
    struct topo_memory_attr_u system;	/**< \brief System-specific Object Attributes */
    /** \brief Misc-specific Object Attributes */
    struct topo_misc_attr_u {
      unsigned depth;			  /**< \brief Depth of misc object */
    } misc;
  } attr;				  /**< \brief Object type-specific attributes */

  /* global position */
  unsigned level;			/**< \brief Vertical index in the hierarchy */
  unsigned number;			/**< \brief Horizontal index in the whole list of similar objects */

  /* father */
  struct topo_obj *father;		/**< \brief Father, \c NULL if root (system object) */
  unsigned index;			/**< \brief Index in father's \c children[] array */
  struct topo_obj *next_sibling;	/**< \brief Next object below the same father*/
  struct topo_obj *prev_sibling;	/**< \brief Previous object below the same father */

  /* children */
  unsigned arity;			/**< \brief Number of children */
  struct topo_obj **children;		/**< \brief Children, \c children[0 .. arity -1] */
  struct topo_obj *first_child;		/**< \brief First child */
  struct topo_obj *last_child;		/**< \brief Last child */

  /* cousins */
  struct topo_obj *next_cousin;		/**< \brief Next object of same type */
  struct topo_obj *prev_cousin;		/**< \brief Previous object of same type */

  /* misc */
  void *userdata;			/**< \brief Application-given private data pointer, initialize to \c NULL, use it as you wish */

  /* cpuset */
  topo_cpuset_t cpuset;			/**< \brief CPUs covered by this object */
};
typedef struct topo_obj * topo_obj_t;


/** @} */

/** \defgroup topology_creation Create and Destroy Topologies
 * @{
 */

/** \brief Allocate a topology context.
 *
 * \param[out] topologyp is assigned a pointer to the new allocated context.
 *
 * \return 0 on success, -1 on error.
 */
extern int topo_topology_init (topo_topology_t *topologyp);
/** \brief Build the actual topology
 *
 * Build the actual topology once initialized with topo_topology_init() and
 * tuned with ::topology_configuration routine.
 * No other routine may be called earlier using this topology context.
 *
 * \param topology is the topology to be loaded with objects.
 *
 * \return 0 on success, -1 on error.
 *
 * \sa topology_configuration
 */
extern int topo_topology_load(topo_topology_t topology);
/** \brief Terminate and free a topology context
 *
 * \param topology is the topology to be freed
 */
extern void topo_topology_destroy (topo_topology_t topology);
/** \brief Run internal checks on a topology structure
 *
 * \param topology is the topology to be checked
 */
extern void topo_topology_check(topo_topology_t topology);

/** @} */

/** \defgroup topology_configuration Configure Topology Detection
 *
 * These functions should be called between topology_init() and topology_load()
 * to configure how the detection should be performed.
 *
 * @{
 */

/** \brief Ignore an object type.
 *
 * Ignore all objects from the given type.
 * The top-level type TOPO_OBJ_SYSTEM and bottom-level type TOPO_OBJ_PROC may
 * not be ignored.
 */
extern int topo_topology_ignore_type(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore an object type if it does not bring any structure.
 *
 * Ignore all objects from the given type as long as they do not bring any structure:
 * Each ignored object should have a single children or be the only child of its father.
 * The top-level type TOPO_OBJ_SYSTEM and bottom-level type TOPO_OBJ_PROC may
 * not be ignored.
 */
extern int topo_topology_ignore_type_keep_structure(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore all objects that do not bring any structure.
 *
 * Ignore all objects that do not bring any structure: 
 * Each ignored object should have a single children or be the only child of its father.
 */
extern int topo_topology_ignore_all_keep_structure(topo_topology_t topology);
/** \brief Flags to be set onto a topology context before load.
 *
 * Flags should be given to topo_topology_set_flags().
 */
enum topo_flags_e {
  /* \brief Detect the whole system, ignore reservations that may have been setup by the administrator.
   *
   * Gather all resources, even if some were disabled by the administrator.
   * For instance, ignore Linux Cpusets and gather all processors and memory nodes.
   */
  TOPO_FLAGS_WHOLE_SYSTEM = (1<<1),
};
/** \brief Set OR'ed flags to non-yet-loaded topology.
 *
 * Set a OR'ed set of topo_flags_e onto a topology that was not yet loaded.
 */
extern int topo_topology_set_flags (topo_topology_t topology, unsigned long flags);
/** \brief Change the file-system root path when building the topology from sysfs/procfs.
 *
 * On Linux system, use sysfs and procfs files as if they were mounted on the given
 * \p fsys_root_path instead of the main file-system root.
 * Not using the main file-system root causes the is_fake field of the topo_topology_info
 * structure to be set.
 */
extern int topo_topology_set_fsys_root(topo_topology_t __topo_restrict topology, const char * __topo_restrict fsys_root_path);
/** \brief Enable synthetic topology.
 *
 * Gather topology information from the given \p description
 * which should be a comma separated string of numbers describing
 * the arity of each level.
 * Each number may be prefixed with a type and a colon to enforce the type
 * of a level.
 */
extern int topo_topology_set_synthetic(topo_topology_t __topo_restrict topology, const char * __topo_restrict description);
/** \brief Enable XML-file based topology.
 *
 * Gather topology information the XML file given at \p xmlpath.
 * This file may have been generated earlier with lstopo file.xml.
 */
extern int topo_topology_set_xml(topo_topology_t __topo_restrict topology, const char * __topo_restrict xmlpath);


/** @} */


/** \defgroup topology_information Get some Topology Information
 * @{
 */

/** \brief Get global information about the topology.
 *
 * Retrieve global information about a loaded topology context.
 */
extern int topo_topology_get_info(topo_topology_t  __topo_restrict topology, struct topo_topology_info * __topo_restrict info);

/** \brief Returns the depth of objects of type \p type.
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns TOPO_TYPE_DEPTH_UNKNOWN.
 */
extern unsigned topo_get_type_depth (topo_topology_t topology, enum topo_obj_type_e type);
#define TOPO_TYPE_DEPTH_UNKNOWN -1 /**< \brief No object of given type exists in the topology. */
#define TOPO_TYPE_DEPTH_MULTIPLE -2 /**< \brief Objects of given type exist at different depth in the topology. */

/** \brief Returns the depth of objects of type \p type or below
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns the depth of the first "present" object typically found
 * inside \p type.
 */
static __inline__ unsigned topo_get_type_or_below_depth (topo_topology_t topology, enum topo_obj_type_e type)
{
  unsigned depth = topo_get_type_depth(topology, type);

  if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
    return depth;

  int order = topo_get_obj_type_order(type);
  int max_order = topo_get_obj_type_order(TOPO_OBJ_PROC);

  while (++order <= max_order) {
    type = topo_get_obj_order_type(order);
    depth = topo_get_type_depth(topology, type);
    if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
      return depth;
  }
  /* Shouldn't ever happen, as there is always a PROC level.  */
  abort();
}

/** \brief Returns the depth of objects of type \p type or above
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns the depth of the first "present" object typically
 * containing \p type.
 */
static __inline__ unsigned topo_get_type_or_above_depth (topo_topology_t topology, enum topo_obj_type_e type)
{
  unsigned depth = topo_get_type_depth(topology, type);

  if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
    return depth;

  int order = topo_get_obj_type_order(type);
  int min_order = topo_get_obj_type_order(TOPO_OBJ_SYSTEM);

  while (++order >= min_order) {
    type = topo_get_obj_order_type(order);
    depth = topo_get_type_depth(topology, type);
    if (depth != TOPO_TYPE_DEPTH_UNKNOWN)
      return depth;
  }
  /* Shouldn't ever happen, as there is always a SYSTEM level.  */
  abort();
}

/** \brief Returns the type of objects at depth \p depth. */
extern enum topo_obj_type_e topo_get_depth_type (topo_topology_t topology, unsigned depth);

/** \brief Returns the width of level at depth \p depth */
extern unsigned topo_get_depth_nbobjs (topo_topology_t topology, unsigned depth);

/** \brief Returns the width of level type \p type */
static inline unsigned topo_get_type_nbobjs (topo_topology_t topology, enum topo_obj_type_e type)
{
	unsigned depth = topo_get_type_depth(topology, type);
	if (depth == TOPO_TYPE_DEPTH_UNKNOWN)
		return 0;
	if (depth == TOPO_TYPE_DEPTH_MULTIPLE)
		return -1; /* FIXME: agregate nbobjs from different levels? */
	return topo_get_depth_nbobjs(topology, depth);
}

/** @} */


/** \defgroup topology_traversal Topology Traversal
 * @{
 */

/** \brief Returns the topology object at index \p index from depth \p depth */
extern topo_obj_t topo_get_obj (topo_topology_t topology, unsigned depth, unsigned index);

/** \brief Returns the top-object of the topology-tree. Its type is ::TOPO_OBJ_SYSTEM. */
static inline topo_obj_t topo_get_system_obj (topo_topology_t topology)
{
  return topo_get_obj(topology, 0, 0);
}

/** \brief Returns the common father object to objects lvl1 and lvl2 */
static inline topo_obj_t topo_get_common_ancestor_obj (topo_obj_t obj1, topo_obj_t obj2)
{
  while (obj1->level > obj2->level)
    obj1 = obj1->father;
  while (obj2->level > obj1->level)
    obj2 = obj2->father;
  while (obj1 != obj2) {
    obj1 = obj1->father;
    obj2 = obj2->father;
  }
  return obj1;
}

/** \brief Returns true if _obj_ is inside the subtree beginning
    with \p subtree_root. */
static inline int topo_obj_is_in_subtree (topo_obj_t obj, topo_obj_t subtree_root)
{
  return topo_cpuset_isincluded(&obj->cpuset, &subtree_root->cpuset);
}

/** \brief Do a depth-first traversal of the topology to find and sort
 *
 *  all objects that are at the same depth than \p src.
 *  Report in \p objs up to \p max physically closest ones to \p src.
 *
 *  \return the actual number of objects that were found.
 */
/* TODO: rather provide an iterator? Provide a way to know how much should be allocated? */
extern int topo_get_closest_objs (topo_topology_t topology, topo_obj_t src, topo_obj_t * __topo_restrict objs, int max);

/** \brief Get the lowest object covering at least the given cpuset \p set */
extern topo_obj_t topo_get_cpuset_covering_obj (topo_topology_t topology, const topo_cpuset_t *set);

/** \brief Get objects covering exactly a given cpuset \p set */
extern int topo_get_cpuset_objs (topo_topology_t topology, const topo_cpuset_t *set,
				 topo_obj_t * __topo_restrict objs, int max);

/** \brief Get the first cache covering a cpuset \p set */
static inline topo_obj_t topo_get_cpuset_covering_cache (topo_topology_t topology, const topo_cpuset_t *set)
{
  topo_obj_t current = topo_get_cpuset_covering_obj(topology, set);
  while (current) {
    if (current->type == TOPO_OBJ_CACHE)
      return current;
    current = current->father;
  }
  return NULL;
}

/** \brief Get the first cache shared between an object and somebody else */
static inline topo_obj_t topo_get_shared_cache_above (topo_topology_t topology, topo_obj_t obj)
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

/** \brief Iterate through objects above CPU set \p set
 *
 * If object \p prev is \c NULL, return the first object at depth \p depth
 * covering part of CPU set \p set.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object covering another part of \p set.
 */
static inline topo_obj_t topo_get_next_obj_above_cpuset_by_depth(topo_topology_t topology,
								 const topo_cpuset_t *set,
								 unsigned depth,
								 topo_obj_t prev)
{
  topo_obj_t next;
  if (prev) {
    if (prev->level != depth)
      return NULL;
    next = prev->next_cousin;
  } else {
    next = topo_get_obj (topology, depth, 0);
  }

  while (next && !topo_cpuset_intersects(set, &next->cpuset))
    next = next->next_cousin;
  return next;
}

/** \brief Iterate through same-type objects covering CPU set \p set
 *
 * If object \p prev is \c NULL, return the first object of type \p type
 * covering part of CPU set \p set.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object of type \p type covering another part of \p set.
 *
 * If there are no or multiple depths for type \p type, \c NULL is returned.
 * The caller may fallback to topo_get_next_obj_above_cpuset_by_depth()
 * for each depth.
 */
static inline topo_obj_t topo_get_next_obj_above_cpuset(topo_topology_t topology,
							const topo_cpuset_t *set,
							topo_obj_type_t type,
							topo_obj_t prev)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_next_obj_above_cpuset_by_depth(topology, set, depth, prev);
}

/** \brief Return the next object at depth \p depth below CPU set \p set.
 *
 * If \p prev is \c NULL, return the first object at depth \p depth below \p cpuset.
 * The next invokation should pass the previous return value in \p prev so as
 * to obtain the next object below \p set.
 */
static inline topo_obj_t
topo_get_next_obj_below_cpuset_by_depth (topo_topology_t topology, const topo_cpuset_t *set,
					 unsigned depth, topo_obj_t prev)
{
  topo_obj_t next;
  if (prev) {
    if (prev->level != depth)
      return NULL;
    next = prev->next_cousin;
  } else {
    next = topo_get_obj (topology, depth, 0);
  }

  while (next && !topo_cpuset_intersects(set, &next->cpuset))
    next = next->next_cousin;
  return next;
}

/** \brief Return the next object of type \p type below CPU set \p set.
 *
 * If there are multiple or no depth for given type, return \c NULL and let the caller
 * fallback to topo_get_next_obj_below_cpuset_by_depth().
 */
static inline topo_obj_t
topo_get_next_obj_below_cpuset (topo_topology_t topology, const topo_cpuset_t *set,
				topo_obj_type_t type, topo_obj_t prev)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_next_obj_below_cpuset_by_depth(topology, set, depth, prev);
}

/** \brief Return the \p index -th object at depth \p depth below CPU set \p set.
 */
static inline topo_obj_t
topo_get_obj_below_cpuset_by_depth (topo_topology_t topology, const topo_cpuset_t *set,
				    unsigned depth, unsigned index)
{
  int count = 0;
  topo_obj_t obj = topo_get_obj(topology, depth, 0);
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

/** \brief Return the \p index -th object of type \p type below CPU set \p set.
 *
 * If there are multiple or no depth for given type, return \c NULL and let the caller
 * fallback to topo_get_obj_below_cpuset_by_depth().
 */
static inline topo_obj_t
topo_get_obj_below_cpuset (topo_topology_t topology, const topo_cpuset_t *set,
			   topo_obj_type_t type, unsigned index)
{
  unsigned depth = topo_get_type_depth(topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_obj_below_cpuset_by_depth(topology, set, depth, index);
}

/** @} */

/** \defgroup topology_conversion Object/String Conversion
 * @{
 */

/** \brief Return a stringified topology object type */
extern const char * topo_obj_type_string (enum topo_obj_type_e l);

/** \brief Return an object type from the string */
extern topo_obj_type_t topo_obj_type_of_string (const char * string);

/** \brief Stringify a given topology object into a human-readable form.
 *
 * \return how many characters were actually written (not including the ending \\0). */
extern int topo_obj_snprintf(char * __topo_restrict string, size_t size,
			     topo_topology_t topology, topo_obj_t obj,
			     const char * __topo_restrict indexprefix, int verbose);

/** \brief Stringify the cpuset containing a set of objects.
 *
 * \return how many characters were actually written (not including the ending \\0). */
extern int topo_obj_cpuset_snprintf(char * __topo_restrict str, size_t size, size_t nobj, const topo_obj_t * __topo_restrict objs);

/** @} */


/** \defgroup topology_binding Binding
 * @{
 */

/** \brief Bind current process on cpus given in cpuset \p set
 *
 * You might want to call topo_cpuset_singlify() first so that
 * a single CPU remains in the set. This way, the process has no
 * of migrating between different CPUs.
 */
extern int topo_set_cpubind(const topo_cpuset_t *set);

/** @} */


#endif /* TOPOLOGY_H */
