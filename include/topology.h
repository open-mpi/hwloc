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
/** \brief Topology context */
typedef struct topo_topology * topo_topology_t;

/** \brief Global information about the topology */
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

/** \brief Type of topology object.
 *
 * Do not rely on any relative ordering of the values.
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
#define TOPO_OBJ_TYPE_MAX (TOPO_OBJ_MISC+1)
typedef enum topo_obj_type_e topo_obj_type_t;

/** \brief Structure of a topology object */
struct topo_obj {
  /* physical information */
  enum topo_obj_type_e type;		/**< \brief Type of object */
  signed physical_index;		/**< \brief OS-provided physical index number */

  union topo_obj_attr_u {
    /** \brief Cache-specific attributes */
    struct topo_cache_attr_u {
      unsigned long memory_kB;		  /**< \brief Size of cache */
      unsigned depth;			  /**< \brief Depth of cache */
    } cache;

    /** \brief Node-specific attributes */
    struct topo_memory_attr_u {
      unsigned long memory_kB;		  /**< \brief Size of memory node */
      unsigned long huge_page_free;	  /**< \brief Number of available huge pages */
    } node;
    struct topo_memory_attr_u machine;	/**< \brief Machine-specific attributes */
    struct topo_memory_attr_u system;	/**< \brief System-specific attributes */
    /** \brief Misc-specific attributes */
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
 * tuned with ::topology_configuration routine
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

/** \defgroup topology_configuration Configure Topology detection
 *
 * These functions should be called between topology_init() and topology_load()
 * to configure how the detection should be performed.
 *
 * @{
 */

/** \brief Ignore an object type. */
extern int topo_topology_ignore_type(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore an object type if it does not bring any structure. */
extern int topo_topology_ignore_type_keep_structure(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore all objects that do not bring any structure. */
extern int topo_topology_ignore_all_keep_structure(topo_topology_t topology);
/** \brief Set OR'ed flags to non-yet-loaded topology. */
enum topo_flags_e {
  TOPO_FLAGS_IGNORE_THREADS = (1<<0),
  TOPO_FLAGS_IGNORE_ADMIN_DISABLE = (1<<1), /* Linux Cpusets, ... */
};
extern int topo_topology_set_flags (topo_topology_t topology, unsigned long flags);
/** \brief Change the file-system root path when building the topology from sysfs/procs. */
extern int topo_topology_set_fsys_root(topo_topology_t topology, const char *fsys_root_path);
/** \brief Enable synthetic topology. */
extern int topo_topology_set_synthetic(struct topo_topology *topology, const char *description);
/** \brief Enable XML-file based topology. */
extern int topo_topology_set_xml(struct topo_topology *topology, const char *xmlpath);


/** @} */


/** \defgroup topology_information Get Some Topology Information
 * @{
 */

/** \brief Get global information about the topology. */
extern int topo_topology_get_info(topo_topology_t topology, struct topo_topology_info *info);

/** \brief Returns the depth of objects of type _type_.
 *
 * If no object of this type is present on the underlying architecture, the
 * function returns the depth of the first "present" object we find uppon \p
 * type.
 */
extern unsigned topo_get_type_depth (topo_topology_t topology, enum topo_obj_type_e type);
#define TOPO_TYPE_DEPTH_UNKNOWN -1
#define TOPO_TYPE_DEPTH_MULTIPLE -2

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
static inline topo_obj_t topo_find_common_ancestor_obj (const topo_obj_t obj1, const topo_obj_t obj2)
{
  topo_obj_t o1 = obj1, o2 = obj2;
  while (o1->level > o2->level)
    o1 = o1->father;
  while (o2->level > o1->level)
    o2 = o2->father;
  while (o1 != o2) {
    o1 = o1->father;
    o2 = o2->father;
  }
  return o1;
}

/** \brief Returns true if _obj_ is inside the subtree beginning
    with \p subtree_root. */
static inline int topo_obj_is_in_subtree (const topo_obj_t obj, const topo_obj_t subtree_root)
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
extern int topo_find_closest_objs (topo_topology_t topology, const topo_obj_t src,
				   topo_obj_t *objs, int max);

/** \brief Find the lowest object covering at least the given cpuset \p set */
extern topo_obj_t topo_find_cpuset_covering_obj (topo_topology_t topology, const topo_cpuset_t *set);

/** \brief Find objects covering exactly a given cpuset \p set */
extern int topo_find_cpuset_objs (topo_topology_t topology, const topo_cpuset_t *set,
				  topo_obj_t *objs, int max);

/** \brief Find the first cache covering a cpuset \p set */
static inline topo_obj_t topo_find_cpuset_covering_cache (topo_topology_t topology, const topo_cpuset_t *set)
{
  topo_obj_t current = topo_find_cpuset_covering_obj(topology, set);
  while (current) {
    if (current->type == TOPO_OBJ_CACHE)
      return current;
    current = current->father;
  }
  return NULL;
}

/** \brief Find the first cache shared between an object and somebody else */
static inline topo_obj_t topo_find_shared_cache_above (topo_topology_t topology, const topo_obj_t obj)
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

/** \brief Among objects at given depth and below a cpuset, return the nth. */
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

/** \brief Among objects at given depth and below a root object, return the nth.
 *
 * If root is \c NULL, use the top-object (system).
 */
static inline topo_obj_t
topo_get_obj_below_by_depth (topo_topology_t topology, const topo_obj_t root,
			     unsigned depth, unsigned index)
{
  topo_obj_t r = root;
  if (!r)
    r = topo_get_system_obj(topology);
  return topo_get_obj_below_cpuset_by_depth(topology, &r->cpuset, depth, index);
}

/** \brief Among objects of given type and below a root object, return the nth.
 *
 * If root is \c NULL, use the top-object (system).
 * If there are multiple depth for given type, return \c NULL and let the caller
 * fallback to topo_get_obj_below_by_depth().
 */
static inline topo_obj_t
topo_get_obj_below_by_type (topo_topology_t topology, const topo_obj_t root,
			    topo_obj_type_t type, unsigned index)
{
  unsigned depth = topo_get_type_depth (topology, type);
  if (depth == TOPO_TYPE_DEPTH_UNKNOWN || depth == TOPO_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return topo_get_obj_below_by_depth(topology, root, depth, index);
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
extern int topo_obj_snprintf(char *string, size_t size,
			     topo_topology_t topology, const topo_obj_t l,
			     const char *indexprefix, int verbose);

/** \brief Stringify the cpuset containing a set of objects.
 *
 * \return how many characters were actually written (not including the ending \\0). */
extern int topo_obj_cpuset_snprintf(char *str, size_t size,
				    size_t nobj, const topo_obj_t const *objs);

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
