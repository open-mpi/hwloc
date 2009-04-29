/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef LIBTOPOLOGY_HELPER_H
#define LIBTOPOLOGY_HELPER_H

#include <config.h>
#include <libtopology/private.h>


#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
extern void lt_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, topo_topology_t topology);
extern void lt_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, topo_topology_t topology);
#endif /* LINUX_SYS || HAVE_LIBKSTAT */
#if defined(LINUX_SYS)
extern void lt_setup_cache_level(int cachelevel, enum lt_level_e topotype, int procid_max, unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes, topo_topology_t topology);
extern void look_linux(topo_topology_t topology, lt_cpuset_t *offline_cpus_set);
extern void topo__linux_get_dmi_info(struct topo_topology *topology);
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

#ifdef __GLIBC__
#if (__GLIBC__ > 2) || (__GLIBC_MINOR__ >= 4)
# define HAVE_OPENAT
#endif
#endif

#ifdef HAVE_OPENAT

/* Use our own filesystem functions.  */
#define lt_fopen(p, m, d)   lt_fopenat(p, m, d)
#define lt_access(p, m, d)  lt_accessat(p, m, d)
#define lt_opendir(p, d)    lt_opendirat(p, d)

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

extern FILE * lt_fopenat(const char *path, const char *mode, int fsys_root_fd);
extern int lt_accessat(const char *path, int mode, int fsys_root_fd);
extern DIR* lt_opendirat(const char *path, int fsys_root_fd);

#else /* !HAVE_OPENAT */

#define lt_fopen(p, m, d)   fopen(p, m)
#define lt_access(p, m, d)  access(p, m)
#define lt_opendir(p, d)    opendir(p)

#endif /* !HAVE_OPENAT */


#define lt_set_empty_os_numbers(l) do { \
		struct lt_level *___l = (l); \
		int i; \
		for(i=0; i<LT_LEVEL_MAX; i++) \
		  ___l->physical_index[i] = -1; \
	} while(0)

#define lt_set_os_numbers(l, _type, _val) do { \
		struct lt_level *__l = (l); \
		lt_set_empty_os_numbers(l); \
		__l->physical_index[_type] = _val; \
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
		lt_setup_level(__l1, LT_LEVEL_MACHINE);			\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		lt_set_empty_os_numbers(__l1);                          \
		__l1->physical_index[LT_LEVEL_MACHINE] = 0;             \
		lt_cpuset_fill(&__l1->cpuset);				\
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


#define lt_setup_topo(t) do {						\
		struct topo_topology * __t = (t);                       \
		__t->nb_processors = 0;					\
		__t->nb_nodes = 0;					\
		__t->nb_levels = 1; /* there's at least MACHINE */	\
		__t->fsys_root_fd = -1;					\
		__t->huge_page_size_kB = 0;				\
		__t->dmi_board_vendor = NULL;				\
		__t->dmi_board_name = NULL;				\
		__t->level_nbitems[0] = 1;                              \
		__t->levels[0] = malloc (2*sizeof (struct lt_level));	\
		lt_setup_machine_level (&(__t->levels[0]));		\
  } while (0)

#endif /* LIBTOPOLOGY_HELPER_H */
