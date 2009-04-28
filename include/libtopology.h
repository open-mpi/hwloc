/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_H
#define LIBTOPOLOGY_H

#include <stdio.h>

#include <libtopology/allocator.h>
#include <libtopology/cpuset.h>

/* \brief Topology context */
struct topo_topology;
typedef struct topo_topology * topo_topology_t;

/** \brief Allocate a topology context. */
extern int topo_topology_init (topo_topology_t *topologyp);
/** \brief Build the actual topology once initialized with _init and tuned with backends and friends */
extern int topo_topology_load(topo_topology_t topology);
/** \brief Terminate and free a topology context. */
extern void topo_topology_destroy (topo_topology_t topology);

/* FIXME: switch to a backend interface */
/** \brief Change the file-system root path when building the topology from sysfs/procs */
extern int topo_topology_set_fsys_root(topo_topology_t topology, const char *fsys_root_path);

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
};
extern int topo_topology_get_info(topo_topology_t topology, struct topo_topology_info *info);


/** \brief Type of topology level */
enum lt_level_e {
	LT_LEVEL_MACHINE,	/**< \brief Whole machine */
	LT_LEVEL_FAKE,	/**< \brief Fake level for meeting the marcel_topo_max_arity constraint */ /* A passer dans marcel_topology.h */
	LT_LEVEL_NODE,	/**< \brief NUMA node */
	LT_LEVEL_DIE,	/**< \brief Physical chip */
	LT_LEVEL_L3,	/**< \brief L3 cache */
	LT_LEVEL_L2,	/**< \brief L2 cache */
	LT_LEVEL_CORE,	/**< \brief Core */
	LT_LEVEL_L1,	/**< \brief L1 cache */
	LT_LEVEL_PROC,	/**< \brief SMT Processor in a core */
	LT_LEVEL_MAX,
};

/** \brief Type of memory attached to the topology level */
enum lt_level_memory_type_e {
	LT_LEVEL_MEMORY_L1 = 0,
	LT_LEVEL_MEMORY_L2 = 1,
	LT_LEVEL_MEMORY_L3 = 2,
	LT_LEVEL_MEMORY_NODE = 3,
	LT_LEVEL_MEMORY_MACHINE = 4,
	LT_LEVEL_MEMORY_TYPE_MAX
};

/** Structure of a topology level */
struct lt_level {
        enum lt_level_e type;	        /**< \brief Type of level */
	unsigned level;			/**< \brief Vertical index in marcel_topo_levels */
	unsigned number;		/**< \brief Horizontal index in marcel_topo_levels[l.level] */
	unsigned index;			/**< \brief Index in fathers' children[] array */

        signed physical_index[LT_LEVEL_MAX]; /**< \brief OS-provided physical index numbers */

	lt_cpuset_t cpuset;		/**< \brief CPUs covered by this level */

	unsigned arity;			/**< \brief Number of children */
	struct lt_level **children;	/**< \brief Children, children[0 .. arity -1] */
	struct lt_level *father;	/**< \brief Father, NULL if root (machine level) */

	unsigned long memory_kB[LT_LEVEL_MEMORY_TYPE_MAX];
        unsigned long huge_page_free;
};

typedef struct lt_level * lt_level_t;

/** \brief Returns the common father level to levels lvl1 and lvl2 */
extern lt_level_t lt_find_common_ancestor (lt_level_t lvl1, lt_level_t lvl2);

/** \brief Returns true if _level_ is inside the subtree beginning
    with _subtree_root_. */
extern int lt_is_in_subtree (lt_level_t subtree_root, lt_level_t level);

/** \brief Do a depth-first traversal of the topology to find and sort
    all levels that are at the same depth than _src_.
    Report in _lvls_ up to _max_ physically closest ones to _src_.
    Return the actual number of levels that were found. */
extern int lt_find_closest(topo_topology_t topology, struct lt_level *src, struct lt_level **lvls, int max);

/** \brief indexes into ::lt_levels, but available from application */
lt_level_t lt_level(unsigned level, unsigned index);

/** \brief return a stringified topology level type */
const char * lt_level_string(enum lt_level_e l);

/** \brief print a human-readable form of the given topology level */
void lt_print_level(topo_topology_t topology, struct lt_level *l,
		    FILE *output, int verbose_mode,
		    const char *separator, const char *indexprefix,
		    const char* labelseparator, const char* levelterm);

#if 0 /* not implemented yet */
/** \brief Returns the depth of levels of type _type_. If no level of
    this type is present on the underlying architecture, the function
    returns the depth of the first "present" level we find uppon
    _type_. */
extern int lt_get_topo_type_depth (enum lt_level_e type);
#endif

#endif /* LIBTOPOLOGY_H */
