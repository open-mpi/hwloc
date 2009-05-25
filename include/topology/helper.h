/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef TOPOLOGY_HELPER_H
#define TOPOLOGY_HELPER_H

#include <config.h>
#include <topology/private.h>
#include <topology/cpuset.h>
#include <topology/debug.h>

#include <assert.h>

void look_cpu(struct topo_topology *topology, topo_cpuset_t *online_cpuset);

#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
extern void topo_setup_socket_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, topo_topology_t topology);
extern void topo_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, topo_topology_t topology);
#endif /* LINUX_SYS || HAVE_LIBKSTAT */
#if defined(LINUX_SYS)
extern void topo_setup_cache_level(int cachelevel, int procid_max, unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes, topo_topology_t topology);
extern void look_linux(struct topo_topology *topology);
extern int topo_backend_sysfs_init(struct topo_topology *topology, const char *fsys_root_path);
extern void topo_backend_sysfs_exit(struct topo_topology *topology);
#endif /* LINUX_SYS */

#ifdef HAVE_LIBLGRP
extern void look_lgrp(topo_topology_t topology);
#endif /* HAVE_LIBLGRP */
#ifdef HAVE_LIBKSTAT
extern void look_kstat(topo_topology_t topology);
#endif /* HAVE_LIBKSTAT */

#ifdef AIX_SYS
extern void look_aix(struct topo_topology *topology);
#endif /* AIX_SYS */

#ifdef OSF_SYS
extern void look_osf(struct topo_topology *topology);
#endif /* OSF_SYS */

#ifdef WIN_SYS
extern void look_windows(struct topo_topology *topology);
#endif /* WIN_SYS */

extern int topo_backend_synthetic_init(struct topo_topology *topology, const char *description);
extern void topo_backend_synthetic_exit(struct topo_topology *topology);
extern void topo_synthetic_load (struct topo_topology *topology);

/* Adds an array of NUM objects LEVEL to the topology.
 * For now, the array must finish with a NULL.  */
static __inline__ void
topo_add_level(struct topo_topology *topology, topo_obj_t *level, unsigned num)
{
  if (level[0]->type == TOPO_OBJ_CACHE)
    topo_debug("--- cache level depth %d", level[0]->cache_depth);
  else
    topo_debug("--- %s level", topo_object_type_string(level[0]->type));
  topo_debug("has number %d\n\n", topology->nb_levels);
  topology->level_nbobjects[topology->nb_levels] = num;
  topology->levels[topology->nb_levels] = level;
  topology->nb_levels++;
}

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
		__l->father = NULL;			\
		__l->physical_index = _index;		\
		__l->userdata = NULL;			\
	} while (0)


#define topo_setup_machine_level(l) do {				\
		struct topo_obj **__p = (l);				\
		struct topo_obj *__l1;					\
		__l1 = malloc(sizeof(struct topo_obj));			\
		assert(__l1);						\
		topo_setup_object(__l1, TOPO_OBJ_MACHINE, 0);		\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		__l1->memory_kB = 0;					\
		__l1->huge_page_free = 0;				\
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


#endif /* TOPOLOGY_HELPER_H */
