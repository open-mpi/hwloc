/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Internal types. */

#ifndef LIBTOPOLOGY_TYPES_H
#define LIBTOPOLOGY_TYPES_H

#include <libtopology.h>

enum topo_ignore_type_e {
  TOPO_IGNORE_LEVEL_NEVER = 0,
  TOPO_IGNORE_LEVEL_KEEP_STRUCTURE,
  TOPO_IGNORE_LEVEL_ALWAYS,
};

struct topo_topology {
  unsigned nb_processors; 				/* Total number of physical processors */
  unsigned nb_nodes; 					/* Number of NUMA nodes */
  unsigned nb_levels;					/* Number of horizontal levels */
  topo_cpuset_t online_cpuset;				/* Available physical resource ids mask */
  topo_cpuset_t admin_disabled_cpuset;			/* Available physical resource that are disabled by the administrator */
  topo_cpuset_t nonfirst_threads_cpuset;		/* Thread siblings that are not the first one in a cpu */
  unsigned level_nbitems[TOPO_LEVEL_MAX]; 		/* Number of items on each horizontal level */
  struct topo_level *levels[TOPO_LEVEL_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbitems[l]] */
  unsigned long flags;
  int type_depth[TOPO_LEVEL_MAX];
  enum topo_ignore_type_e ignored_types[TOPO_LEVEL_MAX];
  unsigned long huge_page_size_kB;
  char *dmi_board_vendor;
  char *dmi_board_name;
  int is_fake;

  /* FIXME: move this to proper backend structures */

  /* sysfs backend parameters */
  int fsys_root_fd;					/* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */

  /* synthetic backend parameters */
  int use_synthetic;
#define TOPO_SYNTHETIC_MAX_DEPTH 128
  unsigned synthetic_description[TOPO_SYNTHETIC_MAX_DEPTH];
};

#endif /* LIBTOPOLOGY_TYPES_H */
