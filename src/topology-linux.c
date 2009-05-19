/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <topology.h>
#include <topology/helper.h>
#include <topology/debug.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_OPENAT

/* Use our own filesystem functions.  */
#define topo_fopen(p, m, d)   topo_fopenat(p, m, d)
#define topo_access(p, m, d)  topo_accessat(p, m, d)
#define topo_opendir(p, d)    topo_opendirat(p, d)

static FILE *
topo_fopenat(const char *path, const char *mode, int fsys_root_fd)
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

static int
topo_accessat(const char *path, int mode, int fsys_root_fd)
{
  const char *relative_path;

  assert(!(fsys_root_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  return faccessat(fsys_root_fd, relative_path, O_RDONLY, 0);
}

static DIR*
topo_opendirat(const char *path, int fsys_root_fd)
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

#else /* !HAVE_OPENAT */

#define topo_fopen(p, m, d)   fopen(p, m)
#define topo_access(p, m, d)  access(p, m)
#define topo_opendir(p, d)    opendir(p)

#endif /* !HAVE_OPENAT */

int
topo_backend_sysfs_init(struct topo_topology *topology, const char *fsys_root_path)
{
#ifdef HAVE_OPENAT
  char *fsys_root_path_env;
  int root;

  assert(topology->backend_type == TOPO_BACKEND_NONE);

  /* Use the root path from the environment variable first,
   * then from the given argument, then the default root.
   */
  fsys_root_path_env = getenv("TOPO_FSYS_ROOT_PATH");
  if (fsys_root_path_env)
    fsys_root_path = fsys_root_path_env;
  if (!fsys_root_path)
    fsys_root_path = "/";

  root = open(fsys_root_path, O_RDONLY | O_DIRECTORY);
  if (root < 0)
    return -1;

  if (strcmp(fsys_root_path, "/"))
    topology->is_fake = 1;

  topology->backend_params.sysfs.root_fd = root;
#else
  topology->backend_params.sysfs.root_fd = -1;
#endif
  topology->backend_type = TOPO_BACKEND_SYSFS;
  return 0;
}

void
topo_backend_sysfs_exit(struct topo_topology *topology)
{
  assert(topology->backend_type == TOPO_BACKEND_SYSFS);
#ifdef HAVE_OPENAT
  close(topology->backend_params.sysfs.root_fd);
#endif
  topology->backend_type = TOPO_BACKEND_NONE;
}

static int
topo_parse_sysfs_unsigned(const char *mappath, unsigned *value, int fsys_root_fd)
{
  char string[11];
  FILE * fd;

  fd = topo_fopen(mappath, "r", fsys_root_fd);
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
#define MAX_KERNEL_CPU_MASK ((TOPO_NBMAXCPUS+KERNEL_CPU_MASK_BITS-1)/KERNEL_CPU_MASK_BITS)

static int
topo_parse_cpumap(const char *mappath, topo_cpuset_t *set, int fsys_root_fd)
{
  char string[KERNEL_CPU_MAP_LEN]; /* enough for a shared map mask (32bits hexa) */
  unsigned long maps[MAX_KERNEL_CPU_MASK];
  int nr_maps = 0;
  FILE * fd;
  int i;

  /* reset to zero first */
  topo_cpuset_zero(set);

  fd = topo_fopen(mappath, "r", fsys_root_fd);
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
  assert(!(nr_maps*KERNEL_CPU_MASK_BITS > TOPO_NBMAXCPUS));

  /* convert into a set */
  for(i=0; i<nr_maps*KERNEL_CPU_MASK_BITS; i++)
    if (maps[i/KERNEL_CPU_MASK_BITS] & 1<<(i%KERNEL_CPU_MASK_BITS))
      topo_cpuset_set(set, i);

  fclose(fd);

  return 0;
}

static void
topo_process_shared_cpu_map(const char *mappath, const char * mapname, unsigned long val,
			    int procid_max, unsigned *ids, unsigned long *vals,
			    unsigned *nr_ids, unsigned givenid, topo_cpuset_t *online_cpuset,
			    int fsys_root_fd)
{
  topo_cpuset_t set;
  int k;

  topo_parse_cpumap(mappath, &set, fsys_root_fd);
  for(k=0; k<procid_max; k++)
    {
      if (topo_cpuset_isset(&set, k))
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
	      if (topo_cpuset_isset(&set, k))
		{
		  topo_debug("--- proc %d has %s number %d\n", k, mapname, newid);
		  ids[k] = newid;
		  vals[newid] = val;
		}
	    }

	  break;
	}
    }
}

static void
topo_parse_cache_shared_cpu_maps(int proc_index, int procid_max, topo_cpuset_t *online_cpuset,
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
    fd = topo_fopen(mappath, "r", fsys_root_fd);
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
    fd = topo_fopen(mappath, "r", fsys_root_fd);
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
    fd = topo_fopen(mappath, "r", fsys_root_fd);
    if (fd)
      {
	if (fgets(string,sizeof(string), fd))
	  kB = atol(string); /* in kB */
	fclose(fd);
      }

    sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", proc_index, i);
    sprintf(cachename, "L%d cache", level+1);
    topo_process_shared_cpu_map(mappath, cachename, kB,
				procid_max, cacheids+level*TOPO_NBMAXCPUS,
				cachesizes+level*TOPO_NBMAXCPUS,
				&nr_caches[level], -1, online_cpuset,
				fsys_root_fd);
  }
}

