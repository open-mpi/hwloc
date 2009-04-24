#include <config.h>
#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

static int
lt_parse_sysfs_unsigned(const char *mappath, unsigned *value, int fsys_root_fd)
{
  char string[11];
  FILE * fd;

  fd = lt_fopen(mappath, "r", fsys_root_fd);
  if (!fd)
    return -1;

  fgets(string, 11, fd);
  *value = strtoul(string, NULL, 10);

  fclose(fd);

  return 0;
}


/* kernel cpumaps are composed of an array of 32bits cpumasks */
#define KERNEL_CPU_MASK_BITS 32
#define KERNEL_CPU_MAP_LEN (KERNEL_CPU_MASK_BITS/4+2)
#define MAX_KERNEL_CPU_MASK ((LIBTOPO_NBMAXCPUS+KERNEL_CPU_MASK_BITS-1)/KERNEL_CPU_MASK_BITS)

static int
lt_parse_cpumap(const char *mappath, lt_cpuset_t *set, int fsys_root_fd)
{
  char string[KERNEL_CPU_MAP_LEN]; /* enough for a shared map mask (32bits hexa) */
  unsigned long maps[MAX_KERNEL_CPU_MASK];
  int nr_maps = 0;
  FILE * fd;
  int i;

  /* reset to zero first */
  lt_cpuset_zero(set);

  fd = lt_fopen(mappath, "r", fsys_root_fd);
  if (!fd)
    return -1;

  /* parse the whole mask */
  while (fgets(string, KERNEL_CPU_MAP_LEN, fd) && *string != '\0') /* read one kernel cpu mask and the ending comma */
    {
      unsigned long map;
      assert(!(nr_maps == MAX_KERNEL_CPU_MASK)); /* too many cpumasks in this cpumap */

      map = strtoul(string, NULL, 16);
      if (!map && !nr_maps)
	/* ignore the first map if it's empty */
	continue;

      memmove(&maps[1], &maps[0], nr_maps*sizeof(*maps));
      maps[0] = map;
      nr_maps++;
    }

  /* check that the map can be stored in our cpuset */
  assert(!(nr_maps*KERNEL_CPU_MASK_BITS > LIBTOPO_NBMAXCPUS));

  /* convert into a set */
  for(i=0; i<nr_maps*KERNEL_CPU_MASK_BITS; i++)
    if (maps[i/KERNEL_CPU_MASK_BITS] & 1<<(i%KERNEL_CPU_MASK_BITS))
      lt_cpuset_set(set, i);

  fclose(fd);

  return 0;
}

static void
lt_process_shared_cpu_map(const char *mappath, const char * mapname, unsigned long val,
			  int procid_max, unsigned *ids, unsigned long *vals,
			  unsigned *nr_ids, unsigned givenid, lt_cpuset_t *offline_cpus_set,
			  int fsys_root_fd)
{
  lt_cpuset_t set;
  int k;

  lt_parse_cpumap(mappath, &set, fsys_root_fd);
  lt_cpuset_clearset(&set, offline_cpus_set);
  for(k=0; k<procid_max; k++)
    {
      if (lt_cpuset_isset(&set, k))
	{
	  /* we found a cpu in the map */
	  unsigned newid;

	  if (ids[k] != -1)
	    /* already got this map, stop using it */
	    break;

	  /* allocate a new id, either by incrementing the global counter, or by using the given id */
	  newid = nr_ids ? (*nr_ids)++ : givenid;

	  /* this cpu didn't have any such id yet, set this id for all cpus in the map */
	  for(; k<procid_max; k++)
	    {
	      if (lt_cpuset_isset(&set, k))
		{
		  ltdebug("--- proc %d has %s number %d\n", k, mapname, newid);
		  ids[k] = newid;
		  vals[newid] = val;
		}
	    }

	  break;
	}
    }
}

