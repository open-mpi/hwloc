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

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef TOPOLOGY_HELPER_H
#define TOPOLOGY_HELPER_H

#include <config.h>
#include <topology/private.h>
#include <topology/cpuset.h>
#include <topology/debug.h>

#include <assert.h>

extern void topo_setup_proc_level(struct topo_topology *topology, topo_cpuset_t *online_cpuset);

#if defined(LINUX_SYS)
#if 0
extern void topo_setup_cache_level(int cachelevel, int procid_max, unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes, topo_topology_t topology);
#endif
extern void topo_look_linux(struct topo_topology *topology);
extern int topo_backend_sysfs_init(struct topo_topology *topology, const char *fsys_root_path);
extern void topo_backend_sysfs_exit(struct topo_topology *topology);
#endif /* LINUX_SYS */

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

extern int topo_backend_synthetic_init(struct topo_topology *topology, const char *description);
extern void topo_backend_synthetic_exit(struct topo_topology *topology);
extern void topo_synthetic_load (struct topo_topology *topology);
extern void topo_add_object(struct topo_topology *topology, topo_obj_t obj);

#ifdef __GLIBC__
#if (__GLIBC__ > 2) || (__GLIBC_MINOR__ >= 4)
# define HAVE_OPENAT
#endif
#endif


#define topo_setup_object(l, _type, _index) do {	\
		struct topo_obj *__l = (l);		\
		__l->type = _type;			\
		topo_cpuset_zero(&__l->cpuset);		\
		__l->arity = 0;				\
		__l->children = NULL;			\
		__l->first_child = NULL;		\
		__l->next_sibling = NULL;		\
		__l->prev_sibling = NULL;		\
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

#define topo_setup_system_level(l) do {					\
		struct topo_obj **__p = (l);				\
		struct topo_obj *__l1;					\
		__l1 = topo_alloc_setup_object(TOPO_OBJ_SYSTEM, 0);	\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		__l1->attr.system.memory_kB = 0;			\
		__l1->attr.system.huge_page_free = 0;			\
		topo_cpuset_fill(&__l1->cpuset);			\
		__p[0] = __l1;						\
		__p[1] = NULL;						\
  } while (0)


#define topo_object_cpuset_from_array(l, _value, _array, _max) do {	\
		struct topo_obj *__l = (l);				\
		unsigned int *__a = (_array);				\
		int k;							\
		for(k=0; k<_max; k++)					\
			if (__a[k] == _value)				\
				topo_cpuset_set(&__l->cpuset, k);	\
	} while (0)

/* Adds an array of NUM objects LEVEL to the topology.  */
static __inline__ void
topo_add_level(struct topo_topology *topology, topo_obj_t *level, unsigned num)
{
  int i;
  for (i = 0; i < num; i++)
    topo_add_object(topology, level[i]);
}

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
		 j, TOPO_CPUSET_PRINTF_VALUE(obj->cpuset));
      topo_add_object(topology, obj);
    }
  topo_debug("\n");
}


#endif /* TOPOLOGY_HELPER_H */
