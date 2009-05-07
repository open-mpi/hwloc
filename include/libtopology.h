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

/** \brief Type of topology level */
enum topo_level_type_e {
  TOPO_LEVEL_MACHINE,	/**< \brief Whole machine */
  TOPO_LEVEL_FAKE,	/**< \brief Fake level for meeting the marcel_topo_max_arity constraint */ /* A passer dans marcel_topology.h */
  TOPO_LEVEL_NODE,	/**< \brief NUMA node */
  TOPO_LEVEL_DIE,	/**< \brief Physical chip */
  TOPO_LEVEL_L3,	/**< \brief L3 cache */
  TOPO_LEVEL_L2,	/**< \brief L2 cache */
  TOPO_LEVEL_CORE,	/**< \brief Core */
  TOPO_LEVEL_L1,	/**< \brief L1 cache */
  TOPO_LEVEL_PROC,	/**< \brief SMT Processor in a core */
  TOPO_LEVEL_MAX,
};
typedef enum topo_level_type_e topo_level_type_t;

/** Structure of a topology level */
struct topo_level {
  enum topo_level_type_e type;		/**< \brief Type of level */
  unsigned level;			/**< \brief Vertical index in marcel_topo_levels */
  unsigned number;			/**< \brief Horizontal index in marcel_topo_levels[l.level] */
  unsigned index;			/**< \brief Index in fathers' children[] array */
  unsigned arity;			/**< \brief Number of children */
  struct topo_level **children;		/**< \brief Children, children[0 .. arity -1] */
  struct topo_level *father;		/**< \brief Father, NULL if root (machine level) */
  signed physical_index;		/**< \brief OS-provided physical index number */
  unsigned long memory_kB;		/**< \brief Size of memory bank or caches */
  unsigned long huge_page_free;
  unsigned cache_depth;
  unsigned admin_disabled;		/**< \brief Set if disabled by the administrator (for instance Linux Cpusets) */
  topo_cpuset_t cpuset;			/**< \brief CPUs covered by this level */
};
typedef struct topo_level * topo_level_t;


/** \brief Allocate a topology context. */
extern int topo_topology_init (topo_topology_t *topologyp);
/** \brief Build the actual topology once initialized with _init and tuned with backends and friends */
extern int topo_topology_load(topo_topology_t topology);
/** \brief Terminate and free a topology context. */
extern void topo_topology_destroy (topo_topology_t topology);

/** \brief Ignore a level type.
    To be called between _init and _load. */
extern int topo_topology_ignore_type(topo_topology_t topology, topo_level_type_t type);
/** \brief Ignore a level type if it does not bring any structure.
    To be called between _init and _load. */
extern int topo_topology_ignore_type_keep_structure(topo_topology_t topology, topo_level_type_t type);
/** \brief Ignore all levels that do not bring any structure.
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


/** \brief Returns the depth of levels of type _type_. If no level of
    this type is present on the underlying architecture, the function
    returns the depth of the first "present" level we find uppon
    _type_. */
extern unsigned topo_get_type_depth (topo_topology_t topology, enum topo_level_type_e type);

/** \brief Returns the type of levels at depth _depth_. */
extern enum topo_level_type_e topo_get_depth_type (topo_topology_t topology, unsigned depth);

/** \brief Returns the width of depth level _depth_ */
extern unsigned topo_get_depth_nbitems (topo_topology_t topology, unsigned depth);

/** \brief Returns the topology level at index _index_ from depth _depth_ */
extern topo_level_t topo_get_level (topo_topology_t topology, unsigned depth, unsigned index);

/** \brief Returns the top-level of the topology-tree. Its type is TOPO_MACHINE_LEVEL. */
static inline topo_level_t topo_get_machine_level (topo_topology_t topology) { return topo_get_level(topology, 0, 0); }

/** \brief Returns the common father level to levels lvl1 and lvl2 */
extern topo_level_t topo_find_common_ancestor_level (topo_level_t lvl1, topo_level_t lvl2);

/** \brief Returns true if _level_ is inside the subtree beginning
    with _subtree_root_. */
extern int topo_level_is_in_subtree (topo_level_t subtree_root, topo_level_t level);

/** \brief Do a depth-first traversal of the topology to find and sort
    all levels that are at the same depth than _src_.
    Report in _lvls_ up to _max_ physically closest ones to _src_.
    Return the actual number of levels that were found. */
extern int topo_find_closest_levels (topo_topology_t topology, topo_level_t src, topo_level_t *lvls, int max);

/** \brief return a stringified topology level type */
extern const char * topo_level_string (enum topo_level_type_e l);

/** \brief print a human-readable form of the given topology level */
extern void topo_print_level (topo_topology_t topology, topo_level_t l,
			      FILE *output, int verbose_mode,
			      const char *indexprefix, const char* levelterm);

#endif /* LIBTOPOLOGY_H */
