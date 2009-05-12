/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef LIBTOPOLOGY_HELPER_H
#define LIBTOPOLOGY_HELPER_H

#include <config.h>
#include <libtopology/private.h>


#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
extern void topo_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, topo_topology_t topology);
extern void topo_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, topo_topology_t topology);
#endif /* LINUX_SYS || HAVE_LIBKSTAT */
#if defined(LINUX_SYS)
extern void topo_setup_cache_level(int cachelevel, int procid_max, unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes, topo_topology_t topology);
extern void look_linux(struct topo_topology *topology);
extern int topo_set_fsys_root(struct topo_topology *topology, const char *fsys_root_path);
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

extern int topo_synthetic_parse_description(struct topo_topology *topology, const char *description);
extern void topo_synthetic_load (struct topo_topology *topology);

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
		__l->admin_disabled = 0;		\
		__l->physical_index = _index;		\
	} while (0)


#define topo_setup_machine_object(l) do {				\
		struct topo_obj **__p = (l);				\
		struct topo_obj *__l1 = &(__p[0][0]);			\
		struct topo_obj *__l2 = &(__p[0][1]);			\
		topo_setup_object(__l1, TOPO_OBJ_MACHINE, 0);		\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		topo_cpuset_fill(&__l1->cpuset);			\
		topo_cpuset_zero(&__l2->cpuset);			\
  } while (0)


#define topo_object_cpuset_from_array(l, _value, _array, _max) do {	\
		struct topo_obj *__l = (l);				\
		unsigned int *__a = (_array);				\
		int k;							\
		for(k=0; k<_max; k++)					\
			if (__a[k] == _value)				\
				topo_cpuset_set(&__l->cpuset, k);	\
	} while (0)


#endif /* LIBTOPOLOGY_HELPER_H */
