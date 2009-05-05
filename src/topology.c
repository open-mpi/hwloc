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

#include <libtopology.h>
#include <libtopology/private.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#ifdef WIN_SYS
#include <windows.h>
#endif

/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `lt_set_fsys_root ()' to
   have the desired effect.  */
static unsigned
lt_fallback_nbprocessors(void) {
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
#warning lt_fallback_nbprocessors will default to 1
  return 1;
#endif
}


/* is it really useful to try to disable these two not so big static functions?
 */
#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
void
lt_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, struct topo_topology *topology)
{
  struct topo_level *die_level;
  int j;

  ltdebug("%d dies\n", numdies);
  die_level=malloc((numdies+1)*sizeof(*die_level));
  assert(die_level);

  for (j = 0; j < numdies; j++)
    {
      lt_setup_level(&die_level[j], TOPO_LEVEL_DIE);
      lt_set_os_numbers(&die_level[j], TOPO_LEVEL_DIE, osphysids[j]);
      lt_level_cpuset_from_array(&die_level[j], j, proc_physids, procid_max);
      ltdebug("die %d has cpuset %"TOPO_PRIxCPUSET"\n",
	      j, TOPO_CPUSET_PRINTF_VALUE(die_level[j].cpuset));
    }
  ltdebug("\n");

  topo_cpuset_zero(&die_level[j].cpuset);

  topology->level_nbitems[topology->nb_levels]=numdies;
  ltdebug("--- die level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=die_level;
  ltdebug("\n");
}

void
lt_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, struct topo_topology *topology)
{
  struct topo_level *core_level;
  int j;

  ltdebug("%d cores\n", numcores);
  core_level=malloc((numcores+1)*sizeof(*core_level));
  assert(core_level);

  for (j = 0; j < numcores; j++)
    {
      lt_setup_level(&core_level[j], TOPO_LEVEL_CORE);
      lt_set_os_numbers(&core_level[j], TOPO_LEVEL_CORE, oscoreids[j]);
      lt_level_cpuset_from_array(&core_level[j], j, proc_coreids, procid_max);
      ltdebug("core %d has cpuset %"TOPO_PRIxCPUSET"\n",
	      j, TOPO_CPUSET_PRINTF_VALUE(core_level[j].cpuset));
    }

  ltdebug("\n");

  topo_cpuset_zero(&core_level[j].cpuset);

  topology->level_nbitems[topology->nb_levels]=numcores;
  ltdebug("--- core level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=core_level;
  ltdebug("\n");
}
#endif /* LINUX_SYS || HAVE_LIBKSTAT */

#if defined(LINUX_SYS)
void
lt_setup_cache_level(int cachelevel, enum topo_level_type_e topotype, int procid_max,
		     unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes,
		     struct topo_topology *topology)
{
  struct topo_level *level;
  int j;

  ltdebug("%d L%d caches\n", numcaches[cachelevel], cachelevel+1);
  level=malloc((numcaches[cachelevel]+1)*sizeof(*level));
  assert(level);

  for (j = 0; j < numcaches[cachelevel]; j++)
    {
      lt_setup_level(&level[j], topotype);

      switch (cachelevel)
	{
	case 2: lt_set_os_numbers(&level[j], TOPO_LEVEL_L3, j); break;
	case 1: lt_set_os_numbers(&level[j], TOPO_LEVEL_L2, j); break;
	case 0: lt_set_os_numbers(&level[j], TOPO_LEVEL_L1, j); break;
	default: assert(!1);
	}

      level[j].memory_kB[TOPO_LEVEL_MEMORY_L1+cachelevel] = cachesizes[cachelevel*LIBTOPO_NBMAXCPUS+j];

      lt_level_cpuset_from_array(&level[j], j, &cacheids[cachelevel*LIBTOPO_NBMAXCPUS], procid_max);

      ltdebug("L%d cache %d has cpuset %"TOPO_PRIxCPUSET"\n",
	      cachelevel+1, j, TOPO_CPUSET_PRINTF_VALUE(level[j].cpuset));
    }
  ltdebug("\n");
  topo_cpuset_zero(&level[j].cpuset);
  topology->level_nbitems[topology->nb_levels]=numcaches[cachelevel];
  ltdebug("--- shared L%d level has number %d\n", cachelevel+1, topology->nb_levels);
  topology->levels[topology->nb_levels++]=level;
  ltdebug("\n");
}
#endif /* LINUX_SYS */

/* Use the value stored in topology->nb_processors.  */
static void
look_cpu(struct topo_topology *topology)
{
  struct topo_level *cpu_level;
  unsigned oscpu,cpu;

  cpu_level=malloc((topology->nb_processors+1)*sizeof(*cpu_level));
  assert(cpu_level);

  ltdebug("\n\n * CPU cpusets *\n\n");
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

      lt_setup_level(&cpu_level[cpu], TOPO_LEVEL_PROC);
      lt_set_os_numbers(&cpu_level[cpu], TOPO_LEVEL_PROC, oscpu);

      topo_cpuset_cpu(&cpu_level[cpu].cpuset, oscpu);
      if (!(topo_cpuset_isset(&topology->admin_disabled_cpuset, oscpu)))
	cpu_level[cpu].admin_disabled = 1;

      ltdebug("cpu %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
	      cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(cpu_level[cpu].cpuset));
      cpu++;
    }
  topo_cpuset_zero(&cpu_level[cpu].cpuset);

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
	  if (topology->levels[l][i].arity)
	    {
	      ltdebug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" arity %u\n",
		      l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	      topology->levels[l][i].children=malloc(topology->levels[l][i].arity*sizeof(void *));
	      assert(topology->levels[l][i].children);

	      m=0;
	      for (j=0; j<topology->level_nbitems[l+1]; j++)
		if (topo_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
		  {
		    assert(!(m >= topology->levels[l][i].arity));
		    topology->levels[l][i].children[m] = &topology->levels[l+1][j];
		    topology->levels[l+1][j].father = &topology->levels[l][i];
		    topology->levels[l+1][j].index = m++;
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
  const struct topo_level *l1 = _l1, *l2 = _l2;
  int first1 = topo_cpuset_first(&l1->cpuset);
  int first2 = topo_cpuset_first(&l2->cpuset);
  /* if empty, return a bit after the last bit of cpuset */
  if (first1 < 0) first1 = LIBTOPO_NBMAXCPUS;
  if (first2 < 0) first2 = LIBTOPO_NBMAXCPUS;
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
  topology->nb_processors = lt_fallback_nbprocessors ();

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
  ltdebug("%d online processors found\n", topology->nb_processors);
  ltdebug("online processor cpuset: %" TOPO_PRIxCPUSET "\n",
	  TOPO_CPUSET_PRINTF_VALUE(topology->online_cpuset));
  ltdebug("admin disabled processor cpuset: %" TOPO_PRIxCPUSET "\n",
	  TOPO_CPUSET_PRINTF_VALUE(topology->admin_disabled_cpuset));
  assert(topology->nb_processors == topo_cpuset_weight(&topology->online_cpuset));

  /* Create actual bottom proc resources, while dropping the offline
   * and admin_disabled ones.
   */
  look_cpu(topology);
  /* topology->nb_processors now does not contain admin-disabled procs anymore. */
  assert(topology->nb_processors);

  ltdebug("\n\n--> discovered %d levels\n\n", topology->nb_levels);

  /* Ignored some levels if requested */
  l=0;
  while (l<topology->nb_levels) {
    enum topo_level_type_e type = topology->levels[l][0].type;
    enum topo_ignore_type_e ignore = topology->ignored_types[type];

    /* ignore if ALWAYS,
     * or ignore if KEEP_STRUCTURE and parent level is similar.
     */
    if (ignore == TOPO_IGNORE_LEVEL_ALWAYS
	|| (ignore == TOPO_IGNORE_LEVEL_KEEP_STRUCTURE
	    && l > 0
	    && topology->level_nbitems[l-1] == topology->level_nbitems[l])) {
      topo_remove_level(topology, l);
    } else {
      l++;
    }
  }

  /* Compute the machine cpuset */
  topo_cpuset_zero(&topology->levels[0][0].cpuset);
  for (i=0; i<topology->level_nbitems[topology->nb_levels-1]; i++)
    topo_cpuset_orset(&topology->levels[0][0].cpuset, &topology->levels[topology->nb_levels-1][i].cpuset);
  /* Make sure machine = online & ~admin_disabled */
  topo_cpuset_t finalset = topology->online_cpuset;
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE))
    topo_cpuset_clearset(&finalset, &topology->admin_disabled_cpuset);
  if (topology->flags & TOPO_FLAGS_IGNORE_THREADS)
    topo_cpuset_clearset(&finalset, &topology->nonfirst_threads_cpuset);
  assert(topo_cpuset_isequal(&finalset, &topology->levels[0][0].cpuset));

  /* Remove disabled/offline CPUs from all cpusets, use the now correct machine cpuset to do so,
   * Then sort levels according to cpu sets, removed empty levels, recount levels, ...
   * No need to look at machine and CPUs, they just got generated correctly.
   */
  for (l=1; l<topology->nb_levels; l++) {
    /* update cpusets */
    for (i=0; i<topology->level_nbitems[l]; i++)
      topo_cpuset_andset(&topology->levels[l][i].cpuset, &topology->levels[0][0].cpuset);

    /* sort sublevels according to cpusets */
    qsort(&topology->levels[l][0], topology->level_nbitems[l], sizeof(*topology->levels[l]), compar);

    /* update level_nbitems by removing the empty ones (they are last), except for NUMA node since we want to keep memory information */
    if (topology->levels[l][0].type != TOPO_LEVEL_NODE) {
      for (i=0; i<topology->level_nbitems[l]; i++)
	if (topo_cpuset_iszero(&topology->levels[l][i].cpuset)) {
	  topology->level_nbitems[l] = i;
	  break;
        }
    }
    ltdebug("%d levels remaining at depth %d after filtering\n",
	    topology->level_nbitems[l], i);
  }

  /* Gather sublevels according to levels */
  for (l=0; l+1<topology->nb_levels; l++) {
    k = 0;
    for (i=0; i<topology->level_nbitems[l]; i++) {
      topo_cpuset_t level_set = topology->levels[l][i].cpuset;
      for (j=k; j<topology->level_nbitems[l+1]; j++) {
	topo_cpuset_t set = level_set;
	topo_cpuset_andset(&set, &topology->levels[l+1][j].cpuset);
	if (!topo_cpuset_iszero(&set)) {
	  /* Sublevel j is part of level i, put it at k.  */
	  struct topo_level level = topology->levels[l+1][j];
	  memmove(&topology->levels[l+1][k+1], &topology->levels[l+1][k], (j-k)*sizeof(*topology->levels[l+1]));
	  topology->levels[l+1][k++] = level;
	}
      }
    }
  }

  /* Now we can put numbers on levels. */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      {
	topology->levels[l][i].number = i;
	ltdebug("level %u,%u: cpuset %"TOPO_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));
      }

  /* And show debug again */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      ltdebug("level %u,%u: cpuset %"TOPO_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));

  /* Compute arity */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  topology->levels[l][i].arity=0;
	  for (j=0; j<topology->level_nbitems[l+1]; j++)
	    if (topo_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
	      topology->levels[l][i].arity++;
	  ltdebug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" arity %u\n",
		  l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	}
    }


  for (i=0; i<topology->level_nbitems[topology->nb_levels-1]; i++)
    ltdebug("level %u,%u: cpuset %"TOPO_PRIxCPUSET" leaf\n",
	    topology->nb_levels-1, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[topology->nb_levels-1][i].cpuset));
  ltdebug("arity done.\n");

  /* and finally connect levels */
  topo_connect(topology);
  ltdebug("connecting done.\n");

  /* intialize all depth to unknown */
  for (l=0; l < TOPO_LEVEL_MAX; l++)
    topology->type_depth[l] = -1;
  /* walk the existing levels to set their depth */
  for (l=0; l<topology->nb_levels; l++)
    topology->type_depth[topology->levels[l][0].type] = l;
  /* setup the depth of all still unknown levels (the one that got merged or never created */
  int type, prevdepth = -1;
  for (type = 0; type < TOPO_LEVEL_MAX; type++)
    {
      if (topology->type_depth[type] == -1)
	topology->type_depth[type] = prevdepth;
      else
	prevdepth = topology->type_depth[type];
    }

  /* set level depth */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      topology->levels[l][i].level = l;

  /* Empty some NUMA node memory if disabled by the administrator */
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE)) {
    l = topology->type_depth[TOPO_LEVEL_NODE];
    for (i=0; i<topology->level_nbitems[l]; i++) {
      /* remove memory if disabled */
      if (topology->levels[l][i].admin_disabled) {
	topology->levels[l][i].memory_kB[TOPO_LEVEL_MEMORY_NODE] = 0;
	topology->levels[l][i].huge_page_free = 0;
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
  topology->fsys_root_fd = -1;
  topology->use_synthetic = 0;
  topology->huge_page_size_kB = 0;
  topology->dmi_board_vendor = NULL;
  topology->dmi_board_name = NULL;
  for(i=0; i< TOPO_LEVEL_MAX; i++)
    topology->ignored_types[i] = TOPO_IGNORE_LEVEL_NEVER;
  topology->level_nbitems[0] = 1;
  topology->levels[0] = malloc (2*sizeof (struct topo_level));
  lt_setup_machine_level (&topology->levels[0]);
  topology->is_fake = 0;

  *topologyp = topology;
  return 0;
}

int
topo_topology_set_fsys_root(struct topo_topology *topology, const char *fsys_root_path)
{
#ifdef LINUX_SYS
  if (lt_set_fsys_root(topology, fsys_root_path))
    return -1;
#endif /* LINUX_SYS */

  return 0;
}

int
topo_topology_set_synthetic(struct topo_topology *topology, const char *description)
{
  return topo_synthetic_parse_description(topology, description);
}

int
topo_topology_set_flags (struct topo_topology *topology, unsigned long flags)
{
  topology->flags = flags;
  return 0;
}

int
topo_topology_ignore_type(struct topo_topology *topology, enum topo_level_type_e type)
{
  if (type >= TOPO_LEVEL_MAX)
    return -1;

  if (type == TOPO_LEVEL_MACHINE)
    /* we don't want 2 heads */
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_LEVEL_ALWAYS;
  return 0;
}

int
topo_topology_ignore_type_keep_structure(struct topo_topology *topology, topo_level_type_t type)
{
  if (type >= TOPO_LEVEL_MAX)
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_LEVEL_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_ignore_all_keep_structure(struct topo_topology *topology)
{
  unsigned type = type;
  for(type=0; type<TOPO_LEVEL_MAX; type++)
    topology->ignored_types[type] = TOPO_IGNORE_LEVEL_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_load (struct topo_topology *topology)
{
#ifdef HAVE_OPENAT
  if (topology->fsys_root_fd < 0)
    if (topo_topology_set_fsys_root(topology, "/") < 0)
      return -1;
#endif

  if (topology->use_synthetic)
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
  /* Last level is not freed because we need it in various places */
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  free(topology->levels[l][i].children);
	  topology->levels[l][i].children = NULL;
	}
      if (l)
	{
	  free(topology->levels[l]);
	  topology->levels[l] = NULL;
	}
    }

  if (topology->fsys_root_fd >= 0)
    close(topology->fsys_root_fd);
  free(topology->dmi_board_vendor);
  free(topology->dmi_board_name);
}
