/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Internal types. */

#ifndef LIBTOPOLOGY_TYPES_H
#define LIBTOPOLOGY_TYPES_H

#include <libtopology.h>

struct topo_topology {
  unsigned nb_processors; 				/* Total number of physical processors */
  unsigned nb_nodes; 					/* Number of NUMA nodes */
  unsigned nb_levels;					/* Number of horizontal levels */
  unsigned level_nbitems[LT_LEVEL_MAX]; 		/* Number of items on each horizontal level */
  struct lt_level *levels[LT_LEVEL_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbitems[l]] */
  unsigned long flags;
  int fsys_root_fd;					/* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */
  int type_depth[LT_LEVEL_MAX];
  unsigned long huge_page_size_kB;
  char *dmi_board_vendor;
  char *dmi_board_name;
};

#endif /* LIBTOPOLOGY_TYPES_H */
