/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_H
#define LIBTOPOLOGY_H

#define LINUX_SYS
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libtopology/allocator.h>
#include <libtopology/cpuset.h>

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
	signed os_node;			/**< \brief OS-provided node number */
	signed os_die;			/**< \brief OS-provided die number */
	signed os_l3;			/**< \brief OS-provided L3 number */
	signed os_l2;			/**< \brief OS-provided L2 number */
	signed os_core;			/**< \brief OS-provided core number */
	signed os_l1;			/**< \brief OS-provided L1 number */
	signed os_cpu;			/**< \brief OS-provided CPU number */

	lt_cpuset_t cpuset;		/**< \brief CPUs covered by this level */

	unsigned arity;			/**< \brief Number of children */
	struct lt_level **children;	/**< \brief Children, children[0 .. arity -1] */
	struct lt_level *father;	/**< \brief Father, NULL if root (machine level) */

	unsigned long memory_kB[LT_LEVEL_MEMORY_TYPE_MAX];
        unsigned long huge_page_free;

	/* allocated by ma_per_level_alloc() */
	char data[LT_PER_LEVEL_ROOM];
};

typedef struct lt_level lt_level_t;


#define lt_set_empty_os_numbers(l) do { \
		struct lt_level *___l = (l); \
		___l->os_node = -1; \
		___l->os_die  = -1;  \
		___l->os_l3   = -1;  \
		___l->os_l2   = -1;  \
		___l->os_core = -1; \
		___l->os_l1   = -1;  \
		___l->os_cpu  = -1;  \
	} while(0)

#define lt_set_os_numbers(l, _field, _val) do { \
		struct lt_level *__l = (l); \
		lt_set_empty_os_numbers(l); \
		__l->os_##_field = _val; \
	} while(0)

#define lt_setup_level(l, _type) do {	       \
		struct lt_level *__l = (l);    \
		__l->type = _type;		\
		lt_cpuset_zero(&__l->cpuset);	\
		__l->arity = 0;			\
		__l->children = NULL;		\
		__l->father = NULL;		\
	} while (0)


#define lt_setup_machine_level(l) do {					\
		struct lt_level **__p = (l);                            \
		struct lt_level *__l1 = &(__p[0][0]);			\
		struct lt_level *__l2 = &(__p[0][1]);			\
		__l1->type = LT_LEVEL_MACHINE;				\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		__l1->os_node = -1;					\
		__l1->os_die = -1;					\
		__l1->os_l3 = -1;					\
		__l1->os_l2 = -1;					\
		__l1->os_core = -1;					\
		__l1->os_l1 = -1;					\
		__l1->os_cpu = -1;					\
		lt_cpuset_fill(&__l1->cpuset);				\
		__l1->arity = 0;					\
		__l1->children = NULL;					\
		__l1->father = NULL;					\
		lt_cpuset_zero(&__l2->cpuset);				\
  } while (0)


#define lt_level_cpuset_from_array(l, _value, _array, _max) do { \
		struct lt_level *__l = (l); \
		unsigned int *__a = (_array); \
		int k; \
		for(k=0; k<_max; k++) \
			if (__a[k] == _value) \
				lt_cpuset_set(&__l->cpuset, k); \
	} while (0)


struct lt_topo {
  unsigned nb_processors; 				/* Total number of physical processors */
  unsigned nb_nodes; 					/* Number of NUMA nodes */
  unsigned nb_levels;					/* Number of horizontal levels */
  unsigned level_nbitems[LT_LEVEL_MAX]; 		/* Number of items on each horizontal level */
  struct lt_level *levels[LT_LEVEL_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbitems[l]] */
  int fsys_root_fd;					/* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */
  int discovering_level;
  int type_depth[LT_LEVEL_MAX];
};

typedef struct lt_topo lt_topo_t;

#define lt_setup_topo(t) do {						\
		lt_topo_t *__t = (t);                                   \
		__t->nb_processors = 0;					\
		__t->nb_nodes = 0;					\
		__t->nb_levels = 0;					\
		__t->fsys_root_fd = -1;					\
		__t->discovering_level = 1;				\
		__t->levels[0] = malloc (2 * sizeof (struct lt_level));	\
		lt_setup_machine_level (&(__t->levels[0]));		\
  } while (0)


/** \brief indexes into ::lt_levels, but available from application */
lt_level_t *lt_level(unsigned level, unsigned index);

/** \brief return a stringified topology level type */
const char * lt_level_string(enum lt_level_e l);

/** \brief print a human-readable form of the given topology level */
void lt_print_level(lt_topo_t *topology, struct lt_level *l, FILE *output, int verbose_mode, const char *separator,
		    const char *indexprefix, const char* labelseparator, const char* levelterm);

/** \brief Returns the common father level to levels lvl1 and lvl2 */
lt_level_t *lt_topo_common_ancestor (lt_level_t *lvl1, lt_level_t *lvl2);

/** \brief Returns true if _level_ is inside the subtree beginning
    with _subtree_root_. */
int lt_topo_is_in_subtree (lt_level_t *subtree_root, lt_level_t *level);


/** \brief Returns the depth of levels of type _type_. If no level of
    this type is present on the underlying architecture, the function
    returns the depth of the first "present" level we find uppon
    _type_. */
extern int lt_get_topo_type_depth (enum lt_level_e type);

extern int lt_set_fsys_root(const char *path, lt_topo_t *topology);
extern int topo_init (lt_topo_t *topology);
extern void topo_fini (lt_topo_t *topology);

#endif /* LIBTOPOLOGY_H */
