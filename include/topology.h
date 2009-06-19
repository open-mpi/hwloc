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
 *
 * See topology/cpuset.h for CPU set specific macros
 * See topology/helper.h for high-level topology traversal helpers
 */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <sys/types.h>
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

  /** \brief set if the topology is different from the actual underlying machine */
  int is_fake;
};


/** @} */

/** \defgroup topology_types Topology Object Types
 * @{
 */

/** \brief Type of topology Object.
 *
 * Do not rely on the ordering of the values as new ones may be defined in the
 * future!  Use the value returned by topo_get_type_order() instead.
 */
typedef enum {
  TOPO_OBJ_SYSTEM,	/**< \brief Whole system (may be a cluster of machines).
  			  * The whole system that is accessible to libtopology.
			  * That may comprise several machines in SSI systems
			  * like Kerrighed.
			  */
  TOPO_OBJ_MACHINE,	/**< \brief Machine.
			  * A set of processors and memory with cache
			  * coherency.
			  */
  TOPO_OBJ_NODE,	/**< \brief NUMA node.
			  * A set of processors around memory which the
			  * processors can directly access.
			  */
  TOPO_OBJ_SOCKET,	/**< \brief Socket, physical package, or chip.
			  * In the physical meaning, i.e. that you can add
			  * or remove physically.
			  */
  TOPO_OBJ_CACHE,	/**< \brief Data cache.
			  * Can be L1, L2, L3, ...
			  */
  TOPO_OBJ_CORE,	/**< \brief Core.
			  * A computation unit (may be shared by several
			  * logical processors).
			  */
  TOPO_OBJ_PROC,	/**< \brief (Logical) Processor.
			  * An execution unit (may share a core with some
			  * other logical processors, e.g. in the case of
			  * an SMT core).
			  */

  TOPO_OBJ_MISC,	/**< \brief Miscellaneous objects.
			  * Objects which do not fit in the above but are
			  * detected by libtopology and are useful to take into
			  * account for affinity. For instance, some OSes
			  * expose their arbitrary processors aggregation this
			  * way.
			  */
} topo_obj_type_t;
/** \brief Maximal value of an Object Type */
#define TOPO_OBJ_TYPE_MAX (TOPO_OBJ_MISC+1)

/** \brief Convert an object type into a number that permits to compare them
 *
 * Types shouldn't be compared as they are, since newer ones may be added in
 * the future.  This function returns an integer value that can be used
 * instead.
 *
 * \note topo_get_type_order(TOPO_OBJ_SYSTEM) will always be the lowest
 * value, and topo_get_type_order(TOPO_OBJ_PROC) will always be the highest
 * value.
 */
int topo_get_type_order(topo_obj_type_t type);

/** \brief converse of topo_get_obj_type_oder()
 *
 * This is the converse of topo_get_type_order().
 */
topo_obj_type_t topo_get_order_type(int order);


/** @} */

/** \defgroup topology_objects Topology Objects
 * @{
 */

/** \brief Structure of a topology Object
 *
 * Applications mustn't modify any field except ::userdata .
 */
struct topo_obj {
  /* physical information */
  topo_obj_type_t type;		/**< \brief Type of object */
  signed os_index;		/**< \brief OS-provided physical index number */

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
    /**< \brief Machine-specific Object Attributes */
    struct topo_machine_attr_u {
      char *dmi_board_vendor;		  /**< \brief DMI Board Vendor name */
      char *dmi_board_name;		  /**< \brief DMI Board Model name */
      unsigned long memory_kB;		  /**< \brief Size of memory node */
      unsigned long huge_page_free;	  /**< \brief Number of available huge pages */
      unsigned long huge_page_size_kB;	  /**< \brief Size of huge pages */
    } machine;
    /**< \brief System-specific Object Attributes */
    struct topo_machine_attr_u system;
    /** \brief Misc-specific Object Attributes */
    struct topo_misc_attr_u {
      unsigned depth;			  /**< \brief Depth of misc object */
    } misc;
  } attr;				  /**< \brief Object type-specific attributes */

  /* global position */
  unsigned depth;			/**< \brief Vertical index in the hierarchy */
  unsigned logical_index;		/**< \brief Horizontal index in the whole list of similar objects,
					 * could be a "cousin_rank" since it's the rank within the "cousin" list below */
  struct topo_obj *next_cousin;		/**< \brief Next object of same type */
  struct topo_obj *prev_cousin;		/**< \brief Previous object of same type */

  /* father */
  struct topo_obj *father;		/**< \brief Father, \c NULL if root (system object) */
  unsigned sibling_rank;		/**< \brief Index in father's \c children[] array */
  struct topo_obj *next_sibling;	/**< \brief Next object below the same father*/
  struct topo_obj *prev_sibling;	/**< \brief Previous object below the same father */

  /* children */
  unsigned arity;			/**< \brief Number of children */
  struct topo_obj **children;		/**< \brief Children, \c children[0 .. arity -1] */
  struct topo_obj *first_child;		/**< \brief First child */
  struct topo_obj *last_child;		/**< \brief Last child */

  /* misc */
  void *userdata;			/**< \brief Application-given private data pointer, initialized to \c NULL, use it as you wish */

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
 * These functions can optionally be called between topology_init() and
 * topology_load() to configure how the detection should be performed, e.g. to
 * ignore some objects types, define a synthetic topology, etc.
 *
 * If none of them is called, the default is to detect all the objects of the
 * machine that the caller is allowed to access.
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
 * If no object of this type is present on the underlying architecture, or if
 * the OS doesn't provide this kind of information, the function returns
 * TOPO_TYPE_DEPTH_UNKNOWN.
 *
 * If type is absent but a similar type is acceptable, see also
 * topo_get_type_or_below_depth() and topo_get_type_or_above_depth().
 */