static void
lt_parse_cache_shared_cpu_maps(int proc_index, int procid_max, lt_cpuset_t *offline_cpus_set,
			       unsigned *cacheids, unsigned long *cachesizes, unsigned *nr_caches,
			       int fsys_root_fd)
{
  int i;

  for(i=0; i<10; i++) {
#define SHARED_CPU_MAP_STRLEN (27+9+12+1+15+1)
    char mappath[SHARED_CPU_MAP_STRLEN];
    char string[20]; /* enough for a level number (one digit) or a type (Data/Instruction/Unified) */
    char cachename[8+1];
    unsigned long kB = 0;
    int level; /* 0 for L1, .... */
    FILE * fd;

    sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/level", proc_index, i);
    fd = lt_fopen(mappath, "r", fsys_root_fd);
    if (fd)
      {
	if (fgets(string,sizeof(string), fd))
	  level = strtoul(string, NULL, 10)-1;
	else
	  continue;
	fclose(fd);
      }
    else
      continue;

    sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/type", proc_index, i);
    fd = lt_fopen(mappath, "r", fsys_root_fd);
    if (fd)
      {
	if (fgets(string,sizeof(string), fd))
	  {
	    fclose(fd);
	    if (!strncmp(string,"Instruction", 11))
	      continue;
	  }
	else
	  {
	    fclose(fd);
	    continue;
	  }
      }
    else
      continue;

    sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/size", proc_index, i);
    fd = lt_fopen(mappath, "r", fsys_root_fd);
    if (fd)
      {
	if (fgets(string,sizeof(string), fd))
	  kB = atol(string); /* in kB */
	fclose(fd);
      }

    sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", proc_index, i);
    sprintf(cachename, "L%d cache", level+1);
    lt_process_shared_cpu_map(mappath, cachename, kB,
			      procid_max, cacheids+level*LIBTOPO_NBMAXCPUS,
			      cachesizes+level*LIBTOPO_NBMAXCPUS,
			      &nr_caches[level], -1, offline_cpus_set,
			      fsys_root_fd);
  }
}

static unsigned long
lt_procfs_meminfo_to_memsize(const char *path, lt_topo_t *topology)
{
  char string[64];
  FILE *fd;

  fd = lt_fopen(path, "r", topology->fsys_root_fd);
  if (!fd)
    return 0;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      unsigned long size;
      if (sscanf(string, "MemTotal: %ld kB", &size) == 1)
	{
	  fclose(fd);
	  return size;
	}
    }

  fclose(fd);
  return 0;
}

static unsigned long
lt_procfs_meminfo_to_hugepagefree(const char *path, lt_topo_t *topology)
{
  char string[64];
  FILE *fd;

  fd = lt_fopen(path, "r", topology->fsys_root_fd);
  if (!fd)
    return 0;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      unsigned long number;
      if (sscanf(string, "HugePages_Free: %ld", &number) == 1)
	{
	  fclose(fd);
	  return number;
	}
    }

  fclose(fd);
  return 0;
}

static unsigned long
lt_sysfs_node_meminfo_to_memsize(const char * path, lt_topo_t *topology)
{
  char string[64];
  FILE *fd;

  fd = lt_fopen(path, "r", topology->fsys_root_fd);
  if (!fd)
    return 0;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      int node;
      unsigned long size;
      if (sscanf(string, "Node %d MemTotal: %ld kB", &node, &size) == 2)
	{
	  fclose(fd);
	  return size;
	}
    }

  fclose(fd);
  return 0;
}

static unsigned long
lt_sysfs_node_meminfo_to_hugepagefree(const char * path, lt_topo_t *topology)
{
  char string[64];
  FILE *fd;

  fd = lt_fopen(path, "r", topology->fsys_root_fd);
  if (!fd)
    return 0;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      int node;
      unsigned long hugepagefree;
      if (sscanf(string, "Node %d HugePages_Free: %ld kB", &node, &hugepagefree) == 2)
	{
	  fclose(fd);
	  return hugepagefree;
	}
    }

  fclose(fd);
  return 0;
}

