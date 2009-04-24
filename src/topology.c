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
#include <libtopology/helper.h>
#include <libtopology/debug.h>

static void
lt_print_level_description(struct lt_level *l, FILE *output, int verbose_mode)
{
  unsigned long type = l->type;
  const char * separator = " + ";
  const char * current_separator = ""; /* not prefix for the first one */

#define lt_print_level_description_level(_l) \
    {									\
      if (type == _l) \
	{	      \
	  fprintf(output, "%s%s", current_separator, lt_level_string(_l)); \
	  current_separator = separator;				\
	}								\
    }

  lt_print_level_description_level(LT_LEVEL_MACHINE)
  lt_print_level_description_level(LT_LEVEL_FAKE)
  lt_print_level_description_level(LT_LEVEL_NODE)
  lt_print_level_description_level(LT_LEVEL_DIE)
  lt_print_level_description_level(LT_LEVEL_L3)
  lt_print_level_description_level(LT_LEVEL_L2)
  lt_print_level_description_level(LT_LEVEL_CORE)
  lt_print_level_description_level(LT_LEVEL_L1)
  lt_print_level_description_level(LT_LEVEL_PROC)
}

#define lt_memory_size_printf_value(_size) \
  (_size)%1024 ? (_size) : (_size)%(1024*1024) ? (_size)>>10 : (_size)>>20
#define lt_memory_size_printf_unit(_size) \
  (_size)%1024 ? "kB" : (_size)%(1024*1024) ? "MB" : "GB"

void
lt_print_level(lt_topo_t *topology, struct lt_level *l, FILE *output, int verbose_mode, const char *separator,
	       const char *indexprefix, const char* labelseparator, const char* levelterm)
{
  lt_print_level_description(l, output, verbose_mode);
  fprintf(output, "%s", labelseparator);
  if (l->type == LT_LEVEL_MACHINE) fprintf(output, "%sMachine(%ld%s)", separator,
				lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_MACHINE]),
				lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_MACHINE]));
  if (l->os_node != -1) fprintf(output, "%sNode%s%u(%ld%s)", separator, indexprefix, l->os_node,
				lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_NODE]),
				lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_NODE]));
  if (l->os_die != -1)  fprintf(output, "%sDie%s%u", separator, indexprefix, l->os_die);
  if (l->os_l3 != -1)   fprintf(output, "%sL3%s%u(%ld%s)", separator, indexprefix, l->os_l3,
				lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_L3]),
				lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_L3]));
  if (l->os_l2 != -1)   fprintf(output, "%sL2%s%u(%ld%s)", separator, indexprefix, l->os_l2,
				lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_L2]),
				lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_L2]));
  if (l->os_core != -1) fprintf(output, "%sCore%s%u", separator, indexprefix, l->os_core);
  if (l->os_l1 != -1)   fprintf(output, "%sL1%s%u(%ld%s)", separator, indexprefix, l->os_l1,
				lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_L1]),
				lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_L1]));
  if (l->os_cpu != -1)  fprintf(output, "%sCPU%s%u", separator, indexprefix, l->os_cpu);
  if (l->level == topology->nb_levels-1) {
    fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  fprintf(output,"%s\n", levelterm);
}


#if (__GLIBC__ > 2) || (__GLIBC_MINOR__ >= 4)
# define HAVE_OPENAT
#endif

#ifdef HAVE_OPENAT

int
lt_set_fsys_root(const char *path, lt_topo_t *topology)
{
  int root;

  root = open(path, O_RDONLY | O_DIRECTORY);
  if (root < 0)
    return -1;

  topology->fsys_root_fd = root;
  return 0;
}