static int
topo_read_cpuset_mask(const char *type, char *info, int infomax, int fsys_root_fd)
{
  char filename[] = "/proc/self/cpuset";
#define CPUSET_NAME_LEN 64
  char cpuset_name[CPUSET_NAME_LEN];
#define CPUSET_FILENAME_LEN 64
  char cpuset_filename[CPUSET_FILENAME_LEN];
  char *tmp;
  FILE *fd;

  /* check whether a cpuset is enabled */
  fd = topo_fopen(filename, "r", fsys_root_fd);
  if (!fd)
    return 0;

  fgets(cpuset_name, sizeof(cpuset_name), fd);
  fclose(fd);

  tmp = strchr(cpuset_name, '\n');
  if (tmp)
    *tmp = '\0';

  /* read the cpuset */
  snprintf(cpuset_filename, CPUSET_FILENAME_LEN, "/dev/cpuset%s/%s", cpuset_name, type);
  topo_debug("Trying to read cpuset file <%s>\n", cpuset_filename);
  fd = topo_fopen(cpuset_filename, "r", fsys_root_fd);
  if (!fd) {
    snprintf(cpuset_filename, CPUSET_FILENAME_LEN, "/cpusets%s/%s", cpuset_name, type);
    topo_debug("Trying to read cpuset file <%s>\n", cpuset_filename);
    fd = topo_fopen(cpuset_filename, "r", fsys_root_fd);
  }

  if (!fd)
    return 0;

  fgets(info, infomax, fd);
  fclose(fd);

  tmp = strchr(info, '\n');
  if (tmp)
    *tmp = '\0';

  return 1;
}

static void
topo_admin_disable_mems_from_cpuset(struct topo_topology *topology, int nodelevel)
{
  struct topo_obj **level = topology->levels[nodelevel];
  int nbobjects = topology->level_nbobjects[nodelevel];
#define CPUSET_MASK_LEN 64
  char cpuset_mask[CPUSET_MASK_LEN];
  char *current, *comma, *tmp;
  int prevlast, nextfirst, nextlast; /* beginning/end of enabled-segments */
  int i, ret;

  ret = topo_read_cpuset_mask("mems", cpuset_mask, CPUSET_MASK_LEN, topology->backend_params.sysfs.root_fd);
  if (!ret)
    return;

  topo_debug("found cpuset mems: %s\n", cpuset_mask);

  current = cpuset_mask;
  prevlast = -1;

  while (1) {
    /* save a pointer to the next comma and erase it to simplify things */
    comma = strchr(current, ',');
    if (comma)
      *comma = '\0';

    /* find current enabled-segment bounds */
    nextfirst = strtoul(current, &tmp, 0);
    if (*tmp == '-')
      nextlast = strtoul(tmp+1, NULL, 0);
    else
      nextlast = nextfirst;
    if (prevlast+1 <= nextfirst-1)
      topo_debug("mems [%d:%d] excluded by cpuset\n", prevlast+1, nextfirst-1);
    for(i=prevlast+1; i<=nextfirst-1; i++) {
      level[i]->memory_kB = 0;
      level[i]->huge_page_free = 0;
    }

    /* switch to next enabled-segment */
    prevlast = nextlast;
    if (!comma)
      break;
    current = comma+1;
  }

  /* disable after last enabled-segment */
  nextfirst = nbobjects;
  if (prevlast+1 <= nextfirst-1)
    topo_debug("mems [%d:%d] excluded by cpuset\n", prevlast+1, nextfirst-1);
  for(i=prevlast+1; i<=nextfirst-1; i++) {
    level[i]->memory_kB = 0;
    level[i]->huge_page_free = 0;
  }
}