static void
look_sysfsnode(lt_topo_t *topology)
{
  unsigned i, osnode;
  unsigned nbnodes = 1;
  struct lt_level *node_level;
  DIR *dir;
  struct dirent *dirent;

  dir = lt_opendir("/sys/devices/system/node", topology->fsys_root_fd);
  if (dir)
    {
      while ((dirent = readdir(dir)) != NULL)
	{
	  unsigned long node;
	  if (strncmp(dirent->d_name, "node", 4))
	    continue;
	  node = strtoul(dirent->d_name+4, NULL, 0);
	  if (nbnodes < node+1)
	    nbnodes = node+1;
	}
      closedir(dir);
    }

  if (nbnodes <= 1)
    {
      topology->nb_nodes = nbnodes;
      return;
    }

  node_level=malloc((nbnodes+1)*sizeof(*node_level));
  assert(node_level);

  for (i=0, osnode=0;; osnode++)
    {
#define NUMA_NODE_STRLEN (29+9+8+1)
      char nodepath[NUMA_NODE_STRLEN];
      lt_cpuset_t cpuset;
      unsigned long size;
      unsigned long hpfree;

      sprintf(nodepath, "/sys/devices/system/node/node%d/cpumap", osnode);
      if (lt_parse_cpumap(nodepath, &cpuset, topology->fsys_root_fd) < 0)
	break;

      /* FIXME: we should clearset offline_cpus_set in case the node cpumap
       * is not uptodate wrt to offline cpus. but offline_cpus_set isn't
       * ready yet.
       */

      sprintf(nodepath, "/sys/devices/system/node/node%d/meminfo", osnode);
      size = lt_sysfs_node_meminfo_to_memsize(nodepath, topology);
      hpfree = lt_sysfs_node_meminfo_to_hugepagefree(nodepath, topology);

      lt_setup_level(&node_level[i], LT_LEVEL_NODE);
      lt_set_os_numbers(&node_level[i], LT_LEVEL_NODE, osnode);
      node_level[i].memory_kB[LT_LEVEL_MEMORY_NODE] = size;
      node_level[i].huge_page_free = hpfree;
      node_level[i].cpuset = cpuset;

      ltdebug("node %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
	      i, osnode, LT_CPUSET_PRINTF_VALUE(node_level[i].cpuset));
      i++;
    }
  nbnodes = i;

  lt_cpuset_zero(&node_level[nbnodes].cpuset);

  topology->level_nbitems[topology->discovering_level] = topology->nb_nodes = nbnodes;
  topology->levels[topology->discovering_level++] = node_level;
}

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void
look__sysfscpu(unsigned *procid_max,
	       lt_cpuset_t *offline_cpus_set,
	       unsigned *nr_procs,
	       unsigned *nr_cores,
	       unsigned *nr_dies,
	       unsigned *proc_physids,
	       unsigned *osphysids,
	       unsigned *proc_coreids,
	       unsigned *oscoreids,
	       lt_topo_t *topology)
{
#define CPU_TOPOLOGY_STR_LEN (27+9+29+1)
  char string[CPU_TOPOLOGY_STR_LEN];
  unsigned cpu_max = 1;
  unsigned nr_offline_cpus = 0;
  struct dirent *dirent;
  DIR *dir;
  int i,j,k;

  dir = lt_opendir("/sys/devices/system/cpu", topology->fsys_root_fd);
  if (dir)
    {
      while ((dirent = readdir(dir)) != NULL)
	{
	  unsigned long cpu;
	  if (strncmp(dirent->d_name, "cpu", 3))
	    continue;
	  cpu = strtoul(dirent->d_name+3, NULL, 0);
	  if (cpu_max < cpu+1)
	    cpu_max = cpu+1;
	}
      closedir(dir);
    }
  ltdebug("found os proc id max %d\n", cpu_max);

  assert(!(cpu_max > LIBTOPO_NBMAXCPUS));

  for(i=0; i<cpu_max; i++)
    {
      lt_cpuset_t dieset, coreset;
      unsigned mydieid, mycoreid;
      FILE *fd;
      char online[2];

      /* check whether the kernel knows another cpu */
      sprintf(string, "/sys/devices/system/cpu/cpu%d", i);
      if (lt_access(string, X_OK, topology->fsys_root_fd) < 0 && errno == ENOENT)
	{
	/* this CPU does not exist */
	  ltdebug("os proc %d has no accessible /sys/devices/system/cpu/cpu%d/\n", i, i);
	  lt_cpuset_set(offline_cpus_set, i);
	  nr_offline_cpus++;
	  continue;
	}

      /* check whether this processor is offline */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/online", i);
      fd = lt_fopen(string, "r", topology->fsys_root_fd);
      if (fd)
	{
	  if (fgets(online, sizeof(online), fd))
	    {
	      fclose(fd);
	      if (atoi(online))
		{
		  ltdebug("os proc %d is online\n", i);
		}
	      else
		{
		  ltdebug("os proc %d is offline\n", i);
		  lt_cpuset_set(offline_cpus_set, i);
		  nr_offline_cpus++;
		  continue;
		}
	    }
	  else
	    {
	      fclose(fd);
	    }
	}

      /* check whether the kernel exports topology information for this cpu */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology", i);
      if (lt_access(string, X_OK, topology->fsys_root_fd) < 0 && errno == ENOENT)
	{
	  ltdebug("os proc %d has no accessible /sys/devices/system/cpu/cpu%d/topology\n", i, i);
	  lt_cpuset_set(offline_cpus_set, i);
	  nr_offline_cpus++;
	  continue;
	}

      mydieid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
      lt_parse_sysfs_unsigned(string, &mydieid, topology->fsys_root_fd);

      mycoreid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_id", i);
      lt_parse_sysfs_unsigned(string, &mycoreid, topology->fsys_root_fd);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_siblings", i);
      lt_parse_cpumap(string, &dieset, topology->fsys_root_fd);
      /* make sure we are in the set, in case the cpumap was crap */
      lt_cpuset_set(&dieset, i);
      lt_cpuset_clearset(&dieset, offline_cpus_set);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", i);
      lt_parse_cpumap(string, &coreset, topology->fsys_root_fd);
      /* make sure we are in the set, in case the cpumap was crap */
      lt_cpuset_set(&coreset, i);
      lt_cpuset_clearset(&coreset, offline_cpus_set);

      for(j=0; j<i; j++)
	if (lt_cpuset_isset(&dieset, j))
	  break;
      if (j==i)
	{
	  /* new die cpumap fill it */
	  for(k=i; k<LIBTOPO_NBMAXCPUS; k++)
	    if (lt_cpuset_isset(&dieset, k))
	      proc_physids[k] = (*nr_dies);
	  ltdebug("die %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
		  (*nr_dies), mydieid, LT_CPUSET_PRINTF_VALUE(dieset));
	  osphysids[(*nr_dies)++] = mydieid;
	}

      for(j=0; j<i; j++)
	if (lt_cpuset_isset(&coreset, j))
	  break;
      if (j==i)
	{
	  /* new core cpumap fill it */
	  for(k=i; k<LIBTOPO_NBMAXCPUS; k++)
	    if (lt_cpuset_isset(&coreset, k))
	      proc_coreids[k] = (*nr_cores);
	  ltdebug("core %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
		  (*nr_cores), mycoreid, LT_CPUSET_PRINTF_VALUE(coreset));
	  oscoreids[(*nr_cores)++] = mycoreid;
	}
    }

  *nr_procs = cpu_max - nr_offline_cpus;
  *procid_max = cpu_max;
  ltdebug("%s: found %u procs\n", __func__, *nr_procs);
}