extern unsigned topo_get_type_depth (topo_topology_t topology, topo_obj_type_t type);
#define TOPO_TYPE_DEPTH_UNKNOWN -1 /**< \brief No object of given type exists in the topology. */
#define TOPO_TYPE_DEPTH_MULTIPLE -2 /**< \brief Objects of given type exist at different depth in the topology. */

/** \brief Returns the type of objects at depth \p depth. */
extern topo_obj_type_t topo_get_depth_type (topo_topology_t topology, unsigned depth);

/** \brief Returns the width of level at depth \p depth */
extern unsigned topo_get_depth_nbobjs (topo_topology_t topology, unsigned depth);

/** @} */


/** \defgroup topology_traversal Retrieve Objects
 * @{
 */

/** \brief Returns the topology object at index \p index from depth \p depth */
extern topo_obj_t topo_get_obj (topo_topology_t topology, unsigned depth, unsigned index);

/** @} */

/** \defgroup topology_conversion Object/String Conversion
 * @{
 */

/** \brief Return a stringified topology object type */
extern const char * topo_obj_type_string (topo_obj_type_t type);

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
 *
 * It is often useful to call topo_cpuset_singlify() first so that a single CPU
 * remains in the set. This way, the process will not even migrate between
 * different CPUs. Some OSes also only support that kind of binding.
 *
 * \note Depending on OSes and implementations, strict binding may not be
 * possible, not be allowed, only used as a hint, etc. If strict binding is
 * required, the strict parameter of the binding functions should be set to 1,
 * and the function will fail if strict binding is not possible or allowed.
 *
 * \note Some OSes do not provide all ways to bind processes, threads, etc and
 * the corresponding binding functions may fail. The most portable version that
 * should be preferred over the others, whenever possible, is
 * topo_set_cpubind(topology, set, 0),
 * as it just binds the current program.
 *
 * \note To unbind, just call the binding function with either a full cpuset or
 * a cpuset equal to the system cpuset.
 * @{
 */

/** \brief Process/Thread binding policy */
typedef enum {
  TOPO_CPUBIND_BASIC = 0,	/**< \brief Bind current process assuming it's mono-threaded */
  TOPO_CPUBIND_PROCESS = 1,	/**< \brief Bind all threads of current process */
  TOPO_CPUBIND_THREAD = 2,	/**< \brief Bind current thread of current process */
} topo_cpubind_policy_t;

/** \brief Bind current process or thread on cpus given in cpuset \p set
 *
 * If \p strict is set and the OS cannot bind strictly, the call will fail
 * with a negative return value.
 */
extern int topo_set_cpubind(topo_topology_t topology, const topo_cpuset_t *set,
			    topo_cpubind_policy_t policy, int strict);

/** \brief Bind a process \p pid on cpus given in cpuset \p set
 *
 * \note topo_pid_t is pid_t on unix platforms, and HANDLE on native Windows
 * platforms
 */
extern int topo_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *set, int strict);

/** \brief Bind a thread \p tid on cpus given in cpuset \p set
 */
#ifdef topo_thread_t
extern int topo_set_thread_cpubind(topo_topology_t topology, topo_thread_t tid, const topo_cpuset_t *set, int strict);
#endif

/** @} */


/* high-level helpers */
#include <topology/helper.h>


#endif /* TOPOLOGY_H */