static void
topo_admin_disable_cpus_from_cpuset(struct topo_topology *topology,
				    topo_cpuset_t *admin_disabled_cpuset)
{
#define CPUSET_MASK_LEN 64
  char cpuset_mask[CPUSET_MASK_LEN];
  char *current, *comma, *tmp;
  int prevlast, nextfirst, nextlast; /* beginning/end of enabled-segments */
  int ret;

  ret = topo_read_cpuset_mask("cpus", cpuset_mask, CPUSET_MASK_LEN, topology->backend_params.sysfs.root_fd);
  if (!ret)
    return;

  topo_debug("found cpuset cpus: %s\n", cpuset_mask);

  current = cpuset_mask;
  prevlast = -1;

  while (1) {
    /* save a pointer to the next comma and erase it to simplify things */
    comma = strchr(current, ',');
    if (comma)
      *comma = '\0';

    /* find current enabled-segment bounds */
    nextfirst = strtoul(current, &tmp, 0);
    if (*tmp == '-')
      nextlast = strtoul(tmp+1, NULL, 0);
    else
      nextlast = nextfirst;
    if (prevlast+1 <= nextfirst-1) {
      topo_debug("cpus [%d:%d] excluded by cpuset\n", prevlast+1, nextfirst-1);
      topo_cpuset_set_range(admin_disabled_cpuset, prevlast+1, nextfirst-1);
    }

    /* switch to next enabled-segment */
    prevlast = nextlast;
    if (!comma)
      break;
    current = comma+1;
  }

  /* disable after last enabled-segment */
  nextfirst = TOPO_NBMAXCPUS;
  if (prevlast+1 <= nextfirst-1) {
    topo_debug("cpus [%d:%d] excluded by cpuset\n", prevlast+1, nextfirst-1);
    topo_cpuset_set_range(admin_disabled_cpuset, prevlast+1, nextfirst-1);
  }
}

static void
topo_get_procfs_meminfo_info(struct topo_topology *topology,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_size_kB,
			     unsigned long *huge_page_free)
{
  char string[64];
  FILE *fd;

  fd = topo_fopen("/proc/meminfo", "r", topology->backend_params.sysfs.root_fd);
  if (!fd)
    return;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      unsigned long number;
      if (sscanf(string, "MemTotal: %lu kB", &number) == 1)
	*mem_size_kB = number;
      else if (sscanf(string, "Hugepagesize: %lu", &number) == 1)
	*huge_page_size_kB = number;
      else if (sscanf(string, "HugePages_Free: %lu", &number) == 1)
	*huge_page_free = number;
    }

  fclose(fd);
}

#define SYSFS_NUMA_NODE_PATH_LEN (29+9+8+1)

static void
topo_sysfs_node_meminfo_info(struct topo_topology *topology,
			     int node,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_free)
{
  char path[SYSFS_NUMA_NODE_PATH_LEN];
  char string[64];
  FILE *fd;

  sprintf(path, "/sys/devices/system/node/node%d/meminfo", node);
  fd = topo_fopen(path, "r", topology->backend_params.sysfs.root_fd);
  if (!fd)
    return;

  while (fgets(string, sizeof(string), fd) && *string != '\0')
    {
      unsigned long number;
      if (sscanf(string, "Node %d MemTotal: %lu kB", &node, &number) == 2)
	*mem_size_kB = number;
      else if (sscanf(string, "Node %d HugePages_Free: %lu kB", &node, &number) == 2)
	*huge_page_free = number;
    }

  fclose(fd);
}