/* Look at Linux' /proc/cpuinfo */
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"
static void
look_cpuinfo(unsigned *procid_max,
	     unsigned *nr_procs,
	     unsigned *nr_cores,
	     unsigned *nr_dies,
	     unsigned *proc_physids,
	     unsigned *osphysids,
	     unsigned *proc_coreids,
	     unsigned *oscoreids,
	     lt_topo_t *topology)
{
  FILE *fd;
  char string[strlen(PHYSID)+1+9+1+1];
  char *endptr;
  unsigned proc_osphysids[LIBTOPO_NBMAXCPUS];
  unsigned proc_oscoreids[LIBTOPO_NBMAXCPUS];
  unsigned core_osphysids[LIBTOPO_NBMAXCPUS];
  long physid;
  long coreid;
  long processor = -1;
  int i;

  memset(proc_physids,0,sizeof(proc_physids));
  memset(proc_osphysids,0,sizeof(proc_osphysids));
  memset(proc_coreids,0,sizeof(proc_coreids));
  memset(proc_oscoreids,0,sizeof(proc_oscoreids));

  if (!(fd=lt_fopen("/proc/cpuinfo","r", topology->fsys_root_fd)))
    {
      fprintf(stderr,"could not open /proc/cpuinfo\n");
      return;
    }

  /* Just record information and count number of dies and cores */

  ltdebug("\n\n * Topology extraction from /proc/cpuinfo *\n\n");
  while (fgets(string,sizeof(string),fd)!=NULL)
    {
#      define getprocnb_begin(field, var)		     \
      if ( !strncmp(field,string,strlen(field)))	     \
	{						     \
	char *c = strchr(string, ':')+1;		     \
	var = strtoul(c,&endptr,0);			     \
	if (endptr==c)							\
	  {								\
	    fprintf(stderr,"no number in "field" field of /proc/cpuinfo\n"); \
	    break;							\
	  }								\
	else if (var==LONG_MIN)						\
	  {								\
	    fprintf(stderr,"too small "field" number in /proc/cpuinfo\n"); \
	    break;							\
	  }								\
	else if (var==LONG_MAX)						\
	  {								\
	    fprintf(stderr,"too big "field" number in /proc/cpuinfo\n"); \
	    break;							\
	  }								\
	ltdebug(field " %ld\n", var)
#      define getprocnb_end()			\
      }
      getprocnb_begin(PROCESSOR,processor);
      getprocnb_end() else
      getprocnb_begin(PHYSID,physid);
      proc_osphysids[processor]=physid;
      for (i=0; i<*nr_dies; i++)
	if (physid == osphysids[i])
	  break;
      proc_physids[processor]=i;
      ltdebug("%ld on die %d (%lx)\n", processor, i, physid);
      if (i==*nr_dies)
	osphysids[(*nr_dies)++] = physid;
      getprocnb_end() else
      getprocnb_begin(COREID,coreid);
      proc_oscoreids[processor]=coreid;
      for (i=0; i<*nr_cores; i++)
	if (coreid == oscoreids[i] && proc_osphysids[processor] == core_osphysids[i])
	  break;
      proc_coreids[processor]=i;
      ltdebug("%ld on core %d (%lx:%x)\n", processor, i, coreid, proc_osphysids[processor]);
      if (i==*nr_cores)
	{
	  core_osphysids[*nr_cores] = proc_osphysids[processor];
	  oscoreids[*nr_cores] = coreid;
	  (*nr_cores)++;
	}
      getprocnb_end()
	if (string[strlen(string)-1]!='\n')
	  {
	    fscanf(fd,"%*[^\n]");
	    getc(fd);
	  }
    }
  fclose(fd);

  /* setup the final number of procs */
  *nr_procs = processor + 1;
  *procid_max = processor + 1;
  ltdebug("%s: found %u procs\n", __func__, *nr_procs);
}


