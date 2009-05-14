/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Internal types. */

#ifndef LIBTOPOLOGY_TYPES_H
#define LIBTOPOLOGY_TYPES_H

#include <libtopology.h>

enum topo_ignore_type_e {
  TOPO_IGNORE_TYPE_NEVER = 0,
  TOPO_IGNORE_TYPE_KEEP_STRUCTURE,
  TOPO_IGNORE_TYPE_ALWAYS,
};

#define TOPO_DEPTH_MAX 128

typedef enum topo_backend_e {
  TOPO_BACKEND_NONE,
  TOPO_BACKEND_SYNTHETIC,
#ifdef LINUX_SYS
  TOPO_BACKEND_SYSFS,
#endif
} topo_backend_t;

struct topo_topology {
  unsigned nb_processors; 				/* Total number of physical processors */
  unsigned nb_nodes; 					/* Number of NUMA nodes */
  unsigned nb_levels;					/* Number of horizontal levels */
  topo_cpuset_t online_cpuset;				/* Available physical resource ids mask */
  topo_cpuset_t admin_disabled_cpuset;			/* Available physical resource that are disabled by the administrator */
  topo_cpuset_t nonfirst_threads_cpuset;		/* Thread siblings that are not the first one in a cpu */
  unsigned level_nbitems[TOPO_DEPTH_MAX]; 		/* Number of items on each horizontal level */
  struct topo_obj *levels[TOPO_DEPTH_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbitems[l]] */
  unsigned long flags;
  int type_depth[TOPO_OBJ_TYPE_MAX];
  enum topo_ignore_type_e ignored_types[TOPO_OBJ_TYPE_MAX];
  unsigned long huge_page_size_kB;
  char *dmi_board_vendor;
  char *dmi_board_name;
  int is_fake;

  topo_backend_t backend_type;
  union topo_backend_params_u {
#ifdef LINUX_SYS
    struct topo_backend_params_sysfs_s {
      /* sysfs backend parameters */
      int root_fd; /* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */
    } sysfs;
#endif /* LINUX_SYS */
    struct topo_backend_params_synthetic_s {
      /* synthetic backend parameters */
#define TOPO_SYNTHETIC_MAX_DEPTH 128
      unsigned description[TOPO_SYNTHETIC_MAX_DEPTH];
    } synthetic;
  } backend_params;
};

#endif /* LIBTOPOLOGY_TYPES_H */