static void
look_sysfsnode(struct topo_topology *topology)
{
  unsigned i, osnode;
  unsigned nbnodes = 1;
  struct topo_obj **node_level;
  DIR *dir;
  struct dirent *dirent;

  dir = topo_opendir("/sys/devices/system/node", topology->backend_params.sysfs.root_fd);
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

  node_level = calloc(nbnodes+1, sizeof(*node_level));
  assert(node_level);

  for (i=0, osnode=0;; osnode++)
    {
      char nodepath[SYSFS_NUMA_NODE_PATH_LEN];
      topo_cpuset_t cpuset;
      unsigned long size = -1;
      unsigned long hpfree = -1;

      sprintf(nodepath, "/sys/devices/system/node/node%u/cpumap", osnode);
      if (topo_parse_cpumap(nodepath, &cpuset, topology->backend_params.sysfs.root_fd) < 0)
	break;

      topo_sysfs_node_meminfo_info(topology, osnode, &size, &hpfree);

      node_level[i] = malloc(sizeof(struct topo_obj));
      assert(node_level[i]);
      topo_setup_object(node_level[i], TOPO_OBJ_NODE, osnode);
      node_level[i]->memory_kB = size;
      node_level[i]->huge_page_free = hpfree;
      node_level[i]->cpuset = cpuset;

      topo_debug("node %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 i, osnode, TOPO_CPUSET_PRINTF_VALUE(node_level[i]->cpuset));
      i++;
    }
  nbnodes = i;

  topology->level_nbobjects[topology->nb_levels] = topology->nb_nodes = nbnodes;
  topology->levels[topology->nb_levels++] = node_level;

  /* Now that we have gathered NUMA node information,
   * empty the admin-disabled ones */
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE))
    topo_admin_disable_mems_from_cpuset(topology, topology->nb_levels-1);
}

