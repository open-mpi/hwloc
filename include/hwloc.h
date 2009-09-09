/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/** \file
 * \brief The hwloc API.
 *
 * See hwloc/cpuset.h for CPU set specific macros
 * See hwloc/helper.h for high-level topology traversal helpers
 */

#ifndef HWLOC_H
#define HWLOC_H

#include <sys/types.h>
#include <stdio.h>


/*
 * Cpuset bitmask definitions
 */

#include <hwloc/cpuset.h>



/** \defgroup hwlocality_topology_info Topology and Topology Info
 * @{
 */

struct hwloc_topology;
/** \brief Topology context
 *
 * To be initialized with hwloc_topology_init() and built with hwloc_topology_load().
 */
typedef struct hwloc_topology * hwloc_topology_t;

/** \brief Global information about a Topology context,
 *
 * To be filled with hwloc_topology_get_info().
 */
struct hwloc_topology_info {
  /** \brief topology size */
  unsigned depth;

  /** \brief set if the topology is different from the actual underlying machine */
  int is_fake;
};


/** @} */

/** \defgroup hwlocality_types Topology Object Types
 * @{
 */

/** \brief Type of topology Object.
 *
 * Do not rely on the ordering of the values as new ones may be defined in the
 * future!  If you need to compare types, use the value returned by
 * hwloc_get_type_order() instead.
 */
typedef enum {
  HWLOC_OBJ_SYSTEM,	/**< \brief Whole system (may be a cluster of machines).
  			  * The whole system that is accessible to hwloc.
			  * That may comprise several machines in SSI systems
			  * like Kerrighed.
			  */
  HWLOC_OBJ_MACHINE,	/**< \brief Machine.
			  * A set of processors and memory with cache
			  * coherency.
			  */
  HWLOC_OBJ_NODE,	/**< \brief NUMA node.
			  * A set of processors around memory which the
			  * processors can directly access.
			  */
  HWLOC_OBJ_SOCKET,	/**< \brief Socket, physical package, or chip.
			  * In the physical meaning, i.e. that you can add
			  * or remove physically.
			  */
  HWLOC_OBJ_CACHE,	/**< \brief Data cache.
			  * Can be L1, L2, L3, ...
			  */
  HWLOC_OBJ_CORE,	/**< \brief Core.
			  * A computation unit (may be shared by several
			  * logical processors).
			  */
  HWLOC_OBJ_PROC,	/**< \brief (Logical) Processor.
			  * An execution unit (may share a core with some
			  * other logical processors, e.g. in the case of
			  * an SMT core).
			  */

  HWLOC_OBJ_MISC,	/**< \brief Miscellaneous objects.
			  * Objects which do not fit in the above but are
			  * detected by hwloc and are useful to take into
			  * account for affinity. For instance, some OSes
			  * expose their arbitrary processors aggregation this
			  * way.
			  */
} hwloc_obj_type_t;
/** \brief Maximal value of an Object Type */
#define HWLOC_OBJ_TYPE_MAX (HWLOC_OBJ_MISC+1)

/** \brief Convert an object type into a number that permits to compare them
 *
 * Types shouldn't be compared as they are, since newer ones may be added in
 * the future.  This function returns an integer value that can be used
 * instead.
 *
 * \note hwloc_get_type_order(HWLOC_OBJ_SYSTEM) will always be the lowest
 * value, and hwloc_get_type_order(HWLOC_OBJ_PROC) will always be the highest
 * value.
 */
int hwloc_get_type_order(hwloc_obj_type_t type);

/** \brief Converse of hwloc_get_type_oder()
 *
 * This is the converse of hwloc_get_type_order().
 */
hwloc_obj_type_t hwloc_get_order_type(int order);


/** @} */

/** \defgroup hwlocality_objects Topology Objects
 * @{
 */

union hwloc_obj_attr_u;

/** \brief Structure of a topology Object
 *
 * Applications mustn't modify any field except ::userdata .
 */
struct hwloc_obj {
  /* physical information */
  hwloc_obj_type_t type;		/**< \brief Type of object */
  signed os_index;		/**< \brief OS-provided physical index number */

  /** \brief Object type-specific Attributes */
  union hwloc_obj_attr_u *attr;