static __attribute__ ((__unused__)) FILE *
lt_fopenat(const char *path, const char *mode, int fsys_root_fd)
{
  int fd;
  const char *relative_path;

  assert(!(fsys_root_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  fd = openat (fsys_root_fd, relative_path, O_RDONLY);
  if (fd == -1)
    return NULL;

  return fdopen(fd, mode);
}

static __attribute__ ((__unused__)) int
lt_accessat(const char *path, int mode, int fsys_root_fd)
{
  const char *relative_path;

  assert(!(fsys_root_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  return faccessat(fsys_root_fd, relative_path, O_RDONLY, 0);
}

static __attribute__ ((__unused__)) DIR*
lt_opendirat(const char *path, int fsys_root_fd)
{
  int dir_fd;
  const char *relative_path;

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  dir_fd = openat(fsys_root_fd, relative_path, O_RDONLY | O_DIRECTORY);
  if (dir_fd < 0)
    return NULL;

  return fdopendir(dir_fd);
}

/* Use our own filesystem functions.  */
#define lt_fopen(p, m, d)   lt_fopenat(p, m, d)
#define lt_access(p, m, d)  lt_accessat(p, m, d)
#define lt_opendir(p, d)    lt_opendirat(p, d)

#else /* !HAVE_OPENAT */

int
lt_set_fsys_root(const char *path, int fsys_root_fd)
{
  ltdebug(stderr, "`lt_set_fsys_root ()' not implemented\n");
  errno = ENOSYS;
  return -1;
}

#define lt_fopen(p, m, d)   fopen(p, m)
#define lt_access(p, m, d)  access(p, m)
#define lt_opendir(p, d)    opendir(p)

#endif /* !HAVE_OPENAT */



/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `lt_set_fsys_root ()' to
   have the desired effect.  */
static unsigned
lt_fallback_nbprocessors(void) {
#if defined(_SC_NPROCESSORS_ONLN)
  return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
  return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
  return sysconf(_SC_NPROC_CONF);
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


const char *
lt_level_string(enum lt_level_e l)
{
  switch (l)
    {
    case LT_LEVEL_MACHINE: return "Machine";
    case LT_LEVEL_FAKE: return "Fake";
    case LT_LEVEL_NODE: return "NUMANode";
    case LT_LEVEL_DIE: return "Die";
    case LT_LEVEL_L3: return "L3Cache";
    case LT_LEVEL_L2: return "L2Cache";
    case LT_LEVEL_CORE: return "Core";
    case LT_LEVEL_L1: return "L1Cache";
    case LT_LEVEL_PROC: return "SMTproc";
    default: return "Unknown";
    }
}


#    if defined(LINUX_SYS) || defined(SOLARIS_SYS)
static void
lt_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, lt_topo_t *topology)
{
  struct lt_level *die_level;
  int j;

  ltdebug("%d dies\n", numdies);
  die_level=malloc((numdies+1)*sizeof(*die_level));
  assert(die_level);

  for (j = 0; j < numdies; j++)
    {
      lt_setup_level(&die_level[j], LT_LEVEL_DIE);
      lt_set_os_numbers(&die_level[j], die, osphysids[j]);
      lt_level_cpuset_from_array(&die_level[j], j, proc_physids, procid_max);
      ltdebug("die %d has cpuset %"LT_PRIxCPUSET"\n",
	      j, LT_CPUSET_PRINTF_VALUE(die_level[j].cpuset));
    }
  ltdebug("\n");

  lt_cpuset_zero(&die_level[j].cpuset);

  topology->level_nbitems[topology->discovering_level]=numdies;
  ltdebug("--- die level has number %d\n", topology->discovering_level);
  topology->levels[topology->discovering_level++]=die_level;
  ltdebug("\n");
}

static void
lt_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, lt_topo_t *topology)
{
  struct lt_level *core_level;
  int j;

  ltdebug("%d cores\n", numcores);
  core_level=malloc((numcores+1)*sizeof(*core_level));
  assert(core_level);

  for (j = 0; j < numcores; j++)
    {
      lt_setup_level(&core_level[j], LT_LEVEL_CORE);
      lt_set_os_numbers(&core_level[j], core, oscoreids[j]);
      lt_level_cpuset_from_array(&core_level[j], j, proc_coreids, procid_max);
      ltdebug("core %d has cpuset %"LT_PRIxCPUSET"\n",
	      j, LT_CPUSET_PRINTF_VALUE(core_level[j].cpuset));
    }

  ltdebug("\n");

  lt_cpuset_zero(&core_level[j].cpuset);

  topology->level_nbitems[topology->discovering_level]=numcores;
  ltdebug("--- core level has number %d\n", topology->discovering_level);
  topology->levels[topology->discovering_level++]=core_level;
  ltdebug("\n");
}
#endif /* LINUX_SYS || SOLARIS_SYS */

#ifdef LINUX_SYS
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"

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
      lt_set_os_numbers(&node_level[i], node, osnode);
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

static void
lt_setup_cache_level(int cachelevel, enum lt_level_e topotype, int procid_max,
		     unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes,
		     lt_topo_t *topology)
{
  struct lt_level *level;
  int j;

  ltdebug("%d L%d caches\n", numcaches[cachelevel], cachelevel+1);
  level=malloc((numcaches[cachelevel]+1)*sizeof(*level));
  assert(level);

  for (j = 0; j < numcaches[cachelevel]; j++)
    {
      lt_setup_level(&level[j], topotype);

      switch (cachelevel)
	{
	case 2: lt_set_os_numbers(&level[j], l3, j); break;
	case 1: lt_set_os_numbers(&level[j], l2, j); break;
	case 0: lt_set_os_numbers(&level[j], l1, j); break;
	default: assert(!1);
	}

      level[j].memory_kB[LT_LEVEL_MEMORY_L1+cachelevel] = cachesizes[cachelevel*LIBTOPO_NBMAXCPUS+j];

      lt_level_cpuset_from_array(&level[j], j, &cacheids[cachelevel*LIBTOPO_NBMAXCPUS], procid_max);

      ltdebug("L%d cache %d has cpuset %"LT_PRIxCPUSET"\n",
	      cachelevel+1, j, LT_CPUSET_PRINTF_VALUE(level[j].cpuset));
    }
  ltdebug("\n");
  lt_cpuset_zero(&level[j].cpuset);
  topology->level_nbitems[topology->discovering_level]=numcaches[cachelevel];
  ltdebug("--- shared L%d level has number %d\n", cachelevel+1, topology->discovering_level);
  topology->levels[topology->discovering_level++]=level;
  ltdebug("\n");
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

  printf("\n\n * Topology summary *\n\n");
  printf("%d processors (%d max id)\n", numprocs, procid_max);

  if (numdies>1)
    printf("%d dies\n", numdies);
  if (numcores>1)
    printf("%d cores\n\n", numcores);

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

#endif /* LINUX_SYS */


#    ifdef SOLARIS_SYS
#      include <sys/lgrp_user.h>
static void
show(lgrp_cookie_t cookie, lgrp_id_t lgrp)
{
  processorid_t cpuids[32];
  lgrp_id_t lgrps[32];
  int i, n;
  n = lgrp_cpus(cookie, lgrp, cpuids, 32, LGRP_CONTENT_ALL);
  for (i = 0; i < n ; i++)
    {
      ltdebug("%ld has %d\n", lgrp, cpuids[i]);
    }
  n = lgrp_children(cookie, lgrp, lgrps, 32);
  ltdebug("%ld %d children\n", lgrp, n);
  for (i = 0; i < n ; i++)
    {
      show(cookie, lgrps[i]);
    }
  ltdebug("%ld children done\n", lgrp);
}

static void
look_lgrp(void)
{
  lgrp_cookie_t cookie = lgrp_init(LGRP_VIEW_OS);
  lgrp_id_t root;
  if (cookie == LGRP_COOKIE_NONE)
    {
      ltdebug("lgrp_init failed: %s\n", strerror(errno));
      return;
    }
  root = lgrp_root(cookie);
  show(cookie, root);
  /* TODO */
  lgrp_fini(cookie);
}

#include <kstat.h>
static void
look_kstat(lt_topo_t *topology)
{
  kstat_ctl_t *kc = kstat_open();
  kstat_t *ksp;
  kstat_named_t *stat;
  unsigned proc_physids[LIBTOPO_NBMAXCPUS];
  unsigned proc_osphysids[LIBTOPO_NBMAXCPUS];
  unsigned osphysids[LIBTOPO_NBMAXCPUS];
  unsigned proc_coreids[LIBTOPO_NBMAXCPUS];
  unsigned proc_oscoreids[LIBTOPO_NBMAXCPUS];
  unsigned oscoreids[LIBTOPO_NBMAXCPUS];
  unsigned core_osphysids[LIBTOPO_NBMAXCPUS];
  unsigned physid, coreid;
  unsigned numprocs = 0;
  unsigned numdies = 0;
  unsigned numcores = 0;
  unsigned i;

  if (!kc)
    {
      ltdebug("kstat_open failed: %s\n", strerror(errno));
      return;
    }
  for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next)
    {
      if (strncmp("cpu_info", ksp->ks_module, 8))
	continue;

      if (ksp->ks_instance != numprocs)
	{
	  fprintf(stderr, "kstat instances not in CPU order: %d comes %d\n", ksp->ks_instance, numprocs);
	  goto out;
	}
      if (kstat_read(kc, ksp, NULL) == -1)
	{
	  fprintf(stderr, "kstat_read failed for CPU%d: %s\n", numprocs, strerror(errno));
	  goto out;
	}
      stat = (kstat_named_t *) kstat_data_lookup(ksp, "chip_id");
      if (!stat)
	{
	  if (numdies)
	    {
	      fprintf(stderr, "could not read die id for CPU%d: %s\n", numprocs, strerror(errno));
	      goto out;
	    }
	}
      else
	{
	  if (stat->data_type != KSTAT_DATA_INT32)
	    {
	      fprintf(stderr, "chip_id is not an INT32\n");
	      goto out;
	    }
	  proc_osphysids[numprocs] = physid = stat->value.i32;
	  for (i = 0; i < numdies; i++)
	    if (physid == osphysids[i])
	      break;
	  proc_physids[numprocs] = i;
	  ltdebug("%u on die %u (%u)\n", numprocs, i, physid);
	  if (i == numdies)
	    osphysids[numdies++] = physid;
	}

      stat = (kstat_named_t *) kstat_data_lookup(ksp, "core_id");
      if (!stat)
	{
	  if (numcores)
	    {
	      fprintf(stderr, "could not read core id for CPU%d: %s\n", numprocs, strerror(errno));
	      goto out;
	    }
	}
      else
	{
	  if (stat->data_type != KSTAT_DATA_INT32)
	    {
	      fprintf(stderr, "core_id is not an INT32\n");
	      goto out;
	    }
	  proc_oscoreids[numprocs] = coreid = stat->value.i32;
	  for (i = 0; i < numcores; i++)
	    if (coreid == oscoreids[i] && proc_osphysids[numprocs] == core_osphysids[i])
	      break;
	  proc_coreids[numprocs] = i;
	  ltdebug("%u on core %u (%u)\n", numprocs, i, coreid);
	  if (i == numcores)
	    {
	      core_osphysids[numcores] = proc_osphysids[numprocs];
	      oscoreids[numcores++] = coreid;
	    }
	}

      numprocs++;
    }

  if (numdies > 1)
    lt_setup_die_level(numprocs, numdies, osphysids, proc_physids, topology);

  if (numcores > 1)
    lt_setup_core_level(numprocs, numcores, oscoreids, proc_coreids, topology);
 out:
  kstat_close(kc);
}
#    endif /* SOLARIS_SYS */


/* Use the value stored in topology->nb_processors.  */
static void
look_cpu(lt_cpuset_t *offline_cpus_set, lt_topo_t *topology)
{
  struct lt_level *cpu_level;
  unsigned oscpu,cpu;

  cpu_level=malloc((topology->nb_processors+1)*sizeof(*cpu_level));
  assert(cpu_level);

  ltdebug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; cpu++,oscpu++)
    {
      while (lt_cpuset_isset(offline_cpus_set, oscpu))
	oscpu++;
      lt_setup_level(&cpu_level[cpu], LT_LEVEL_PROC);
      lt_set_os_numbers(&cpu_level[cpu], cpu, oscpu);

      lt_cpuset_cpu(&cpu_level[cpu].cpuset, oscpu);

      ltdebug("cpu %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
	      cpu, oscpu, LT_CPUSET_PRINTF_VALUE(cpu_level[cpu].cpuset));
    }
  lt_cpuset_zero(&cpu_level[cpu].cpuset);

  topology->level_nbitems[topology->discovering_level]=topology->nb_processors;
  topology->levels[topology->discovering_level++]=cpu_level;
}


/* Connect levels */
static void
topo_connect(lt_topo_t *topology)
{
  int l, i, j, m;
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
	{
	  if (topology->levels[l][i].arity)
	    {
	      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
		      l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	      topology->levels[l][i].children=malloc(topology->levels[l][i].arity*sizeof(void *));
	      assert(topology->levels[l][i].children);

	      m=0;
	      for (j=0; !lt_cpuset_iszero(&topology->levels[l+1][j].cpuset); j++)
		if (lt_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
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
  const struct lt_level *l1 = _l1, *l2 = _l2;
  return lt_cpuset_first(&l1->cpuset) - lt_cpuset_first(&l2->cpuset);
}

/* Main discovery loop */
static void
topo_discover(lt_topo_t *topology)
{
  unsigned l,i=0,j;

  assert(topology!=NULL);

  /* Initialize the number of processor to some reasonable default, e.g.,
     obtained using sysconf(3).  */
  topology->nb_processors = lt_fallback_nbprocessors ();

#ifdef HAVE_OPENAT
  if (topology->fsys_root_fd < 0)
    {
      /* Get a file descriptor to the file system root.  */
      char *fsys_root_path = getenv("LT_FSYS_ROOT_PATH");
      if (!fsys_root_path)
	fsys_root_path= "/";	

      if (lt_set_fsys_root(fsys_root_path, topology))
	{
	  perror ("opening file system root");
	  assert(0);
	}
    }
#endif

  /* Raw detection, from coarser levels to finer levels */
  unsigned k;
  /*	unsigned nbsublevels; */
  /*	unsigned sublevelarity; */
  lt_cpuset_t offline_cpus_set;

  lt_cpuset_zero(&offline_cpus_set);

#    ifdef LINUX_SYS
  look_sysfsnode(topology);
  look_sysfscpu(&offline_cpus_set, topology);
#    endif
#    ifdef  AIX_SYS
  for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++)
    {
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  ltdebug("looking AIX node sdl %d\n",i);
	  look_rset(i, LT_LEVEL_NODE);
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  ltdebug("looking AIX L2 sdl %d\n",i);
	  look_rset(i, LT_LEVEL_L2);
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  ltdebug("looking AIX core sdl %d\n",i);
	  look_rset(i, LT_LEVEL_CORE);
	}
#      endif
      if (i == rs_getinfo(NULL, R_SMPSDL, 0))
	ltdebug("not looking AIX \"SMP\" sdl %d\n",i);
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	{
	  ltdebug("looking AIX max sdl %d\n",i);
	  look_rset(i, LT_LEVEL_PROC);
	}
    }
#    endif /* AIX_SYS */
#    ifdef SOLARIS_SYS
  look_lgrp();
  look_kstat();
#    endif /* SOLARIS_SYS */
  look_cpu(&offline_cpus_set, topology);

  topology->nb_levels=topology->discovering_level;
  ltdebug("\n\n--> discovered %d levels\n\n", topology->nb_levels);

  assert(topology->nb_processors);

  /* Compute the machine cpuset */
  lt_cpuset_zero(&topology->levels[0][0].cpuset);
  for (i=0; i<topology->level_nbitems[1]; i++)
    lt_cpuset_orset(&topology->levels[0][0].cpuset, &topology->levels[1][i].cpuset);

  /* Compute the whole machine memory and huge page */
  /* FIXME: Only when no numa node available ? */
  topology->levels[0][0].memory_kB[LT_LEVEL_MEMORY_MACHINE]=
    lt_procfs_meminfo_to_memsize("/proc/meminfo", topology);
  topology->levels[0][0].huge_page_free =
    lt_procfs_meminfo_to_hugepagefree("/proc/meminfo", topology);

  /* sort levels according to cpu sets */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      /* first sort sublevels according to cpu sets */
      qsort(&topology->levels[l+1][0], topology->level_nbitems[l+1], sizeof(*topology->levels[l+1]), compar);
      k = 0;
      /* then gather sublevels according to levels */
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  lt_cpuset_t level_set = topology->levels[l][i].cpuset;
	  for (j=k; j<topology->level_nbitems[l+1]; j++)
	    {
	      lt_cpuset_t set = level_set;
	      lt_cpuset_andset(&set, &topology->levels[l+1][j].cpuset);
	      if (!lt_cpuset_iszero(&set))
		{
		  /* Sublevel j is part of level i, put it at k.  */
		  struct lt_level level = topology->levels[l+1][j];
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
	ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));
      }

  /* And show debug again */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));

  /* Compute arity */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
	{
	  topology->levels[l][i].arity=0;
	  for (j=0; !lt_cpuset_iszero(&topology->levels[l+1][j].cpuset); j++)
	    if (lt_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
	      topology->levels[l][i].arity++;
	  ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
		  l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	}
    }


  for (i=0; !lt_cpuset_iszero(&topology->levels[topology->nb_levels-1][i].cpuset); i++)
    ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" leaf\n",
	    topology->nb_levels-1, i, LT_CPUSET_PRINTF_VALUE(topology->levels[topology->nb_levels-1][i].cpuset));
  ltdebug("arity done.\n");

  /* and finally connect levels */
  topo_connect(topology);
  ltdebug("connecting done.\n");

  /* intialize all depth to unknown */
  for (l=0; l < LT_LEVEL_MAX; l++)
    topology->type_depth[l] = -1;

  /* walk the existing levels to set their depth */
  for (l=0; l<topology->nb_levels; l++)
    topology->type_depth[topology->levels[l][0].type] = l;

  /* setup the depth of all still unknown levels (the one that got merged or never created */
  int type, prevdepth = -1;
  for (type = 0; type < LT_LEVEL_MAX; type++)
    {
      if (topology->type_depth[type] == -1)
	topology->type_depth[type] = prevdepth;
      else
	prevdepth = topology->type_depth[type];
    }
}

int
topo_init (lt_topo_t *topology)
{
  if(!topology)
    return 0;

  lt_setup_topo(topology);
  topo_discover(topology);

  return 1;
}

void
topo_fini (lt_topo_t *topology)
{
  unsigned l,i;
  /* Last level is not freed because we need it in various places */
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
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
}
