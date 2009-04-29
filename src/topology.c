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
      ltdebug("die %d has cpuset %"LT_PRIxCPUSET"\n",
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
      ltdebug("core %d has cpuset %"LT_PRIxCPUSET"\n",
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

      ltdebug("L%d cache %d has cpuset %"LT_PRIxCPUSET"\n",
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
look_cpu(topo_cpuset_t *offline_cpus_set, struct topo_topology *topology)
{
  struct topo_level *cpu_level;
  unsigned oscpu,cpu;

  cpu_level=malloc((topology->nb_processors+1)*sizeof(*cpu_level));
  assert(cpu_level);

  ltdebug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; cpu++,oscpu++)
    {
      while (topo_cpuset_isset(offline_cpus_set, oscpu))
	oscpu++;
      lt_setup_level(&cpu_level[cpu], TOPO_LEVEL_PROC);
      lt_set_os_numbers(&cpu_level[cpu], TOPO_LEVEL_PROC, oscpu);

      topo_cpuset_cpu(&cpu_level[cpu].cpuset, oscpu);

      ltdebug("cpu %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
	      cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(cpu_level[cpu].cpuset));
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
	      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
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

static int
compar(const void *_l1, const void *_l2)
{
  const struct topo_level *l1 = _l1, *l2 = _l2;
  /* empty cpuset are always considered higher, so they are stored at the end, that's ok */
  return topo_cpuset_first(&l1->cpuset) - topo_cpuset_first(&l2->cpuset);
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
  /*	unsigned nbsublevels; */
  /*	unsigned sublevelarity; */
  topo_cpuset_t offline_cpus_set;

  topo_cpuset_zero(&offline_cpus_set);

#    ifdef LINUX_SYS
  look_linux(topology, &offline_cpus_set);
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

#    ifdef  WINDOWS_SYS
  look_windows(topology);
#    endif /* WINDOWS_SYS */

  look_cpu(&offline_cpus_set, topology);

  ltdebug("\n\n--> discovered %d levels\n\n", topology->nb_levels);

  assert(topology->nb_processors);

  /* Compute the machine cpuset */
  topo_cpuset_zero(&topology->levels[0][0].cpuset);
  for (i=0; i<topology->level_nbitems[1]; i++)
    topo_cpuset_orset(&topology->levels[0][0].cpuset, &topology->levels[1][i].cpuset);

  /* sort levels according to cpu sets */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      /* first sort sublevels according to cpu sets */
      qsort(&topology->levels[l+1][0], topology->level_nbitems[l+1], sizeof(*topology->levels[l+1]), compar);
      k = 0;
      /* then gather sublevels according to levels */
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  topo_cpuset_t level_set = topology->levels[l][i].cpuset;
	  for (j=k; j<topology->level_nbitems[l+1]; j++)
	    {
	      topo_cpuset_t set = level_set;
	      topo_cpuset_andset(&set, &topology->levels[l+1][j].cpuset);
	      if (!topo_cpuset_iszero(&set))
		{
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
	ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));
      }

  /* And show debug again */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));

  /* Compute arity */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  topology->levels[l][i].arity=0;
	  for (j=0; j<topology->level_nbitems[l+1]; j++)
	    if (topo_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
	      topology->levels[l][i].arity++;
	  ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
		  l, i, TOPO_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	}
    }


  for (i=0; i<topology->level_nbitems[topology->nb_levels-1]; i++)
    ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" leaf\n",
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

  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      topology->levels[l][i].level = l;
}

int
topo_topology_init (struct topo_topology **topologyp)
{
  struct topo_topology *topology;
  topology = topo_default_allocator.allocate (sizeof (struct topo_topology));
  if(!topology)
    return -1;

  lt_setup_topo(topology);

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
topo_topology_set_flags (topo_topology_t topology, unsigned long flags)
{
  topology->flags = flags;
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