static void
topo_linux_setup_cpu_level(struct topo_topology *topology,
			   topo_cpuset_t *online_cpuset,
			   topo_cpuset_t *nonfirst_threads_cpuset)
{
  struct topo_obj **cpu_level;
  unsigned oscpu,cpu;

  cpu_level = calloc(topology->nb_processors+1, sizeof(*cpu_level));
  assert(cpu_level);

  topo_debug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; oscpu++)
    {
      if (!topo_cpuset_isset(online_cpuset, oscpu))
       continue;

      if ((topology->flags & TOPO_FLAGS_IGNORE_THREADS)
         && topo_cpuset_isset(nonfirst_threads_cpuset, oscpu)) {
       topology->nb_processors--;
       continue;
      }

      cpu_level[cpu] = malloc(sizeof(struct topo_obj));
      assert(cpu_level[cpu]);
      topo_setup_object(cpu_level[cpu], TOPO_OBJ_PROC, oscpu);

      topo_cpuset_cpu(&cpu_level[cpu]->cpuset, oscpu);

      topo_debug("cpu %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(cpu_level[cpu]->cpuset));
      cpu++;
    }

  topology->level_nbobjects[topology->nb_levels]=topology->nb_processors;
  topology->levels[topology->nb_levels++]=cpu_level;
}

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void
look__sysfscpu(struct topo_topology *topology,
	       topo_cpuset_t *admin_disabled_cpuset)
{
  struct topo_obj **die_level = NULL;
  unsigned ndies = 0;
  struct topo_obj **core_level = NULL;
  unsigned ncores = 0;
  struct topo_obj **thread_level = NULL;
  unsigned nthreads = 0;
  struct topo_obj **cache_level[] = { [0 ... TOPO_CACHE_LEVEL_MAX-1 ] = NULL };
  unsigned ncaches[] = { [0 ... TOPO_CACHE_LEVEL_MAX-1 ] = 0 };
  topo_cpuset_t cpuset;
  int nbprocessors;
#define CPU_TOPOLOGY_STR_LEN (27+9+29+1)
  char string[CPU_TOPOLOGY_STR_LEN];
  DIR *dir;
  int i,j;
  FILE *fd;

  /* fill the cpuset of interesting cpus */
  topo_cpuset_zero(&cpuset);
  dir = topo_opendir("/sys/devices/system/cpu", topology->backend_params.sysfs.root_fd);
  if (dir) {
    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL) {
      unsigned long cpu;
      char online[2];

      if (strncmp(dirent->d_name, "cpu", 3))
	continue;
      cpu = strtoul(dirent->d_name+3, NULL, 0);

      assert(cpu < TOPO_NBMAXCPUS);

      /* check whether cpusets exclude this cpu */
      if (topo_cpuset_isset(admin_disabled_cpuset, cpu)) {
	topo_debug("os proc %ld is disabled by the administrator\n", cpu);
	continue;
      }

      /* check whether the kernel exports topology information for this cpu */
      sprintf(string, "/sys/devices/system/cpu/cpu%ld/topology", cpu);
      if (topo_access(string, X_OK, topology->backend_params.sysfs.root_fd) < 0 && errno == ENOENT) {
	topo_debug("os proc %ld has no accessible /sys/devices/system/cpu/cpu%ld/topology\n",
		   cpu, cpu);
	continue;
      }

      /* check whether this processor is offline */
      sprintf(string, "/sys/devices/system/cpu/cpu%ld/online", cpu);
      fd = topo_fopen(string, "r", topology->backend_params.sysfs.root_fd);
      if (fd) {
	if (fgets(online, sizeof(online), fd)) {
	  fclose(fd);
	  if (atoi(online)) {
	    topo_debug("os proc %ld is online\n", cpu);
	  } else {
	    topo_debug("os proc %ld is offline\n", cpu);
	    continue;
	  }
	} else {
	  fclose(fd);
	}
      }

      if (topology->flags & TOPO_FLAGS_IGNORE_THREADS) {
	/* check whether it is a non-first thread in the core */
	topo_cpuset_t coreset;
	sprintf(string, "/sys/devices/system/cpu/cpu%ld/topology/thread_siblings", cpu);
	topo_parse_cpumap(string, &coreset, topology->backend_params.sysfs.root_fd);
	if (topo_cpuset_first(&coreset) != cpu) {
	  topo_debug("os proc %ld is not first thread in coreset %" TOPO_PRIxCPUSET "\n",
		     cpu, TOPO_CPUSET_PRINTF_VALUE(coreset));
	  continue;
	}
      }

      topo_cpuset_set(&cpuset, cpu);
    }
    closedir(dir);
  }

  topology->nb_processors = nbprocessors = topo_cpuset_weight(&cpuset);;
  topo_debug("found %d cpus, cpuset %" TOPO_PRIxCPUSET "\n",
	     nbprocessors, TOPO_CPUSET_PRINTF_VALUE(cpuset));

  topo_cpuset_foreach_begin(i, &cpuset)
    {
      topo_cpuset_t dieset, coreset, threadset;
      unsigned mydieid, mycoreid;
      int index;

      /* look at the die */
      mydieid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
      topo_parse_sysfs_unsigned(string, &mydieid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_siblings", i);
      topo_parse_cpumap(string, &dieset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&dieset, admin_disabled_cpuset);
      assert(topo_cpuset_weight(&dieset) >= 1);

      if (topo_cpuset_first(&dieset) == i) {
	/* first cpu in this die, add the die */
	if (!ndies) {
	  die_level = calloc(nbprocessors+1, sizeof(*die_level));
	  assert(die_level);
	}
	index = ndies;
	die_level[index] = malloc(sizeof(struct topo_obj));
	assert(die_level[index]);
	topo_setup_object(die_level[index], TOPO_OBJ_DIE, mydieid);
	die_level[index]->cpuset = dieset;
	topo_debug("die %d os number %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   index, mydieid, TOPO_CPUSET_PRINTF_VALUE(dieset));
	ndies++;
      }

      /* look at the core */
      mycoreid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_id", i);
      topo_parse_sysfs_unsigned(string, &mycoreid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", i);
      topo_parse_cpumap(string, &coreset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&coreset, admin_disabled_cpuset);
      assert(topo_cpuset_weight(&coreset) >= 1);

      if (topo_cpuset_first(&coreset) == i) {
	/* first cpu in this core, add the core */
	if (!ncores) {
	  core_level = calloc(nbprocessors+1, sizeof(*core_level));
	  assert(core_level);
	}
	index = ncores;
	core_level[index] = malloc(sizeof(struct topo_obj));
	assert(core_level[index]);
	topo_setup_object(core_level[index], TOPO_OBJ_CORE, mycoreid);
	core_level[index]->cpuset = coreset;
	topo_debug("core %d os number %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   index, mycoreid, TOPO_CPUSET_PRINTF_VALUE(coreset));
	ncores++;
      }

      /* look at the thread */
      topo_cpuset_cpu(&threadset, i);
      topo_cpuset_clearset(&threadset, admin_disabled_cpuset);
      assert(topo_cpuset_weight(&threadset) == 1);

      /* add the thread */
      if (!nthreads) {
	thread_level = calloc(nbprocessors+1, sizeof(*thread_level));
	assert(thread_level);
      }
      index = nthreads;
      thread_level[index] = malloc(sizeof(struct topo_obj));
      assert(thread_level[index]);
      topo_setup_object(thread_level[index], TOPO_OBJ_PROC, i);
      thread_level[index]->cpuset = threadset;
      topo_debug("thread %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 index, TOPO_CPUSET_PRINTF_VALUE(threadset));
      nthreads++;

      /* look at the caches */
      for(j=0; j<10; j++) {
	char mappath[SHARED_CPU_MAP_STRLEN];
	char string[20]; /* enough for a level number (one digit) or a type (Data/Instruction/Unified) */
	topo_cpuset_t cacheset;
	unsigned long kB = 0;
	int depth; /* 0 for L1, .... */
	FILE * fd;

	/* get the cache level depth */
	sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/level", i, j);
	fd = topo_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(string,sizeof(string), fd))
	    depth = strtoul(string, NULL, 10)-1;
	  else
	    continue;
	  fclose(fd);
	} else
	  continue;

	/* ignore Instruction caches */
	sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/type", i, j);
	fd = topo_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(string, sizeof(string), fd)) {
	    fclose(fd);
	    if (!strncmp(string, "Instruction", 11))
	      continue;
	  } else {
	    fclose(fd);
	    continue;
	  }
	} else
	  continue;

	/* get the cache size */
	sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/size", i, j);
	fd = topo_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(string,sizeof(string), fd))
	    kB = atol(string); /* in kB */
	  fclose(fd);
	}

	sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", i, j);
	topo_parse_cpumap(mappath, &cacheset, topology->backend_params.sysfs.root_fd);
	topo_cpuset_clearset(&cacheset, admin_disabled_cpuset);
	if (topo_cpuset_weight(&cacheset) < 1)
	  /* mask is wrong (happens on ia64), assumes it's not shared */
	  topo_cpuset_cpu(&cacheset, i);

	if (topo_cpuset_first(&cacheset) == i) {
	  /* first cpu in this cache, add the cache */
	  if (!ncaches[depth]) {
	    cache_level[depth] = calloc(nbprocessors+1, sizeof(*core_level));
	    assert(cache_level[depth]);
	  }
	  index = ncaches[depth];
	  cache_level[depth][index] = malloc(sizeof(struct topo_obj));
	  assert(cache_level[depth][index]);
	  topo_setup_object(cache_level[depth][index], TOPO_OBJ_CACHE, -1);
	  cache_level[depth][index]->memory_kB = kB;
	  cache_level[depth][index]->cache_depth = depth+1;
	  cache_level[depth][index]->cpuset = cacheset;
	  topo_debug("cache %d depth %d has cpuset %"TOPO_PRIxCPUSET"\n",
		     ncaches[depth], depth, TOPO_CPUSET_PRINTF_VALUE(cacheset));
	  ncaches[depth]++;
	}
      }
    }
  topo_cpuset_foreach_end();

  /* add levels to the topology */

  if (die_level) {
    topology->level_nbobjects[topology->nb_levels] = ndies;
    topo_debug("--- die level has number %d\n", topology->nb_levels);
    topology->levels[topology->nb_levels++] = die_level;
    topo_debug("\n");
  }

  if (core_level) {
    topology->level_nbobjects[topology->nb_levels] = ncores;
    topo_debug("--- core level has number %d\n", topology->nb_levels);
    topology->levels[topology->nb_levels++] = core_level;
    topo_debug("\n");
  }

  for(i=TOPO_CACHE_LEVEL_MAX-1; i>=0; i--) {
    if (cache_level[i]) {
      topology->level_nbobjects[topology->nb_levels] = ncaches[i];
      topo_debug("--- cache level depth %d has number %d\n", i, topology->nb_levels);
      topology->levels[topology->nb_levels++] = cache_level[i];
      topo_debug("\n");
    }
  }

  if (thread_level) {
    topology->level_nbobjects[topology->nb_levels] = nthreads;
    topo_debug("--- thread level has number %d\n", topology->nb_levels);
    topology->levels[topology->nb_levels++] = thread_level;
    topo_debug("\n");
  }
}


