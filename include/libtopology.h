/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_H
#define LIBTOPOLOGY_H

#include <stdio.h>

#include <libtopology/allocator.h>
#include <libtopology/cpuset.h>


/* \brief Topology context */
struct topo_topology;
typedef struct topo_topology * topo_topology_t;

/* \brief Global information about the topology */
struct topo_topology_info {
  /* topology size */
  unsigned nb_processors;
  unsigned nb_nodes;
  unsigned depth;

  /* machine specifics */
  char *dmi_board_vendor;
  char *dmi_board_name;
  unsigned long huge_page_size_kB;
  int is_fake; /* set if the topology is different from the actual underlying machine */
};

/** \brief Type of topology object */
enum topo_obj_type_e {
  /* objects that are always ordered the same in the hierarchy */
  TOPO_OBJ_MACHINE,	/**< \brief Whole machine */
  TOPO_OBJ_NODE,	/**< \brief NUMA node */
  TOPO_OBJ_DIE,	/**< \brief Physical chip */
  TOPO_OBJ_CORE,	/**< \brief Core */
  TOPO_OBJ_PROC,	/**< \brief SMT Processor in a core */

  /* objects that may appear at various depth among the above ones */
  TOPO_OBJ_FAKE,	/**< \brief Fake object that may be needed under special circumstances */
  TOPO_OBJ_CACHE,	/**< \brief Cache */
};
#define TOPO_OBJ_ORDERED_TYPE_MAX (TOPO_OBJ_PROC+1)
#define TOPO_OBJ_TYPE_MAX (TOPO_OBJ_CACHE+1)
typedef enum topo_obj_type_e topo_obj_type_t;

/** Structure of a topology object */
struct topo_obj {
  enum topo_obj_type_e type;		/**< \brief Type of object */
  unsigned level;			/**< \brief Vertical index in the hierarchy */
  unsigned number;			/**< \brief Horizontal index in the whole list of similar objects */
  unsigned index;			/**< \brief Index in fathers' children[] array */
  unsigned arity;			/**< \brief Number of children */
  struct topo_obj **children;		/**< \brief Children, children[0 .. arity -1] */
  struct topo_obj *father;		/**< \brief Father, NULL if root (machine object) */
  signed physical_index;		/**< \brief OS-provided physical index number */
  unsigned long memory_kB;		/**< \brief Size of memory bank or caches */
  unsigned long huge_page_free;
  unsigned cache_depth;
  unsigned admin_disabled;		/**< \brief Set if disabled by the administrator (for instance Linux Cpusets) */
  topo_cpuset_t cpuset;			/**< \brief CPUs covered by this object */
};
typedef struct topo_obj * topo_obj_t;


/** \brief Allocate a topology context. */
extern int topo_topology_init (topo_topology_t *topologyp);
/** \brief Build the actual topology once initialized with _init and tuned with backends and friends */
extern int topo_topology_load(topo_topology_t topology);
/** \brief Terminate and free a topology context. */
extern void topo_topology_destroy (topo_topology_t topology);

/** \brief Ignore a object type.
    To be called between _init and _load. */
extern int topo_topology_ignore_type(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore a object type if it does not bring any structure.
    To be called between _init and _load. */
extern int topo_topology_ignore_type_keep_structure(topo_topology_t topology, topo_obj_type_t type);
/** \brief Ignore all objects that do not bring any structure.
    To be called between _init and _load. */
extern int topo_topology_ignore_all_keep_structure(topo_topology_t topology);
/** \brief Set OR'ed flags to non-yet-loaded topology.
    To be called between _init and _load.  */
enum topo_flags_e {
  TOPO_FLAGS_IGNORE_THREADS = (1<<0),
  TOPO_FLAGS_IGNORE_ADMIN_DISABLE = (1<<1), /* Linux Cpusets, ... */
};
extern int topo_topology_set_flags (topo_topology_t topology, unsigned long flags);
/* FIXME: switch to a backend interface */
/** \brief Change the file-system root path when building the topology from sysfs/procs.
    To be called between _init and _load.  */
extern int topo_topology_set_fsys_root(topo_topology_t topology, const char *fsys_root_path);
/** \brief Enable synthetic topology. */
extern int topo_topology_set_synthetic(struct topo_topology *topology, const char *description);


/* \brief Get global information about the topology.
    To be called between _init and _load.  */
extern int topo_topology_get_info(topo_topology_t topology, struct topo_topology_info *info);


/** \brief Returns the depth of objects of type _type_. If no object of
    this type is present on the underlying architecture, the function
    returns the depth of the first "present" object we find uppon
    _type_. */
extern unsigned topo_get_type_depth (topo_topology_t topology, enum topo_obj_type_e type);
#define TOPO_TYPE_DEPTH_UNKNOWN -1
#define TOPO_TYPE_DEPTH_MULTIPLE -2

/** \brief Returns the type of objects at depth _depth_. */
extern enum topo_obj_type_e topo_get_depth_type (topo_topology_t topology, unsigned depth);

/** \brief Returns the width of level at depth _depth_ */
extern unsigned topo_get_depth_nbitems (topo_topology_t topology, unsigned depth);

/** \brief Returns the topology object at index _index_ from depth _depth_ */
extern topo_obj_t topo_get_object (topo_topology_t topology, unsigned depth, unsigned index);

/** \brief Returns the top-object of the topology-tree. Its type is TOPO_OBJ_MACHINE. */
static inline topo_obj_t topo_get_machine_object (topo_topology_t topology) { return topo_get_object(topology, 0, 0); }

/** \brief Returns the common father object to objects lvl1 and lvl2 */
extern topo_obj_t topo_find_common_ancestor_object (topo_obj_t obj1, topo_obj_t obj2);

/** \brief Returns true if _obj_ is inside the subtree beginning
    with _subtree_root_. */
extern int topo_object_is_in_subtree (topo_obj_t subtree_root, topo_obj_t obj);

/** \brief Do a depth-first traversal of the topology to find and sort
    all objects that are at the same depth than _src_.
    Report in _lvls_ up to _max_ physically closest ones to _src_.
    Return the actual number of objects that were found. */
extern int topo_find_closest_objects (topo_topology_t topology, topo_obj_t src, topo_obj_t *objs, int max);

/* \brief Find the lowest object covering at least the given cpu set */
extern struct topo_obj * topo_find_cpuset_ancestor_object (struct topo_topology *topology, topo_cpuset_t *set);

/** \brief return a stringified topology object type */
extern const char * topo_object_type_string (enum topo_obj_type_e l);

/** \brief print a human-readable form of the given topology object */
extern void topo_print_object (topo_topology_t topology, topo_obj_t l,
			       FILE *output, int verbose_mode,
			       const char *indexprefix, const char* term);

/** \brief stringify the cpuset containing a set of objects */
extern int topo_object_cpuset_snprintf(char *str, size_t size, size_t nobj, topo_obj_t *objs);

/** \brief bind current process on cpus given in the set */
extern int topo_set_cpubind(topo_cpuset_t *set);

#endif /* LIBTOPOLOGY_H */