  /* global position */
  unsigned depth;			/**< \brief Vertical index in the hierarchy */
  unsigned logical_index;		/**< \brief Horizontal index in the whole list of similar objects,
					 * could be a "cousin_rank" since it's the rank within the "cousin" list below */
  struct hwloc_obj *next_cousin;		/**< \brief Next object of same type */
  struct hwloc_obj *prev_cousin;		/**< \brief Previous object of same type */

  /* father */
  struct hwloc_obj *father;		/**< \brief Father, \c NULL if root (system object) */
  unsigned sibling_rank;		/**< \brief Index in father's \c children[] array */
  struct hwloc_obj *next_sibling;	/**< \brief Next object below the same father*/
  struct hwloc_obj *prev_sibling;	/**< \brief Previous object below the same father */

  /* children */
  unsigned arity;			/**< \brief Number of children */
  struct hwloc_obj **children;		/**< \brief Children, \c children[0 .. arity -1] */
  struct hwloc_obj *first_child;		/**< \brief First child */
  struct hwloc_obj *last_child;		/**< \brief Last child */

  /* misc */
  void *userdata;			/**< \brief Application-given private data pointer, initialized to \c NULL, use it as you wish */

  /* cpuset */
  hwloc_cpuset_t cpuset;			/**< \brief CPUs covered by this object */

  signed os_level;			/**< \brief OS-provided physical level */
};
typedef struct hwloc_obj * hwloc_obj_t;

/** \brief Object type-specific Attributes */
union hwloc_obj_attr_u {
  /** \brief Cache-specific Object Attributes */
  struct hwloc_cache_attr_u {
    unsigned long memory_kB;		  /**< \brief Size of cache */
    unsigned depth;			  /**< \brief Depth of cache */
  } cache;
  /** \brief Node-specific Object Attributes */
  struct hwloc_memory_attr_u {
    unsigned long memory_kB;		  /**< \brief Size of memory node */
    unsigned long huge_page_free;	  /**< \brief Number of available huge pages */
  } node;
  /**< \brief Machine-specific Object Attributes */
  struct hwloc_machine_attr_u {
    char *dmi_board_vendor;		  /**< \brief DMI Board Vendor name */
    char *dmi_board_name;		  /**< \brief DMI Board Model name */
    unsigned long memory_kB;		  /**< \brief Size of memory node */
    unsigned long huge_page_free;	  /**< \brief Number of available huge pages */
    unsigned long huge_page_size_kB;	  /**< \brief Size of huge pages */
  } machine;
  /**< \brief System-specific Object Attributes */
  struct hwloc_machine_attr_u system;
  /** \brief Misc-specific Object Attributes */
  struct hwloc_misc_attr_u {
    unsigned depth;			  /**< \brief Depth of misc object */
  } misc;
};

/** @} */

/** \defgroup hwlocality_creation Create and Destroy Topologies
 * @{
 */

/** \brief Allocate a topology context.
 *
 * \param[out] topologyp is assigned a pointer to the new allocated context.
 *
 * \return 0 on success, -1 on error.
 */
extern int hwloc_topology_init (hwloc_topology_t *topologyp);
/** \brief Build the actual topology
 *
 * Build the actual topology once initialized with hwloc_topology_init() and
 * tuned with ::topology_configuration routine.
 * No other routine may be called earlier using this topology context.
 *
 * \param topology is the topology to be loaded with objects.
 *
 * \return 0 on success, -1 on error.
 *
 * \sa topology_configuration
 */
extern int hwloc_topology_load(hwloc_topology_t topology);
/** \brief Terminate and free a topology context
 *
 * \param topology is the topology to be freed
 */
extern void hwloc_topology_destroy (hwloc_topology_t topology);
/** \brief Run internal checks on a topology structure
 *
 * \param topology is the topology to be checked
 */
extern void hwloc_topology_check(hwloc_topology_t topology);

/** @} */

/** \defgroup hwlocality_configuration Configure Topology Detection
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
 * The top-level type HWLOC_OBJ_SYSTEM and bottom-level type HWLOC_OBJ_PROC may
 * not be ignored.
 */
extern int hwloc_topology_ignore_type(hwloc_topology_t topology, hwloc_obj_type_t type);
/** \brief Ignore an object type if it does not bring any structure.
 *
 * Ignore all objects from the given type as long as they do not bring any structure:
 * Each ignored object should have a single children or be the only child of its father.
 * The top-level type HWLOC_OBJ_SYSTEM and bottom-level type HWLOC_OBJ_PROC may
 * not be ignored.
 */