/* Look at Linux' /proc/cpuinfo */
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"
static void
look_cpuinfo(struct topo_topology *topology,
	     topo_cpuset_t *admin_disabled_cpuset)
{
  FILE *fd;
  char string[strlen(PHYSID)+1+9+1+1];
  char *endptr;
  unsigned proc_physids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned osphysids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_coreids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned oscoreids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_cacheids[] = { [0 ... TOPO_CACHE_LEVEL_MAX*TOPO_NBMAXCPUS-1] = -1 };
  unsigned long cache_sizes[] = { [0 ... TOPO_CACHE_LEVEL_MAX*TOPO_NBMAXCPUS-1] = 0 };
  unsigned proc_osphysids[TOPO_NBMAXCPUS];
  unsigned proc_oscoreids[TOPO_NBMAXCPUS];
  unsigned core_osphysids[TOPO_NBMAXCPUS];
  topo_cpuset_t online_cpuset, nonfirst_threads_cpuset;
  unsigned procid_max=0;
  unsigned numprocs=0;
  unsigned numdies=0;
  unsigned numcores=0;
  unsigned numcaches[] = { [0 ... TOPO_CACHE_LEVEL_MAX-1] = 0 };
  long physid;
  long coreid;
  long processor = -1;
  int i, j;

  topo_cpuset_zero(&online_cpuset);
  topo_cpuset_zero(&nonfirst_threads_cpuset);

  /* FIXME: support admin_disabled_cpuset here as well */
  /* FIXME: support nonfirst_threads_cpuset here as well */

  memset(proc_physids,0,sizeof(proc_physids));
  memset(proc_osphysids,0,sizeof(proc_osphysids));
  memset(proc_coreids,0,sizeof(proc_coreids));
  memset(proc_oscoreids,0,sizeof(proc_oscoreids));

  if (!(fd=topo_fopen("/proc/cpuinfo","r", topology->backend_params.sysfs.root_fd)))
    {
      fprintf(stderr,"could not open /proc/cpuinfo\n");
      return;
    }

  /* Just record information and count number of dies and cores */

  topo_debug("\n\n * Topology extraction from /proc/cpuinfo *\n\n");
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
	topo_debug(field " %ld\n", var)
#      define getprocnb_end()			\
      }
      getprocnb_begin(PROCESSOR,processor);
      getprocnb_end() else
      getprocnb_begin(PHYSID,physid);
      proc_osphysids[processor]=physid;
      for (i=0; i<numdies; i++)
	if (physid == osphysids[i])
	  break;
      proc_physids[processor]=i;
      topo_debug("%ld on die %d (%lx)\n", processor, i, physid);
      if (i==numdies)
	osphysids[(numdies)++] = physid;
      getprocnb_end() else
      getprocnb_begin(COREID,coreid);
      proc_oscoreids[processor]=coreid;
      for (i=0; i<numcores; i++)
	if (coreid == oscoreids[i] && proc_osphysids[processor] == core_osphysids[i])
	  break;
      proc_coreids[processor]=i;
      topo_debug("%ld on core %d (%lx:%x)\n", processor, i, coreid, proc_osphysids[processor]);
      if (i==numcores)
	{
	  core_osphysids[numcores] = proc_osphysids[processor];
	  oscoreids[numcores] = coreid;
	  (numcores)++;
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
  numprocs = processor + 1;
  procid_max = processor + 1;
  topo_debug("%s: found %u procs\n", __func__, numprocs);

  /* we have a contigous range of online cpus */
  topo_cpuset_set_range(&online_cpuset, 0, processor);

  topo_debug("\n * Topology summary *\n");
  topo_debug("%d processors (%d max id)\n", numprocs, procid_max);

  if (numdies>1)
    topo_debug("%d dies\n", numdies);
  if (numcores>1)
    topo_debug("%d cores\n", numcores);

  if (numdies>1)
    topo_setup_die_level(procid_max, numdies, osphysids, proc_physids, topology);

  for(j=0; j<procid_max; j++)
    topo_parse_cache_shared_cpu_maps(j, procid_max, &online_cpuset,
				     proc_cacheids, cache_sizes, numcaches, topology->backend_params.sysfs.root_fd);

  if (numcores>1)
    topo_setup_core_level(procid_max, numcores, oscoreids, proc_coreids, topology);

  /* process caches at the end since we don't if they exist,
     and let the core code reorder levels
  */
  if (numcaches[2] > 0)
      /* setup L3 caches */
      topo_setup_cache_level(2, procid_max, numcaches, proc_cacheids, cache_sizes, topology);
  if (numcaches[1] > 0)
      /* setup L2 caches */
      topo_setup_cache_level(1, procid_max, numcaches, proc_cacheids, cache_sizes, topology);
  if (numcaches[0] > 0)
      /* setup L1 caches */
      topo_setup_cache_level(0, procid_max, numcaches, proc_cacheids, cache_sizes, topology);

  /* Override the default returned by `ma_fallback_nbprocessors ()'.  */
  topology->nb_processors = numprocs;
  topo_debug("%s: found %u procs\n", __func__, topology->nb_processors);

  /* From here, topology->nb_processors is set to the number of available
   * hardware resources, and online_cpuset covers them.
   */
  topo_debug("%d online processors found\n", topology->nb_processors);
  topo_debug("online processor cpuset: %" TOPO_PRIxCPUSET "\n",
	     TOPO_CPUSET_PRINTF_VALUE(online_cpuset));
  assert(topology->nb_processors == topo_cpuset_weight(&online_cpuset));

  /* setup the cpu level, removing nonfirst threads */
  topo_linux_setup_cpu_level(topology, &online_cpuset, &nonfirst_threads_cpuset);
  /* FIXME: use the common look_cpu ? */
}