static void
look_sysfscpu(lt_cpuset_t *offline_cpus_set, lt_topo_t *topology)
{
  unsigned proc_physids[] = { [0 ... LIBTOPO_NBMAXCPUS-1] = -1 };
  unsigned osphysids[] = { [0 ... LIBTOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_coreids[] = { [0 ... LIBTOPO_NBMAXCPUS-1] = -1 };
  unsigned oscoreids[] = { [0 ... LIBTOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_cacheids[] = { [0 ... LIBTOPO_CACHE_LEVEL_MAX*LIBTOPO_NBMAXCPUS-1] = -1 };
  unsigned long cache_sizes[] = { [0 ... LIBTOPO_CACHE_LEVEL_MAX*LIBTOPO_NBMAXCPUS-1] = 0 };
  int j;

  unsigned procid_max=0;
  unsigned numprocs=0;
  unsigned numdies=0;
  unsigned numcores=0;
  unsigned numcaches[] = { [0 ... LIBTOPO_CACHE_LEVEL_MAX-1] = 0 };

  if (lt_access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK, topology->fsys_root_fd) < 0
      || lt_access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK, topology->fsys_root_fd) < 0
      || lt_access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK, topology->fsys_root_fd) < 0
      || lt_access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK, topology->fsys_root_fd) < 0)
    {
      /* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
      look_cpuinfo(&procid_max, &numprocs, &numcores, &numdies,
		   proc_physids, osphysids,
		   proc_coreids, oscoreids,
		   topology);
    }
  else
    {
      look__sysfscpu(&procid_max, offline_cpus_set, &numprocs, &numcores, &numdies,
		     proc_physids, osphysids,
		     proc_coreids, oscoreids,
		     topology);
    }

  ltdebug("\n * Topology summary *\n");
  ltdebug("%d processors (%d max id)\n", numprocs, procid_max);

  if (numdies>1)
    ltdebug("%d dies\n", numdies);
  if (numcores>1)
    ltdebug("%d cores\n", numcores);

  if (numdies>1)
    lt_setup_die_level(procid_max, numdies, osphysids, proc_physids, topology);

  for(j=0; j<procid_max; j++)
    {
      lt_parse_cache_shared_cpu_maps(j, procid_max, offline_cpus_set, proc_cacheids, cache_sizes, numcaches, topology->fsys_root_fd);
    }

  if (numcaches[2] > 0)
    {
      /* setup L3 caches */
      lt_setup_cache_level(2, LT_LEVEL_L3, procid_max, numcaches, proc_cacheids, cache_sizes, topology);
    }

  if (numcaches[1] > 0)
    {
      /* setup L2 caches */
      lt_setup_cache_level(1, LT_LEVEL_L2, procid_max, numcaches, proc_cacheids, cache_sizes, topology);
    }

  if (numcores>1)
    lt_setup_core_level(procid_max, numcores, oscoreids, proc_coreids, topology);

  if (numcaches[0] > 0)
    {
      /* setup L1 caches */
      lt_setup_cache_level(0, LT_LEVEL_L1, procid_max, numcaches, proc_cacheids, cache_sizes, topology);
    }

  /* Override the default returned by `ma_fallback_nbprocessors ()'.  */
  topology->nb_processors = numprocs;
  ltdebug("%s: found %u procs\n", __func__, topology->nb_processors);
}

void
look_linux(lt_topo_t *topology, lt_cpuset_t *offline_cpus_set)
{
  look_sysfsnode(topology);
  look_sysfscpu(offline_cpus_set, topology);

  /* Compute the whole machine memory and huge page */
  /* FIXME: Only when no numa node available ? */
  topology->levels[0][0].memory_kB[LT_LEVEL_MEMORY_MACHINE]=
    lt_procfs_meminfo_to_memsize("/proc/meminfo", topology);
  topology->levels[0][0].huge_page_free =
    lt_procfs_meminfo_to_hugepagefree("/proc/meminfo", topology);
}
