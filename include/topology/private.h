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

/* Internal types and helpers. */

#ifndef TOPOLOGY_PRIVATE_H
#define TOPOLOGY_PRIVATE_H

#include <config.h>
#include <topology.h>
#include <topology/private.h>
#include <topology/cpuset.h>
#include <topology/debug.h>

#include <assert.h>

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


#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif


extern void topo_setup_proc_level(struct topo_topology *topology, unsigned nb_processors, topo_cpuset_t *online_cpuset);
extern unsigned topo_fallback_nbprocessors(void);

#if defined(LINUX_SYS)
extern void topo_look_linux(struct topo_topology *topology);
extern int topo_backend_sysfs_init(struct topo_topology *topology, const char *fsys_root_path);
extern void topo_backend_sysfs_exit(struct topo_topology *topology);
#endif /* LINUX_SYS */

#ifdef HAVE_XML
extern int topo_backend_xml_init(struct topo_topology *topology, const char *xmlpath);
extern void topo_look_xml(struct topo_topology *topology);
extern void topo_backend_xml_exit(struct topo_topology *topology);
#endif /* HAVE_XML */

#ifdef HAVE_LIBLGRP
extern void topo_look_lgrp(topo_topology_t topology);
#endif /* HAVE_LIBLGRP */
#ifdef HAVE_LIBKSTAT
extern void topo_look_kstat(topo_topology_t topology);
#endif /* HAVE_LIBKSTAT */

#ifdef AIX_SYS
extern void topo_look_aix(struct topo_topology *topology);
#endif /* AIX_SYS */

#ifdef OSF_SYS
extern void topo_look_osf(struct topo_topology *topology);
#endif /* OSF_SYS */

#ifdef WIN_SYS
extern void topo_look_windows(struct topo_topology *topology);
#endif /* WIN_SYS */

#ifdef DARWIN_SYS
extern void topo_look_darwin(struct topo_topology *topology);
#endif /* DARWIN_SYS */

extern int topo_backend_synthetic_init(struct topo_topology *topology, const char *description);
extern void topo_backend_synthetic_exit(struct topo_topology *topology);
extern void topo_look_synthetic (struct topo_topology *topology);

extern void topo_add_object(struct topo_topology *topology, topo_obj_t obj);


/** \brief Return a locally-allocated stringified cpuset for printf-like calls. */
#define TOPO_CPUSET_PRINTF_VALUE(x)	({					\
	char *__buf = alloca(TOPO_CPUSET_STRING_LENGTH+1);			\
	topo_cpuset_snprintf(__buf, TOPO_CPUSET_STRING_LENGTH+1, x);		\
	__buf;									\
     })

#define topo_setup_object(l, _type, _index) do {	\
		struct topo_obj *__l = (l);		\
		__l->type = _type;			\
		topo_cpuset_zero(&__l->cpuset);		\
		__l->arity = 0;				\
		__l->children = NULL;			\
		__l->first_child = NULL;		\
		__l->next_sibling = NULL;		\
		__l->prev_sibling = NULL;		\
		__l->next_cousin = NULL;		\
		__l->prev_cousin = NULL;		\
		__l->father = NULL;			\
		__l->physical_index = _index;		\
		__l->userdata = NULL;			\
	} while (0)

static inline struct topo_obj *
topo_alloc_setup_object(enum topo_obj_type_e type, unsigned index)
{
  struct topo_obj *obj = malloc(sizeof(*obj));
  assert(obj);
  topo_setup_object(obj, type, index);
  return obj;
}

#define topo_object_cpuset_from_array(l, _value, _array, _max) do {	\
		struct topo_obj *__l = (l);				\
		unsigned int *__a = (_array);				\
		int k;							\
		for(k=0; k<_max; k++)					\
			if (__a[k] == _value)				\
				topo_cpuset_set(&__l->cpuset, k);	\
	} while (0)

/* Configures an array of NUM objects of type TYPE with physical IDs OSPHYSIDS
 * and for which processors have ID PROC_PHYSIDS, and add them to the topology.
 * */
static __inline__ void
topo_setup_level(int procid_max, unsigned num, unsigned *osphysids, unsigned *proc_physids, struct topo_topology *topology, enum topo_obj_type_e type)
{
  struct topo_obj *obj;
  int j;

  topo_debug("%d %s\n", num, topo_obj_type_string(type));

  for (j = 0; j < num; j++)
    {
      obj = topo_alloc_setup_object(type, osphysids[j]);
      topo_object_cpuset_from_array(obj, j, proc_physids, procid_max);
      topo_debug("%s %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 topo_obj_type_string(type),
		 j, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
      topo_add_object(topology, obj);
    }
  topo_debug("\n");
}

#endif /* TOPOLOGY_PRIVATE_H */
