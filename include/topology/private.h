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

/* Internal types. */

#ifndef TOPOLOGY_TYPES_H
#define TOPOLOGY_TYPES_H

#include <topology.h>

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
#ifdef HAVE_XML
  TOPO_BACKEND_XML,
#endif
} topo_backend_t;

struct topo_topology {
  unsigned nb_levels;					/* Number of horizontal levels */
  unsigned level_nbobjects[TOPO_DEPTH_MAX]; 		/* Number of objects on each horizontal level */
  struct topo_obj **levels[TOPO_DEPTH_MAX];		/* Direct access to levels, levels[l = 0 .. nblevels-1][0..level_nbobjects[l]] */
  unsigned long flags;
  int type_depth[TOPO_OBJ_TYPE_MAX];
  enum topo_ignore_type_e ignored_types[TOPO_OBJ_TYPE_MAX];
  unsigned long huge_page_size_kB;
  char *dmi_board_vendor;
  char *dmi_board_name;
  int is_fake;
  int is_loaded;

  topo_backend_t backend_type;
  union topo_backend_params_u {
#ifdef LINUX_SYS
    struct topo_backend_params_sysfs_s {
      /* sysfs backend parameters */
      int root_fd; /* The file descriptor for the file system root, used when browsing, e.g., Linux' sysfs and procfs. */
    } sysfs;
#endif /* LINUX_SYS */
#ifdef HAVE_XML
    struct topo_backend_params_xml_s {
      /* xml backend parameters */
      void *doc;
    } xml;
#endif /* HAVE_XML */
    struct topo_backend_params_synthetic_s {
      /* synthetic backend parameters */
#define TOPO_SYNTHETIC_MAX_DEPTH 128
      unsigned arity[TOPO_SYNTHETIC_MAX_DEPTH];
      topo_obj_type_t type[TOPO_SYNTHETIC_MAX_DEPTH];
      unsigned id[TOPO_SYNTHETIC_MAX_DEPTH];
      unsigned depth[TOPO_SYNTHETIC_MAX_DEPTH]; /* For cache/misc */
    } synthetic;
  } backend_params;
};

#endif /* TOPOLOGY_TYPES_H */