extern int hwloc_topology_ignore_type_keep_structure(hwloc_topology_t topology, hwloc_obj_type_t type);
/** \brief Ignore all objects that do not bring any structure.
 *
 * Ignore all objects that do not bring any structure:
 * Each ignored object should have a single children or be the only child of its father.
 */
extern int hwloc_topology_ignore_all_keep_structure(hwloc_topology_t topology);
/** \brief Flags to be set onto a topology context before load.
 *
 * Flags should be given to hwloc_topology_set_flags().
 */
enum hwloc_topology_flags_e {
  /* \brief Detect the whole system, ignore reservations that may have been setup by the administrator.
   *
   * Gather all resources, even if some were disabled by the administrator.
   * For instance, ignore Linux Cpusets and gather all processors and memory nodes.
   */
  HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM = (1<<1),
};
/** \brief Set OR'ed flags to non-yet-loaded topology.
 *
 * Set a OR'ed set of hwloc_topology_flags_e onto a topology that was not yet loaded.
 */
extern int hwloc_topology_set_flags (hwloc_topology_t topology, unsigned long flags);
/** \brief Change the file-system root path when building the topology from sysfs/procfs.
 *
 * On Linux system, use sysfs and procfs files as if they were mounted on the given
 * \p fsys_root_path instead of the main file-system root.
 * Not using the main file-system root causes the is_fake field of the hwloc_topology_info
 * structure to be set.
 */
extern int hwloc_topology_set_fsys_root(hwloc_topology_t __hwloc_restrict topology, const char * __hwloc_restrict fsys_root_path);
/** \brief Enable synthetic topology.
 *
 * Gather topology information from the given \p description
 * which should be a comma separated string of numbers describing
 * the arity of each level.
 * Each number may be prefixed with a type and a colon to enforce the type
 * of a level.
 */
extern int hwloc_topology_set_synthetic(hwloc_topology_t __hwloc_restrict topology, const char * __hwloc_restrict description);
/** \brief Enable XML-file based topology.
 *
 * Gather topology information the XML file given at \p xmlpath.
 * This file may have been generated earlier with lstopo file.xml.
 */
extern int hwloc_topology_set_xml(hwloc_topology_t __hwloc_restrict topology, const char * __hwloc_restrict xmlpath);


/** @} */


/** \defgroup hwlocality_information Get some Topology Information
 * @{
 */

/** \brief Get additional global information about the topology.
 *
 * Retrieve additional global information about a loaded topology context.
 * Might be useful if the whole topology depth is needed for instance.
 */
extern int hwloc_topology_get_info(hwloc_topology_t  __hwloc_restrict topology, struct hwloc_topology_info * __hwloc_restrict info);

/** \brief Returns the depth of objects of type \p type.
 *
 * If no object of this type is present on the underlying architecture, or if
 * the OS doesn't provide this kind of information, the function returns
 * HWLOC_TYPE_DEPTH_UNKNOWN.
 *
 * If type is absent but a similar type is acceptable, see also
 * hwloc_get_type_or_below_depth() and hwloc_get_type_or_above_depth().
 */
extern int hwloc_get_type_depth (hwloc_topology_t topology, hwloc_obj_type_t type);
#define HWLOC_TYPE_DEPTH_UNKNOWN -1 /**< \brief No object of given type exists in the topology. */
#define HWLOC_TYPE_DEPTH_MULTIPLE -2 /**< \brief Objects of given type exist at different depth in the topology. */

/** \brief Returns the type of objects at depth \p depth. */
extern hwloc_obj_type_t hwloc_get_depth_type (hwloc_topology_t topology, unsigned depth);

/** \brief Returns the width of level at depth \p depth */
extern unsigned hwloc_get_depth_nbobjs (hwloc_topology_t topology, unsigned depth);

/** @} */


/** \defgroup hwlocality_traversal Retrieve Objects
 * @{
 */

/** \brief Returns the topology object at index \p index from depth \p depth */
extern hwloc_obj_t hwloc_get_obj_by_depth (hwloc_topology_t topology, unsigned depth, unsigned index);

/** @} */

/** \defgroup hwlocality_conversion Object/String Conversion
 * @{
 */