static void
look_sysfscpu(struct topo_topology *topology)
{
  topo_cpuset_t admin_disabled_cpuset;

  /* gather the list of admin-disabled cpus */
  topo_cpuset_zero(&admin_disabled_cpuset);
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE))
    topo_admin_disable_cpus_from_cpuset(topology, &admin_disabled_cpuset);

  if (topo_access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0)
    {
      /* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
      /* cpuset cpus ignored */
      look_cpuinfo(topology, &admin_disabled_cpuset);
    }
  else
    {
      look__sysfscpu(topology, &admin_disabled_cpuset);
    }
}

static void
topo__get_dmi_info(struct topo_topology *topology)
{
#define DMI_BOARD_STRINGS_LEN 50
  char dmi_line[DMI_BOARD_STRINGS_LEN];
  char *tmp;
  FILE *fd;

  dmi_line[0] = '\0';
  fd = topo_fopen("/sys/class/dmi/id/board_vendor", "r", topology->backend_params.sysfs.root_fd);
  if (fd) {
    fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (dmi_line[0] != '\0') {
      tmp = strchr(dmi_line, '\n');
      if (tmp)
	*tmp = '\0';
      topology->dmi_board_vendor = strdup(dmi_line);
      topo_debug("found DMI board vendor '%s'\n", topology->dmi_board_vendor);
    }
  }

  dmi_line[0] = '\0';
  fd = topo_fopen("/sys/class/dmi/id/board_name", "r", topology->backend_params.sysfs.root_fd);
  if (fd) {
    fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (dmi_line[0] != '\0') {
      tmp = strchr(dmi_line, '\n');
      if (tmp)
	*tmp = '\0';
      topology->dmi_board_name = strdup(dmi_line);
      topo_debug("found DMI board name '%s'\n", topology->dmi_board_name);
    }
  }
}

void
look_linux(struct topo_topology *topology)
{
  look_sysfsnode(topology);
  look_sysfscpu(topology);

  /* Compute the whole machine memory and huge page */
  topo_get_procfs_meminfo_info(topology,
			       &topology->levels[0][0]->memory_kB,
			       &topology->huge_page_size_kB,
			       &topology->levels[0][0]->huge_page_free);
			       /* FIXME: gather page_size_kB as well? MaMI needs it */

  topo__get_dmi_info(topology);
}
