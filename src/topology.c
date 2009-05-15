/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#define _ATFILE_SOURCE
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <topology.h>
#include <topology/private.h>
#include <topology/helper.h>
#include <topology/debug.h>
#include <topology/allocator.h>

#ifdef WIN_SYS
#include <windows.h>
#endif

/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `topo_set_fsys_root ()' to
   have the desired effect.  */
static unsigned
topo_fallback_nbprocessors(void) {
	/* TODO: change into HAVE_SC_foobar, for OSes that don't provide the macro version */
#if defined(_SC_NPROCESSORS_ONLN)
  return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
  return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
  return sysconf(_SC_NPROC_CONF);
	/* TODO: change into HAVE_HOST_INFO */
#elif defined(DARWIN_SYS)
  struct host_basic_info info;
  mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
  host_info(mach_host_self(), HOST_BASIC_INFO, (integer_t*) &info, &count);
  return info.avail_cpus;
#elif defined(WIN_SYS)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
#warning No known way to discover number of available processors on this system
#warning topo_fallback_nbprocessors will default to 1
  return 1;
#endif
}


/* is it really useful to try to disable these two not so big static functions?
 */
#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
void
topo_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, struct topo_topology *topology)
{
  struct topo_obj **die_level;
  int j;

  topo_debug("%d dies\n", numdies);
  die_level = calloc(numdies+1, sizeof(*die_level));
  assert(die_level);

  for (j = 0; j < numdies; j++)
    {
      die_level[j] = malloc(sizeof(struct topo_obj));
      assert(die_level[j]);
      topo_setup_object(die_level[j], TOPO_OBJ_DIE, osphysids[j]);
      topo_object_cpuset_from_array(die_level[j], j, proc_physids, procid_max);
      topo_debug("die %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 j, TOPO_CPUSET_PRINTF_VALUE(die_level[j]->cpuset));
    }
  topo_debug("\n");

  topology->level_nbitems[topology->nb_levels]=numdies;
  topo_debug("--- die level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=die_level;
  topo_debug("\n");
}

void
topo_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, struct topo_topology *topology)
{
  struct topo_obj **core_level;
  int j;

  topo_debug("%d cores\n", numcores);
  core_level = calloc(numcores+1, sizeof(*core_level));
  assert(core_level);

  for (j = 0; j < numcores; j++)
    {
      core_level[j] = malloc(sizeof(struct topo_obj));
      assert(core_level[j]);
      topo_setup_object(core_level[j], TOPO_OBJ_CORE, oscoreids[j]);
      topo_object_cpuset_from_array(core_level[j], j, proc_coreids, procid_max);
      topo_debug("core %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 j, TOPO_CPUSET_PRINTF_VALUE(core_level[j]->cpuset));
    }

  topo_debug("\n");

  topology->level_nbitems[topology->nb_levels]=numcores;
  topo_debug("--- core level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=core_level;
  topo_debug("\n");
}
#endif /* LINUX_SYS || HAVE_LIBKSTAT */

#if defined(LINUX_SYS)
void
topo_setup_cache_level(int cachelevel, int procid_max,
		       unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes,
		       struct topo_topology *topology)
{
  struct topo_obj **level;
  int j;

  topo_debug("%d L%d caches\n", numcaches[cachelevel], cachelevel+1);
  level = calloc((numcaches[cachelevel]+1), sizeof(*level));
  assert(level);

  for (j = 0; j < numcaches[cachelevel]; j++)
    {
      level[j] = malloc(sizeof(struct topo_obj));
      assert(level[j]);
      topo_setup_object(level[j], TOPO_OBJ_CACHE, j);
      level[j]->memory_kB = cachesizes[cachelevel*TOPO_NBMAXCPUS+j];
      level[j]->cache_depth = cachelevel+1;

      topo_object_cpuset_from_array(level[j], j, &cacheids[cachelevel*TOPO_NBMAXCPUS], procid_max);

      topo_debug("L%d cache %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 cachelevel+1, j, TOPO_CPUSET_PRINTF_VALUE(level[j]->cpuset));
    }
  topo_debug("\n");

  topology->level_nbitems[topology->nb_levels]=numcaches[cachelevel];
  topo_debug("--- shared L%d level has number %d\n", cachelevel+1, topology->nb_levels);
  topology->levels[topology->nb_levels++]=level;
  topo_debug("\n");
}
#endif /* LINUX_SYS */

/* Use the value stored in topology->nb_processors.  */
static void
look_cpu(struct topo_topology *topology)
{
  struct topo_obj **cpu_level;
  unsigned oscpu,cpu;

  cpu_level = calloc(topology->nb_processors+1, sizeof(*cpu_level));
  assert(cpu_level);

  topo_debug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; oscpu++)
    {
      if (!topo_cpuset_isset(&topology->online_cpuset, oscpu))
	continue;

      if ((!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE)
	   && topo_cpuset_isset(&topology->admin_disabled_cpuset, oscpu))
	  || ((topology->flags & TOPO_FLAGS_IGNORE_THREADS)
	      && topo_cpuset_isset(&topology->nonfirst_threads_cpuset, oscpu))) {
	topology->nb_processors--;
	continue;
      }

      cpu_level[cpu] = malloc(sizeof(struct topo_obj));
      assert(cpu_level[cpu]);
      topo_setup_object(cpu_level[cpu], TOPO_OBJ_PROC, oscpu);

      topo_cpuset_cpu(&cpu_level[cpu]->cpuset, oscpu);
      if (!(topo_cpuset_isset(&topology->admin_disabled_cpuset, oscpu)))
	cpu_level[cpu]->admin_disabled = 1;

      topo_debug("cpu %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(cpu_level[cpu]->cpuset));
      cpu++;
    }

  topology->level_nbitems[topology->nb_levels]=topology->nb_processors;
  topology->levels[topology->nb_levels++]=cpu_level;
}


/* Connect levels */
static void
topo_connect(struct topo_topology *topology)
{
  int l, i, j, m;
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  if (topology->levels[l][i]->arity)
	    {
	      topo_debug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" arity %u\n",
			 l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i]->cpuset), topology->levels[l][i]->arity);
	      topology->levels[l][i]->children=malloc(topology->levels[l][i]->arity*sizeof(void *));
	      assert(topology->levels[l][i]->children);

	      m=0;
	      for (j=0; j<topology->level_nbitems[l+1]; j++)
		if (topo_cpuset_isincluded(&topology->levels[l][i]->cpuset, &topology->levels[l+1][j]->cpuset))
		  {
		    assert(!(m >= topology->levels[l][i]->arity));
		    topology->levels[l][i]->children[m] = topology->levels[l+1][j];
		    topology->levels[l+1][j]->father = topology->levels[l][i];
		    topology->levels[l+1][j]->index = m++;
		  }
	    }
	}
    }
}

/* Remove level at depth _depth_ */
static void
topo_remove_level(struct topo_topology *topology, unsigned depth)
{
  free (topology->levels[depth]);
  memmove (&topology->levels[depth], &topology->levels[depth+1],
	   (topology->nb_levels-1-depth) * sizeof(topology->levels[depth]));
  memmove (&topology->level_nbitems[depth], &topology->level_nbitems[depth+1],
	   (topology->nb_levels-1-depth) * sizeof(topology->level_nbitems[depth]));
  topology->nb_levels--;
}

static int
compar(const void *_l1, const void *_l2)
{
  const struct topo_obj *l1 = *(struct topo_obj **)_l1;
  const struct topo_obj *l2 = *(struct topo_obj **)_l2;
  int first1 = topo_cpuset_first(&l1->cpuset);
  int first2 = topo_cpuset_first(&l2->cpuset);
  /* if empty, return a bit after the last bit of cpuset */
  if (first1 < 0) first1 = TOPO_NBMAXCPUS;
  if (first2 < 0) first2 = TOPO_NBMAXCPUS;
  return first1 - first2;
}

/* Main discovery loop */
static void
topo_discover(struct topo_topology *topology)
{
  unsigned l,i=0,j;

  assert(topology!=NULL);

  /* Initialize the number of processor to some reasonable default, e.g.,
     obtained using sysconf(3).  */
  topology->nb_processors = topo_fallback_nbprocessors ();

  /* Raw detection, from coarser levels to finer levels */
  unsigned k;

#    ifdef LINUX_SYS
  look_linux(topology);
#    endif /* LINUX_SYS */

#    ifdef  AIX_SYS
  look_aix(topology);
#    endif /* AIX_SYS */

#    ifdef  OSF_SYS
  look_osf(topology);
#    endif /* OSF_SYS */

#    ifdef HAVE_LIBLGRP
  look_lgrp(topology);
#    endif /* HAVE_LIBLGRP */
#    ifdef HAVE_LIBKSTAT
  look_kstat(topology);
#    endif /* HAVE_LIBKSTAT */

#    ifdef  WIN_SYS
  look_windows(topology);
#    endif /* WIN_SYS */

  /* From here, topology->nb_processors is set to the number of available
   * hardware resources, and topology->online_cpuset covers them.
   * Administrator-disabled resources (such as Linux Cpusets) are not
   * removed yet. Disabled cpus are in topology->admin_disabled_cpuset,
   * while disabled mems are marked as admin-disabled.
   */
  topo_debug("%d online processors found\n", topology->nb_processors);
  topo_debug("online processor cpuset: %" TOPO_PRIxCPUSET "\n",
	     TOPO_CPUSET_PRINTF_VALUE(topology->online_cpuset));
  topo_debug("admin disabled processor cpuset: %" TOPO_PRIxCPUSET "\n",
	     TOPO_CPUSET_PRINTF_VALUE(topology->admin_disabled_cpuset));
  assert(topology->nb_processors == topo_cpuset_weight(&topology->online_cpuset));

  /* Create actual bottom proc resources, while dropping the offline
   * and admin_disabled ones.
   */
  look_cpu(topology);
  /* topology->nb_processors now does not contain admin-disabled procs anymore. */
  assert(topology->nb_processors);

  topo_debug("\n\n--> discovered %d levels\n\n", topology->nb_levels);

  /* Ignore some levels if requested */
  l=0;
  while (l<topology->nb_levels) {
    enum topo_obj_type_e type = topology->levels[l][0]->type;
    enum topo_ignore_type_e ignore = topology->ignored_types[type];

    /* ignore if ALWAYS,
     * or ignore if KEEP_STRUCTURE and parent object is similar.
     */
    if (ignore == TOPO_IGNORE_TYPE_ALWAYS
	|| (ignore == TOPO_IGNORE_TYPE_KEEP_STRUCTURE
	    && l > 0
	    && topology->level_nbitems[l-1] == topology->level_nbitems[l])) {
      topo_remove_level(topology, l);
    } else {
      l++;
    }
  }

  /* Sort levels */
  /* FIXME: We only sort according to level_nbitems.
     It assumes that levels are fully filled with "identical" objects.
     In case of irregular architectures (one CPU with different cache levels),
     we might have to break levels, ...
  */
  for (l=0; l<topology->nb_levels; l++) {
    /* only CACHE maybe wrongly ordered so far */
    if (topology->levels[l][0]->type != TOPO_OBJ_CACHE)
      continue;

    /* find how much to move backwards */
    for (i=0; i<l; i++) {
      if (topology->level_nbitems[i] > topology->level_nbitems[l]) {
	/* move l before i */
	struct topo_obj **saved_level = topology->levels[l];
	unsigned saved_nbitems = topology->level_nbitems[l];
	topo_debug("moving level %d (%d items) before %d (%d items)\n",
		   l, saved_nbitems, i, topology->level_nbitems[i]);
	memmove(&topology->level_nbitems[i+1], &topology->level_nbitems[i], (l-i)*sizeof(topology->level_nbitems[i]));
	memmove(&topology->levels[i+1], &topology->levels[i], (l-i)*sizeof(topology->levels[i]));
	topology->levels[i] = saved_level;
	topology->level_nbitems[i] = saved_nbitems;
	break;
      }
    }
  }

  /* Compute the machine cpuset */
  topo_cpuset_zero(&topology->levels[0][0]->cpuset);
  for (i=0; i<topology->level_nbitems[topology->nb_levels-1]; i++)
    topo_cpuset_orset(&topology->levels[0][0]->cpuset, &topology->levels[topology->nb_levels-1][i]->cpuset);
  /* Make sure machine = online & ~admin_disabled */
  topo_cpuset_t finalset = topology->online_cpuset;
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE))
    topo_cpuset_clearset(&finalset, &topology->admin_disabled_cpuset);
  if (topology->flags & TOPO_FLAGS_IGNORE_THREADS)
    topo_cpuset_clearset(&finalset, &topology->nonfirst_threads_cpuset);
  assert(topo_cpuset_isequal(&finalset, &topology->levels[0][0]->cpuset));

  /* Remove disabled/offline CPUs from all cpusets, use the now correct machine cpuset to do so,
   * Then sort levels according to cpu sets, removed empty levels, recount levels, ...
   * No need to look at machine and CPUs, they just got generated correctly.
   */
  for (l=1; l<topology->nb_levels; l++) {
    /* update cpusets */
    for (i=0; i<topology->level_nbitems[l]; i++)
      topo_cpuset_andset(&topology->levels[l][i]->cpuset, &topology->levels[0][0]->cpuset);

    /* sort sublevels according to cpusets */
    qsort(&topology->levels[l][0], topology->level_nbitems[l], sizeof(topology->levels[l]), compar);

    /* update level_nbitems by removing the empty ones (they are last), except for NUMA node since we want to keep memory information */
    if (topology->levels[l][0]->type != TOPO_OBJ_NODE) {
      for (i=0; i<topology->level_nbitems[l]; i++)
	if (topo_cpuset_iszero(&topology->levels[l][i]->cpuset)) {
	  /* free remaining elements, clear first freed one, and update nbitems */
	  for(j=i; j<topology->level_nbitems[l]; j++)
	    free(topology->levels[l][j]);
	  topology->levels[l][i] = NULL;
	  topology->level_nbitems[l] = i;
	  break;
        }
    }
    topo_debug("%d levels remaining at depth %d after filtering\n",
	       topology->level_nbitems[l], l);
  }

  /* Gather sublevels according to levels */
  for (l=0; l+1<topology->nb_levels; l++) {
    k = 0;
    for (i=0; i<topology->level_nbitems[l]; i++) {
      topo_cpuset_t level_set = topology->levels[l][i]->cpuset;
      for (j=k; j<topology->level_nbitems[l+1]; j++) {
	topo_cpuset_t set = level_set;
	topo_cpuset_andset(&set, &topology->levels[l+1][j]->cpuset);
	if (!topo_cpuset_iszero(&set)) {
	  /* Sublevel j is part of level i, put it at k.  */
	  struct topo_obj *obj = topology->levels[l+1][j];
	  memmove(&topology->levels[l+1][k+1], &topology->levels[l+1][k], (j-k)*sizeof(*topology->levels[l+1]));
	  topology->levels[l+1][k++] = obj;
	}
      }
    }
  }

  /* Now we can put numbers on levels. */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      {
	topology->levels[l][i]->number = i;
	topo_debug("level %u,%u: cpuset %"TOPO_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i]->cpuset));
      }

  /* And show debug again */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      topo_debug("level %u,%u: cpuset %"TOPO_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i]->cpuset));

  /* Compute arity */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  topology->levels[l][i]->arity=0;
	  for (j=0; j<topology->level_nbitems[l+1]; j++)
	    if (topo_cpuset_isincluded(&topology->levels[l][i]->cpuset, &topology->levels[l+1][j]->cpuset))
	      topology->levels[l][i]->arity++;
	  topo_debug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" arity %u\n",
		     l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i]->cpuset), topology->levels[l][i]->arity);
	}
    }


  for (i=0; i<topology->level_nbitems[topology->nb_levels-1]; i++)
    topo_debug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" leaf\n",
	       topology->nb_levels-1, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[topology->nb_levels-1][i]->cpuset));
  topo_debug("arity done.\n");

  /* and finally connect levels */
  topo_connect(topology);
  topo_debug("connecting done.\n");

  /* intialize all depth to unknown */
  for (l=0; l < TOPO_OBJ_TYPE_MAX; l++)
    topology->type_depth[l] = TOPO_TYPE_DEPTH_UNKNOWN;
  /* walk the existing levels to set their depth */
  for (l=0; l<topology->nb_levels; l++) {
    enum topo_obj_type_e type = topology->levels[l][0]->type;
    if (topology->type_depth[type] == TOPO_TYPE_DEPTH_UNKNOWN) {
      topology->type_depth[type] = l;
    } else {
      assert(type >= TOPO_OBJ_ORDERED_TYPE_MAX);
      topology->type_depth[type] = TOPO_TYPE_DEPTH_MULTIPLE; /* mark as unknown */
    }
  }
  /* setup the depth of all still unknown levels (the one that got merged or never created */
  int type, prevdepth = TOPO_TYPE_DEPTH_UNKNOWN;
  for (type = 0; type < TOPO_OBJ_ORDERED_TYPE_MAX; type++)
    {
      if (topology->type_depth[type] == TOPO_TYPE_DEPTH_UNKNOWN) {
	topology->type_depth[type] = prevdepth;
      } else {
	prevdepth = topology->type_depth[type];
      }
    }

  /* set level depth */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      topology->levels[l][i]->level = l;

  /* Empty some NUMA node memory if disabled by the administrator */
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE)) {
    l = topology->type_depth[TOPO_OBJ_NODE];
    for (i=0; i<topology->level_nbitems[l]; i++) {
      /* remove memory if disabled */
      if (topology->levels[l][i]->admin_disabled) {
	topology->levels[l][i]->memory_kB = 0;
	topology->levels[l][i]->huge_page_free = 0;
      }
    }
  }
}

int
topo_topology_init (struct topo_topology **topologyp)
{
  struct topo_topology *topology;
  int i;

  topology = topo_default_allocator.allocate (sizeof (struct topo_topology));
  if(!topology)
    return -1;

  topology->nb_processors = 0;
  topology->nb_nodes = 0;
  topology->nb_levels = 1; /* there's at least MACHINE */
  topo_cpuset_zero(&topology->online_cpuset);
  topo_cpuset_zero(&topology->admin_disabled_cpuset);
  topo_cpuset_zero(&topology->nonfirst_threads_cpuset);
  topology->flags = 0;
  topology->is_fake = 0;
  topology->backend_type = TOPO_BACKEND_NONE; /* backend not specified by default */
  topology->huge_page_size_kB = 0;
  topology->dmi_board_vendor = NULL;
  topology->dmi_board_name = NULL;
  for(i=0; i< TOPO_OBJ_TYPE_MAX; i++)
    topology->ignored_types[i] = TOPO_IGNORE_TYPE_NEVER;
  memset(topology->level_nbitems, 0, sizeof(topology->level_nbitems));
  topology->level_nbitems[0] = 1;
  for (i=0; i < TOPO_OBJ_TYPE_MAX; i++)
    topology->type_depth[i] = -1;
  topology->levels[0] = malloc (2*sizeof (struct topo_obj));
  topo_setup_machine_level (topology->levels[0]);

  *topologyp = topology;
  return 0;
}

static void
topo_backend_exit(struct topo_topology *topology)
{
  switch (topology->backend_type) {
#ifdef LINUX_SYS
  case TOPO_BACKEND_SYSFS:
    topo_backend_sysfs_exit(topology);
    break;
#endif
  case TOPO_BACKEND_SYNTHETIC:
    topo_backend_synthetic_exit(topology);
    break;
  default:
    break;
  }

  assert(topology->backend_type == TOPO_BACKEND_NONE);
}

int
topo_topology_set_fsys_root(struct topo_topology *topology, const char *fsys_root_path)
{
  /* cleanup existing backend */
  topo_backend_exit(topology);

#ifdef LINUX_SYS
  if (topo_backend_sysfs_init(topology, fsys_root_path) < 0)
    return -1;
#endif /* LINUX_SYS */

  return 0;
}

int
topo_topology_set_synthetic(struct topo_topology *topology, const char *description)
{
  /* cleanup existing backend */
  topo_backend_exit(topology);

  return topo_backend_synthetic_init(topology, description);
}

int
topo_topology_set_flags (struct topo_topology *topology, unsigned long flags)
{
  topology->flags = flags;
  return 0;
}

int
topo_topology_ignore_type(struct topo_topology *topology, enum topo_obj_type_e type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  if (type == TOPO_OBJ_MACHINE)
    /* we don't want 2 heads */
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_ALWAYS;
  return 0;
}

int
topo_topology_ignore_type_keep_structure(struct topo_topology *topology, topo_obj_type_t type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_ignore_all_keep_structure(struct topo_topology *topology)
{
  unsigned type;
  for(type=0; type<TOPO_OBJ_TYPE_MAX; type++)
    topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_load (struct topo_topology *topology)
{
  if (topology->backend_type == TOPO_BACKEND_NONE) {
    /* if we haven't chosen the backend, set the OS-specific one if needed */
#ifdef LINUX_SYS
    if (topo_backend_sysfs_init(topology, "/") < 0)
      return -1;
#endif
  }

  if (topology->backend_type == TOPO_BACKEND_SYNTHETIC)
    topo_synthetic_load(topology);
  else
    topo_discover(topology);

  return 0;
}

int
topo_topology_get_info(struct topo_topology *topology, struct topo_topology_info *info)
{
  info->nb_processors = topology->nb_processors;
  info->nb_nodes = topology->nb_nodes;
  info->depth = topology->nb_levels;
  info->dmi_board_vendor = topology->dmi_board_vendor;
  info->dmi_board_name = topology->dmi_board_name;
  info->huge_page_size_kB = topology->huge_page_size_kB;
  info->is_fake = topology->is_fake;
  return 0;
}

void
topo_topology_destroy (struct topo_topology *topology)
{
  unsigned l,i;

  for (l=0; l<topology->nb_levels; l++) {
    for (i=0; i<topology->level_nbitems[l]; i++) {
      free(topology->levels[l][i]->children);
      free(topology->levels[l][i]);
    }
    free(topology->levels[l]);
  }

  topo_backend_exit(topology);

  free(topology->dmi_board_vendor);
  free(topology->dmi_board_name);

  topo_default_allocator.free(topology);
}
