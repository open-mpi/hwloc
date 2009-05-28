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
topo_admin_disable_set_from_cpuset(struct topo_topology *topology,
				   const char *name,
				   topo_cpuset_t *admin_disabled_set)
{
#define CPUSET_MASK_LEN 64
  char cpuset_mask[CPUSET_MASK_LEN];
  char *current, *comma, *tmp;
  int prevlast, nextfirst, nextlast; /* beginning/end of enabled-segments */
  int ret;

  ret = topo_read_cpuset_mask(name, cpuset_mask, CPUSET_MASK_LEN, topology->backend_params.sysfs.root_fd);
  if (!ret)
    return;

  topo_debug("found cpuset %s: %s\n", name, cpuset_mask);

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
      topo_debug("%s [%d:%d] excluded by cpuset\n", name, prevlast+1, nextfirst-1);
      topo_cpuset_set_range(admin_disabled_set, prevlast+1, nextfirst-1);
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
    topo_debug("%s [%d:%d] excluded by cpuset\n", name, prevlast+1, nextfirst-1);
    topo_cpuset_set_range(admin_disabled_set, prevlast+1, nextfirst-1);
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
look_sysfsnode(struct topo_topology *topology,
	       topo_cpuset_t *admin_disabled_mems_set)
{
  unsigned osnode;
  unsigned nbnodes = 1;
  DIR *dir;
  struct dirent *dirent;
  topo_obj_t node;

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
    return;

  for (osnode=0; osnode < nbnodes; osnode++)
    {
      char nodepath[SYSFS_NUMA_NODE_PATH_LEN];
      topo_cpuset_t cpuset;
      unsigned long size = -1;
      unsigned long hpfree = -1;

      sprintf(nodepath, "/sys/devices/system/node/node%u/cpumap", osnode);
      if (topo_parse_cpumap(nodepath, &cpuset, topology->backend_params.sysfs.root_fd) < 0)
	continue;

      if (topo_cpuset_isset(admin_disabled_mems_set, osnode)) {
	size = 0; hpfree = 0;
      } else
	topo_sysfs_node_meminfo_info(topology, osnode, &size, &hpfree);

      node = topo_alloc_setup_object(TOPO_OBJ_NODE, osnode);
      node->memory_kB = size;
      node->huge_page_free = hpfree;
      node->cpuset = cpuset;

      topo_debug("node %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 i, osnode, TOPO_CPUSET_PRINTF_VALUE(node->cpuset));
      topo_add_object(topology, node);
    }
}

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void
look_sysfscpu(struct topo_topology *topology,
	      topo_cpuset_t *admin_disabled_cpus_set)
{
  struct topo_obj **socket_level = NULL;
  unsigned nsockets = 0;
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
      if (topo_cpuset_isset(admin_disabled_cpus_set, cpu)) {
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
      topo_cpuset_t socketset, coreset, threadset;
      unsigned mysocketid, mycoreid;
      int index;

      /* look at the socket */
      mysocketid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", i);
      topo_parse_sysfs_unsigned(string, &mysocketid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_siblings", i);
      topo_parse_cpumap(string, &socketset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&socketset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&socketset) >= 1);

      if (topo_cpuset_first(&socketset) == i) {
	/* first cpu in this socket, add the socket */
	if (!nsockets) {
	  socket_level = malloc(nbprocessors * sizeof(*socket_level));
	  assert(socket_level);
	}
	index = nsockets;
	socket_level[index] = topo_alloc_setup_object(TOPO_OBJ_SOCKET, mysocketid);
	socket_level[index]->cpuset = socketset;
	topo_debug("socket %d os number %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   index, mysocketid, TOPO_CPUSET_PRINTF_VALUE(socketset));
	nsockets++;
      }

      /* look at the core */
      mycoreid = 0; /* shut-up the compiler */
      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/core_id", i);
      topo_parse_sysfs_unsigned(string, &mycoreid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "/sys/devices/system/cpu/cpu%d/topology/thread_siblings", i);
      topo_parse_cpumap(string, &coreset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&coreset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&coreset) >= 1);

      if (topo_cpuset_first(&coreset) == i) {
	/* first cpu in this core, add the core */
	if (!ncores) {
	  core_level = malloc(nbprocessors * sizeof(*core_level));
	  assert(core_level);
	}
	index = ncores;
	core_level[index] = topo_alloc_setup_object(TOPO_OBJ_CORE, mycoreid);
	core_level[index]->cpuset = coreset;
	topo_debug("core %d os number %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   index, mycoreid, TOPO_CPUSET_PRINTF_VALUE(coreset));
	ncores++;
      }

      /* look at the thread */
      topo_cpuset_cpu(&threadset, i);
      topo_cpuset_clearset(&threadset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&threadset) == 1);

      /* add the thread */
      if (!nthreads) {
	thread_level = malloc(nbprocessors * sizeof(*thread_level));
	assert(thread_level);
      }
      index = nthreads;
      thread_level[index] = topo_alloc_setup_object(TOPO_OBJ_PROC, i);
      thread_level[index]->cpuset = threadset;
      topo_debug("thread %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 index, TOPO_CPUSET_PRINTF_VALUE(threadset));
      nthreads++;

      /* look at the caches */
      for(j=0; j<10; j++) {
#define SHARED_CPU_MAP_STRLEN (27+9+12+1+15+1)
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

	/* TODO we could just use the mask instead of building an array of cache levels */
	sprintf(mappath, "/sys/devices/system/cpu/cpu%d/cache/index%d/shared_cpu_map", i, j);
	topo_parse_cpumap(mappath, &cacheset, topology->backend_params.sysfs.root_fd);
	topo_cpuset_clearset(&cacheset, admin_disabled_cpus_set);
	if (topo_cpuset_weight(&cacheset) < 1)
	  /* mask is wrong (happens on ia64), assumes it's not shared */
	  topo_cpuset_cpu(&cacheset, i);

	if (topo_cpuset_first(&cacheset) == i) {
	  /* first cpu in this cache, add the cache */
	  if (!ncaches[depth]) {
	    cache_level[depth] = malloc(nbprocessors * sizeof(*core_level));
	    assert(cache_level[depth]);
	  }
	  index = ncaches[depth];
	  cache_level[depth][index] = topo_alloc_setup_object(TOPO_OBJ_CACHE, -1);
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

  if (socket_level)
    topo_add_level(topology, socket_level, nsockets);

  if (core_level)
    topo_add_level(topology, core_level, ncores);

  for(i=TOPO_CACHE_LEVEL_MAX-1; i>=0; i--)
    if (cache_level[i])
      topo_add_level(topology, cache_level[i], ncaches[i]);

  if (thread_level)
    topo_add_level(topology, thread_level, nthreads);
}


/* Look at Linux' /proc/cpuinfo */
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"
static void
look_cpuinfo(struct topo_topology *topology,
	     topo_cpuset_t *admin_disabled_cpus_set)
{
  FILE *fd;
  char string[strlen(PHYSID)+1+9+1+1];
  char *endptr;
  unsigned proc_physids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned osphysids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_coreids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned oscoreids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_osphysids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned proc_oscoreids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  unsigned core_osphysids[] = { [0 ... TOPO_NBMAXCPUS-1] = -1 };
  topo_cpuset_t online_cpuset;
  unsigned procid_max=0;
  unsigned numprocs=0;
  unsigned numsockets=0;
  unsigned numcores=0;
  long physid;
  long coreid;
  long processor = -1;
  int i;

  topo_cpuset_zero(&online_cpuset);

  if (!(fd=topo_fopen("/proc/cpuinfo","r", topology->backend_params.sysfs.root_fd)))
    {
      fprintf(stderr,"could not open /proc/cpuinfo\n");
      return;
    }

  /* Just record information and count number of sockets and cores */

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
      topo_cpuset_set(&online_cpuset, processor);
      getprocnb_end() else
      getprocnb_begin(PHYSID,physid);
      proc_osphysids[processor]=physid;
      for (i=0; i<numsockets; i++)
	if (physid == osphysids[i])
	  break;
      proc_physids[processor]=i;
      topo_debug("%ld on socket %d (%lx)\n", processor, i, physid);
      if (i==numsockets)
	osphysids[(numsockets)++] = physid;
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
  procid_max = processor + 1;
  topology->nb_processors = numprocs = topo_cpuset_weight(&online_cpuset);

  /* clear admin-disabled cpus */
  topo_cpuset_foreach_begin(i, &online_cpuset) {
    if (topo_cpuset_isset(admin_disabled_cpus_set, i)) {
      topo_cpuset_clr(&online_cpuset, i);
      proc_osphysids[i] = -1;
      proc_physids[i] = -1;
      proc_oscoreids[i] = -1;
      proc_coreids[i] = -1;
    }
  } topo_cpuset_foreach_end();

  /* clear nonfirst threads.
   * we could fill a dedicated cpuset in the above parsing loop but
   * we'd have to check whether the first thread is admin-disabled.
   * so just do a final loop here looking for same core ids
   * now that admin-disabled cpus have been removed.
   */
  if (topology->flags & TOPO_FLAGS_IGNORE_THREADS) {
    topo_cpuset_foreach_begin(i, &online_cpuset) {
      int j;
      for(j=i+1; j<procid_max; j++)
	if (proc_coreids[j] == proc_coreids[i]) {
	  topo_cpuset_clr(&online_cpuset, j);
	  proc_osphysids[j] = -1;
	  proc_physids[j] = -1;
	  proc_oscoreids[j] = -1;
	  proc_coreids[j] = -1;
	}
    } topo_cpuset_foreach_end();
  }

  /* From here, topology->nb_processors is set to the number of available
   * hardware resources, and online_cpuset covers them.
   */
  topo_debug("%u online processors found, with id max %u\n", numprocs, procid_max);
  topo_debug("online processor cpuset: %" TOPO_PRIxCPUSET "\n",
	     TOPO_CPUSET_PRINTF_VALUE(online_cpuset));

  topo_debug("\n * Topology summary *\n");
  topo_debug("%d processors (%d max id)\n", numprocs, procid_max);

  topo_debug("%d sockets\n", numsockets);
  if (numsockets>0)
    topo_setup_level(procid_max, numsockets, osphysids, proc_physids, topology, TOPO_OBJ_SOCKET);

  topo_debug("%d cores\n", numcores);
  if (numcores>0)
    topo_setup_level(procid_max, numcores, oscoreids, proc_coreids, topology, TOPO_OBJ_CORE);

  /* setup the cpu level, removing nonfirst threads */
  topo_setup_proc_level(topology, &online_cpuset);
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
topo_look_linux(struct topo_topology *topology)
{
  topo_cpuset_t admin_disabled_cpus_set, admin_disabled_mems_set;

  /* Gather the list of admin-disabled cpus and mems */
  topo_cpuset_zero(&admin_disabled_cpus_set);
  topo_cpuset_zero(&admin_disabled_mems_set);
  if (!(topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE)) {
    topo_admin_disable_set_from_cpuset(topology, "cpus", &admin_disabled_cpus_set);
    topo_admin_disable_set_from_cpuset(topology, "mems", &admin_disabled_mems_set);
  }

  /* Gather NUMA information */
  look_sysfsnode(topology, &admin_disabled_mems_set);

  /* Gather the list of cpus now */
  if (getenv("TOPO_LINUX_USE_CPUINFO")
      || topo_access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
      || topo_access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0) {
      /* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
    look_cpuinfo(topology, &admin_disabled_cpus_set);
  } else {
    look_sysfscpu(topology, &admin_disabled_cpus_set);
  }

  /* Compute the whole machine memory and huge page */
  topo_get_procfs_meminfo_info(topology,
			       &topology->levels[0][0]->memory_kB,
			       &topology->huge_page_size_kB,
			       &topology->levels[0][0]->huge_page_free);
			       /* FIXME: gather page_size_kB as well? MaMI needs it */

  /* Gather DMI info */
  topo__get_dmi_info(topology);
}