/** \brief Return a stringified topology object type */
extern const char * hwloc_obj_type_string (hwloc_obj_type_t type);

/** \brief Return an object type from the string */
extern hwloc_obj_type_t hwloc_obj_type_of_string (const char * string);

/** \brief Stringify a given topology object into a human-readable form.
 *
 * \return how many characters were actually written (not including the ending \\0). */
extern int hwloc_obj_snprintf(char * __hwloc_restrict string, size_t size,
			     hwloc_topology_t topology, hwloc_obj_t obj,
			     const char * __hwloc_restrict indexprefix, int verbose);

/** \brief Stringify the cpuset containing a set of objects.
 *
 * \return how many characters were actually written (not including the ending \\0). */
extern int hwloc_obj_cpuset_snprintf(char * __hwloc_restrict str, size_t size, size_t nobj, const hwloc_obj_t * __hwloc_restrict objs);

/** @} */


/** \defgroup hwlocality_binding Binding
 *
 * It is often useful to call hwloc_cpuset_singlify() first so that a single CPU
 * remains in the set. This way, the process will not even migrate between
 * different CPUs. Some OSes also only support that kind of binding.
 *
 * \note Some OSes do not provide all ways to bind processes, threads, etc and
 * the corresponding binding functions may fail. ENOSYS is returned when it is
 * not possible to bind the requested kind of object processes/threads). EXDEV
 * is returned when the requested cpuset can not be enforced (e.g. some systems
 * only allow one CPU, and some other systems only allow one NUMA node)
 *
 * The most portable version that
 * should be preferred over the others, whenever possible, is
 *
 * \code
 * hwloc_set_cpubind(topology, set, 0),
 * \endcode
 *
 * as it just binds the current program, assuming it is monothread, or
 *
 * \code
 * hwloc_set_cpubind(topology, set, HWLOC_CPUBIND_THREAD),
 * \endcode
 *
 * which binds the current thread of the current program (which may be
 * multithreaded).
 *
 * \note To unbind, just call the binding function with either a full cpuset or
 * a cpuset equal to the system cpuset.
 * @{
 */

/** \brief Process/Thread binding policy.
 *
 * These flags can be used to refine the binding policy.
 *
 * The default (0) is to bind the current process, assumed to be mono-thread,
 * in a non-strict way.  This is the most portable way to bind as all OSes
 * usually provide it.
 *
 * \note Depending on OSes and implementations, strict binding (i.e. the
 * thread/process will really never be scheduled outside of the cpuset) may not
 * be possible, not be allowed, only used as a hint when no load balancing is
 * needed, etc.  If strict binding is required, the strict flag should be set,
 * and the function will fail if strict binding is not possible or allowed.
 *
 */
typedef enum {
  HWLOC_CPUBIND_PROCESS = (1<<0),	/**< \brief Bind all threads of the current multithreaded process.
					  * This may not be supported by some OSes (e.g. Linux). */
  HWLOC_CPUBIND_THREAD = (1<<1),		/**< \brief Bind current thread of current process */
  HWLOC_CPUBIND_STRICT = (1<<2),		/**< \brief Request for strict binding from the OS
					 * Note that strict binding may not be
					 * allowed for administrative reasons,
					 * and the binding function will fail
					 * in that case.
					 */
} hwloc_cpubind_policy_t;

/** \brief Bind current process or thread on cpus given in cpuset \p set
 */
extern int hwloc_set_cpubind(hwloc_topology_t topology, const hwloc_cpuset_t *set,
			    int policy);

/** \brief Bind a process \p pid on cpus given in cpuset \p set
 *
 * \note hwloc_pid_t is pid_t on unix platforms, and HANDLE on native Windows
 * platforms
 *
 * \note HWLOC_CPUBIND_THREAD can not be used in \p policy.
 */
extern int hwloc_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, const hwloc_cpuset_t *set, int policy);

/** \brief Bind a thread \p tid on cpus given in cpuset \p set
 *
 * \note hwloc_thread_t is pthread_t on unix platforms, and HANDLE on native
 * Windows platforms
 *
 * \note HWLOC_CPUBIND_PROCESS can not be used in \p policy.
 */
#ifdef hwloc_thread_t
extern int hwloc_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, const hwloc_cpuset_t *set, int policy);
#endif

/** @} */


/* high-level helpers */
#include <hwloc/helper.h>


#endif /* HWLOC_H */
