/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <hwloc/linux.h>
#include <private/private.h>
#include <private/debug.h>

#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <pthread.h>

#ifndef HWLOC_HAVE_CPU_SET
/* libc doesn't have support for sched_setaffinity, build system call
 * ourselves: */
#    include <linux/unistd.h>
#    ifndef __NR_sched_setaffinity
#       ifdef __i386__
#         define __NR_sched_setaffinity 241
#       elif defined(__x86_64__)
#         define __NR_sched_setaffinity 203
#       elif defined(__ia64__)
#         define __NR_sched_setaffinity 1231
#       elif defined(__hppa__)
#         define __NR_sched_setaffinity 211
#       elif defined(__alpha__)
#         define __NR_sched_setaffinity 395
#       elif defined(__s390__)
#         define __NR_sched_setaffinity 239
#       elif defined(__sparc__)
#         define __NR_sched_setaffinity 261
#       elif defined(__m68k__)
#         define __NR_sched_setaffinity 311
#       elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) || defined(__powerpc64__) || defined(__ppc64__)
#         define __NR_sched_setaffinity 222
#       elif defined(__arm__)
#         define __NR_sched_setaffinity 241
#       elif defined(__cris__)
#         define __NR_sched_setaffinity 241
/*#       elif defined(__mips__)
  #         define __NR_sched_setaffinity TODO (32/64/nabi) */
#       else
#         warning "don't know the syscall number for sched_setaffinity on this architecture, will not support binding"
#         define sched_setaffinity(pid, lg, mask) (errno = ENOSYS, -1)
#       endif
#       ifndef sched_setaffinity
          _syscall3(int, sched_setaffinity, pid_t, pid, unsigned int, lg, unsigned long *, mask);
#       endif
#    endif
#endif

#ifdef HAVE_OPENAT

/* Use our own filesystem functions.  */
#define hwloc_fopen(p, m, d)   hwloc_fopenat(p, m, d)
#define hwloc_access(p, m, d)  hwloc_accessat(p, m, d)
#define hwloc_opendir(p, d)    hwloc_opendirat(p, d)

static FILE *
hwloc_fopenat(const char *path, const char *mode, int fsroot_fd)
{
  int fd;
  const char *relative_path;

  assert(!(fsroot_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  fd = openat (fsroot_fd, relative_path, O_RDONLY);
  if (fd == -1)
    return NULL;

  return fdopen(fd, mode);
}

static int
hwloc_accessat(const char *path, int mode, int fsroot_fd)
{
  const char *relative_path;

  assert(!(fsroot_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  return faccessat(fsroot_fd, relative_path, O_RDONLY, 0);
}

static DIR*
hwloc_opendirat(const char *path, int fsroot_fd)
{
  int dir_fd;
  const char *relative_path;

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  dir_fd = openat(fsroot_fd, relative_path, O_RDONLY | O_DIRECTORY);
  if (dir_fd < 0)
    return NULL;

  return fdopendir(dir_fd);
}

#else /* !HAVE_OPENAT */

#define hwloc_fopen(p, m, d)   fopen(p, m)
#define hwloc_access(p, m, d)  access(p, m)
#define hwloc_opendir(p, d)    opendir(p)

#endif /* !HAVE_OPENAT */

static int
hwloc_linux_set_tid_cpubind(hwloc_topology_t topology, pid_t tid, hwloc_cpuset_t hwloc_set, int strict)
{

  /* TODO Kerrighed: Use
   * int migrate (pid_t pid, int destination_node);
   * int migrate_self (int destination_node);
   * int thread_migrate (int thread_id, int destination_node);
   */

/* TODO: use dynamic size cpusets */
#ifdef HWLOC_HAVE_CPU_SET
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  hwloc_cpuset_foreach_begin(cpu, hwloc_set)
    CPU_SET(cpu, &linux_set);
  hwloc_cpuset_foreach_end();

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return sched_setaffinity(tid, &linux_set);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return sched_setaffinity(tid, sizeof(linux_set), &linux_set);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#else /* CPU_SET */
  unsigned long mask = hwloc_cpuset_to_ulong(hwloc_set);

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return sched_setaffinity(tid, (void*) &mask);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return sched_setaffinity(tid, sizeof(mask), (void*) &mask);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#endif /* CPU_SET */
}

static int
hwloc_linux_set_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_linux_set_tid_cpubind(topology, 0, hwloc_set, strict);
}

static int
hwloc_linux_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_linux_set_tid_cpubind(topology, 0, hwloc_set, strict);
}

#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
#pragma weak pthread_setaffinity_np

static int
hwloc_linux_set_thread_cpubind(hwloc_topology_t topology, pthread_t tid, hwloc_cpuset_t hwloc_set, int strict)
{
  if (!pthread_setaffinity_np) {
    /* ?! Application uses set_thread_cpubind, but doesn't link against libpthread ?! */
    errno = ENOSYS;
    return -1;
  }
  /* TODO Kerrighed: Use
   * int migrate (pid_t pid, int destination_node);
   * int migrate_self (int destination_node);
   * int thread_migrate (int thread_id, int destination_node);
   */

#ifdef HWLOC_HAVE_CPU_SET
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  hwloc_cpuset_foreach_begin(cpu, hwloc_set)
    CPU_SET(cpu, &linux_set);
  hwloc_cpuset_foreach_end();

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return pthread_setaffinity_np(tid, &linux_set);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return pthread_setaffinity_np(tid, sizeof(linux_set), &linux_set);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#else /* CPU_SET */
  unsigned long mask = hwloc_cpuset_to_ulong(hwloc_set);

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return pthread_setaffinity_np(tid, (void*) &mask);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return pthread_setaffinity_np(tid, sizeof(mask), (void*) &mask);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#endif /* CPU_SET */
}
#endif /* HAVE_DECL_PTHREAD_SETAFFINITY_NP */

int
hwloc_backend_sysfs_init(struct hwloc_topology *topology, const char *fsroot_path)
{
#ifdef HAVE_OPENAT
  int root;

  assert(topology->backend_type == HWLOC_BACKEND_NONE);

  if (!fsroot_path)
    fsroot_path = "/";

  root = open(fsroot_path, O_RDONLY | O_DIRECTORY);
  if (root < 0)
    return -1;

  if (strcmp(fsroot_path, "/"))
    topology->is_thissystem = 0;

  topology->backend_params.sysfs.root_fd = root;
#else
  topology->backend_params.sysfs.root_fd = -1;
#endif
  topology->backend_type = HWLOC_BACKEND_SYSFS;
  return 0;
}

void
hwloc_backend_sysfs_exit(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_SYSFS);
#ifdef HAVE_OPENAT
  close(topology->backend_params.sysfs.root_fd);
#endif
  topology->backend_type = HWLOC_BACKEND_NONE;
}

static int
hwloc_parse_sysfs_unsigned(const char *mappath, unsigned *value, int fsroot_fd)
{
  char string[11];
  FILE * fd;

  fd = hwloc_fopen(mappath, "r", fsroot_fd);
  if (!fd) {
    *value = -1;
    return -1;
  }

  if (!fgets(string, 11, fd)) {
    *value = -1;
    return -1;
  }
  *value = strtoul(string, NULL, 10);

  fclose(fd);

  return 0;
}


/* kernel cpumaps are composed of an array of 32bits cpumasks */
#define KERNEL_CPU_MASK_BITS 32
#define KERNEL_CPU_MAP_LEN (KERNEL_CPU_MASK_BITS/4+2)
#define MAX_KERNEL_CPU_MASK ((HWLOC_NBMAXCPUS+KERNEL_CPU_MASK_BITS-1)/KERNEL_CPU_MASK_BITS)

hwloc_cpuset_t
hwloc_linux_parse_cpumap_file(FILE *file)
{
  char string[KERNEL_CPU_MAP_LEN]; /* enough for a shared map mask (32bits hexa) */
  unsigned long maps[MAX_KERNEL_CPU_MASK];
  hwloc_cpuset_t set;
  int nr_maps = 0;

  int i;

  /* allocate and reset to zero first */
  set = hwloc_cpuset_alloc();

  /* parse the whole mask */
  while (fgets(string, KERNEL_CPU_MAP_LEN, file) && *string != '\0') /* read one kernel cpu mask and the ending comma */
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
  assert(!(nr_maps*KERNEL_CPU_MASK_BITS > HWLOC_NBMAXCPUS));

  /* convert into a set */
  for(i=0; i<nr_maps*KERNEL_CPU_MASK_BITS; i++)
    if (maps[i/KERNEL_CPU_MASK_BITS] & 1<<(i%KERNEL_CPU_MASK_BITS))
      hwloc_cpuset_set(set, i);

  return set;
}

static hwloc_cpuset_t
hwloc_parse_cpumap(const char *mappath, int fsroot_fd)
{
  hwloc_cpuset_t set;
  FILE * file;

  file = hwloc_fopen(mappath, "r", fsroot_fd);
  if (!file)
    return NULL;

  set = hwloc_linux_parse_cpumap_file(file);

  fclose(file);
  return set;
}

static int
hwloc_read_cpuset_mask(const char *filename, const char *type, char *info, int infomax, int fsroot_fd)
{
#define CPUSET_NAME_LEN 64
  char cpuset_name[CPUSET_NAME_LEN];
#define CPUSET_FILENAME_LEN 64
  char cpuset_filename[CPUSET_FILENAME_LEN];
  char *tmp;
  FILE *fd;

  /* check whether a cpuset is enabled */
  fd = hwloc_fopen(filename, "r", fsroot_fd);
  if (!fd)
    return 0;

  tmp = fgets(cpuset_name, sizeof(cpuset_name), fd);
  fclose(fd);
  if (!tmp)
    return 0;

  tmp = strchr(cpuset_name, '\n');
  if (tmp)
    *tmp = '\0';

  /* read the cpuset */
  snprintf(cpuset_filename, CPUSET_FILENAME_LEN, "/dev/cpuset%s/%s", cpuset_name, type);
  hwloc_debug("Trying to read cpuset file <%s>\n", cpuset_filename);
  fd = hwloc_fopen(cpuset_filename, "r", fsroot_fd);
  if (!fd) {
    snprintf(cpuset_filename, CPUSET_FILENAME_LEN, "/cpusets%s/%s", cpuset_name, type);
    hwloc_debug("Trying to read cpuset file <%s>\n", cpuset_filename);
    fd = hwloc_fopen(cpuset_filename, "r", fsroot_fd);
  }

  if (!fd)
    return 0;

  tmp = fgets(info, infomax, fd);
  fclose(fd);
  if (!tmp)
    return 0;

  tmp = strchr(info, '\n');
  if (tmp)
    *tmp = '\0';

  return 1;
}

static void
hwloc_admin_disable_set_from_cpuset(struct hwloc_topology *topology,
				   const char *path,
				   const char *name,
				   hwloc_cpuset_t admin_disabled_set)
{
#define CPUSET_MASK_LEN 64
  char cpuset_mask[CPUSET_MASK_LEN];
  char *current, *comma, *tmp;
  int prevlast, nextfirst, nextlast; /* beginning/end of enabled-segments */
  int ret;

  ret = hwloc_read_cpuset_mask(path, name, cpuset_mask, CPUSET_MASK_LEN, topology->backend_params.sysfs.root_fd);
  if (!ret)
    return;

  hwloc_debug("found cpuset %s: %s\n", name, cpuset_mask);

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
      hwloc_debug("%s [%d:%d] excluded by cpuset\n", name, prevlast+1, nextfirst-1);
      hwloc_cpuset_set_range(admin_disabled_set, prevlast+1, nextfirst-1);
    }

    /* switch to next enabled-segment */
    prevlast = nextlast;
    if (!comma)
      break;
    current = comma+1;
  }

  /* disable after last enabled-segment */
  nextfirst = HWLOC_NBMAXCPUS;
  if (prevlast+1 <= nextfirst-1) {
    hwloc_debug("%s [%d:%d] excluded by cpuset\n", name, prevlast+1, nextfirst-1);
    hwloc_cpuset_set_range(admin_disabled_set, prevlast+1, nextfirst-1);
  }
}

static void
hwloc_get_procfs_meminfo_info(struct hwloc_topology *topology,
			     const char *path,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_size_kB,
			     unsigned long *huge_page_free)
{
  char string[64];
  FILE *fd;

  fd = hwloc_fopen(path, "r", topology->backend_params.sysfs.root_fd);
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

#define SYSFS_NUMA_NODE_PATH_LEN 128

static void
hwloc_sysfs_node_meminfo_info(struct hwloc_topology *topology,
			     const char *syspath,
			     int node,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_free)
{
  char path[SYSFS_NUMA_NODE_PATH_LEN];
  char string[64];
  FILE *fd;

  sprintf(path, "%s/node%d/meminfo", syspath, node);
  fd = hwloc_fopen(path, "r", topology->backend_params.sysfs.root_fd);
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
hwloc_parse_node_distance(const char *distancepath, unsigned nbnodes, unsigned *distances, int fsroot_fd)
{
  char string[4096]; /* enough for hundreds of nodes */
  char *tmp, *next;
  FILE * fd;

  fd = hwloc_fopen(distancepath, "r", fsroot_fd);
  if (!fd)
    return;

  if (!fgets(string, sizeof(string), fd))
    return;

  tmp = string;
  while (tmp) {
    unsigned distance = strtoul(tmp, &next, 0);
    if (next == tmp)
      break;
    *distances = distance;
    distances++;
    nbnodes--;
    if (!nbnodes)
      break;
    tmp = next+1;
  }

  fclose(fd);
}

static void
look_sysfsnode(struct hwloc_topology *topology,
	       const char *path,
	       hwloc_cpuset_t admin_disabled_cpus_set,
	       hwloc_cpuset_t admin_disabled_mems_set)
{
  unsigned osnode;
  unsigned nbnodes = 1;
  DIR *dir;
  struct dirent *dirent;
  hwloc_obj_t node;

  dir = hwloc_opendir(path, topology->backend_params.sysfs.root_fd);
  if (dir)
    {
      while ((dirent = readdir(dir)) != NULL)
	{
	  unsigned long numnode;
	  if (strncmp(dirent->d_name, "node", 4))
	    continue;
	  numnode = strtoul(dirent->d_name+4, NULL, 0);
	  if (nbnodes < numnode+1)
	    nbnodes = numnode+1;
	}
      closedir(dir);
    }

  if (nbnodes <= 1)
    return;

  /* For convenience, put these declarations inside a block.  Saves us
     from a bunch of mallocs, particularly with the 2D array. */
  {
      hwloc_obj_t nodes[nbnodes];
      unsigned distances[nbnodes][nbnodes];
      for (osnode=0; osnode < nbnodes; osnode++) {
          char nodepath[SYSFS_NUMA_NODE_PATH_LEN];
          hwloc_cpuset_t cpuset;
          unsigned long size = -1;
          unsigned long hpfree = -1;
          
          sprintf(nodepath, "%s/node%u/cpumap", path, osnode);
          cpuset = hwloc_parse_cpumap(nodepath, topology->backend_params.sysfs.root_fd);
          if (!cpuset)
              continue;
          
          /* clear disabled cpus */
          hwloc_cpuset_clearset(cpuset, admin_disabled_cpus_set);
          
          if (hwloc_cpuset_isset(admin_disabled_mems_set, osnode)) {
              size = 0; hpfree = 0;
          } else
              hwloc_sysfs_node_meminfo_info(topology, path, osnode, &size, &hpfree);
          
          node = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, osnode);
          node->cpuset = cpuset;
          node->attr->node.memory_kB = size;
          node->attr->node.huge_page_free = hpfree;
          node->cpuset = cpuset;
          
          hwloc_debug_1arg_cpuset("os node %u has cpuset %s\n",
                                  osnode, node->cpuset);
          hwloc_add_object(topology, node);
          nodes[osnode] = node;
          
          sprintf(nodepath, "%s/node%u/distance", path, osnode);
          hwloc_parse_node_distance(nodepath, nbnodes, distances[osnode], topology->backend_params.sysfs.root_fd);
      }
      
      hwloc_setup_misc_level_from_distances(topology, nbnodes, nodes, (unsigned*) distances);
  }
}

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void
look_sysfscpu(struct hwloc_topology *topology, const char *path,
	      hwloc_cpuset_t admin_disabled_cpus_set)
{
  hwloc_cpuset_t cpuset;
#define CPU_TOPOLOGY_STR_LEN 128
  char str[CPU_TOPOLOGY_STR_LEN];
  DIR *dir;
  int i,j;
  FILE *fd;

  cpuset = hwloc_cpuset_alloc();

  /* fill the cpuset of interesting cpus */
  dir = hwloc_opendir(path, topology->backend_params.sysfs.root_fd);
  if (dir) {
    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL) {
      unsigned long cpu;
      char online[2];

      if (strncmp(dirent->d_name, "cpu", 3))
	continue;
      cpu = strtoul(dirent->d_name+3, NULL, 0);

      assert(cpu < HWLOC_NBMAXCPUS);

      /* check whether cpusets exclude this cpu */
      if (hwloc_cpuset_isset(admin_disabled_cpus_set, cpu)) {
	hwloc_debug("os proc %lu is disabled by the administrator\n", cpu);
	continue;
      }

      /* check whether the kernel exports topology information for this cpu */
      sprintf(str, "%s/cpu%lu/topology", path, cpu);
      if (hwloc_access(str, X_OK, topology->backend_params.sysfs.root_fd) < 0 && errno == ENOENT) {
	hwloc_debug("os proc %lu has no accessible %s/cpu%lu/topology\n",
		   cpu, path, cpu);
	continue;
      }

      /* check whether this processor is offline */
      sprintf(str, "%s/cpu%lu/online", path, cpu);
      fd = hwloc_fopen(str, "r", topology->backend_params.sysfs.root_fd);
      if (fd) {
	if (fgets(online, sizeof(online), fd)) {
	  fclose(fd);
	  if (atoi(online)) {
	    hwloc_debug("os proc %lu is online\n", cpu);
	  } else {
	    hwloc_debug("os proc %lu is offline\n", cpu);
	    continue;
	  }
	} else {
	  fclose(fd);
	}
      }

      hwloc_cpuset_set(cpuset, cpu);
    }
    closedir(dir);
  }

  hwloc_debug_1arg_cpuset("found %d cpus, cpuset %s\n",
	     hwloc_cpuset_weight(cpuset), cpuset);

  hwloc_cpuset_foreach_begin(i, cpuset)
    {
      struct hwloc_obj *socket, *core, *thread;
      hwloc_cpuset_t socketset, coreset, threadset;
      unsigned mysocketid, mycoreid;

      /* look at the socket */
      mysocketid = 0; /* shut-up the compiler */
      sprintf(str, "%s/cpu%d/topology/physical_package_id", path, i);
      hwloc_parse_sysfs_unsigned(str, &mysocketid, topology->backend_params.sysfs.root_fd);

      sprintf(str, "%s/cpu%d/topology/core_siblings", path, i);
      socketset = hwloc_parse_cpumap(str, topology->backend_params.sysfs.root_fd);
      assert(socketset);
      hwloc_cpuset_clearset(socketset, admin_disabled_cpus_set);
      assert(hwloc_cpuset_weight(socketset) >= 1);

      if (hwloc_cpuset_first(socketset) == i) {
	/* first cpu in this socket, add the socket */
	socket = hwloc_alloc_setup_object(HWLOC_OBJ_SOCKET, mysocketid);
	socket->cpuset = socketset;
        hwloc_debug_1arg_cpuset("os socket %u has cpuset %s\n",
		   mysocketid, socketset);
	hwloc_add_object(topology, socket);
      } else
	free(socketset);

      /* look at the core */
      mycoreid = 0; /* shut-up the compiler */
      sprintf(str, "%s/cpu%d/topology/core_id", path, i);
      hwloc_parse_sysfs_unsigned(str, &mycoreid, topology->backend_params.sysfs.root_fd);

      sprintf(str, "%s/cpu%d/topology/thread_siblings", path, i);
      coreset = hwloc_parse_cpumap(str, topology->backend_params.sysfs.root_fd);
      assert(coreset);
      hwloc_cpuset_clearset(coreset, admin_disabled_cpus_set);
      assert(hwloc_cpuset_weight(coreset) >= 1);

      if (hwloc_cpuset_first(coreset) == i) {
	core = hwloc_alloc_setup_object(HWLOC_OBJ_CORE, mycoreid);
	core->cpuset = coreset;
        hwloc_debug_1arg_cpuset("os core %u has cpuset %s\n",
		   mycoreid, coreset);
	hwloc_add_object(topology, core);
      } else
	free(coreset);

      /* look at the thread */
      threadset = hwloc_cpuset_alloc();
      hwloc_cpuset_cpu(threadset, i);
      hwloc_cpuset_clearset(threadset, admin_disabled_cpus_set);
      assert(hwloc_cpuset_weight(threadset) == 1);

      /* add the thread */
      thread = hwloc_alloc_setup_object(HWLOC_OBJ_PROC, i);
      thread->cpuset = threadset;
      hwloc_debug_1arg_cpuset("thread %d has cpuset %s\n",
		 i, threadset);
      hwloc_add_object(topology, thread);

      /* look at the caches */
      for(j=0; j<10; j++) {
#define SHARED_CPU_MAP_STRLEN 128
	char mappath[SHARED_CPU_MAP_STRLEN];
	char str2[20]; /* enough for a level number (one digit) or a type (Data/Instruction/Unified) */
	struct hwloc_obj *cache;
	hwloc_cpuset_t cacheset;
	unsigned long kB = 0;
	int depth; /* 0 for L1, .... */

	/* get the cache level depth */
	sprintf(mappath, "%s/cpu%d/cache/index%d/level", path, i, j);
	fd = hwloc_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(str2,sizeof(str2), fd))
	    depth = strtoul(str2, NULL, 10)-1;
	  else
	    continue;
	  fclose(fd);
	} else
	  continue;

	/* ignore Instruction caches */
	sprintf(mappath, "%s/cpu%d/cache/index%d/type", path, i, j);
	fd = hwloc_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(str2, sizeof(str2), fd)) {
	    fclose(fd);
	    if (!strncmp(str2, "Instruction", 11))
	      continue;
	  } else {
	    fclose(fd);
	    continue;
	  }
	} else
	  continue;

	/* get the cache size */
	sprintf(mappath, "%s/cpu%d/cache/index%d/size", path, i, j);
	fd = hwloc_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(str2,sizeof(str2), fd))
	    kB = atol(str2); /* in kB */
	  fclose(fd);
	}

	sprintf(mappath, "%s/cpu%d/cache/index%d/shared_cpu_map", path, i, j);
	cacheset = hwloc_parse_cpumap(mappath, topology->backend_params.sysfs.root_fd);
	assert(cacheset);
	hwloc_cpuset_clearset(cacheset, admin_disabled_cpus_set);
	if (hwloc_cpuset_weight(cacheset) < 1)
	  /* mask is wrong (happens on ia64), assumes it's not shared */
	  hwloc_cpuset_cpu(cacheset, i);

	if (hwloc_cpuset_first(cacheset) == i) {
	  /* first cpu in this cache, add the cache */
	  cache = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, -1);
	  cache->attr->cache.memory_kB = kB;
	  cache->attr->cache.depth = depth+1;
	  cache->cpuset = cacheset;
          hwloc_debug_1arg_cpuset("cache depth %d has cpuset %s\n",
		     depth, cacheset);
	  hwloc_add_object(topology, cache);
	} else
	  free(cacheset);
      }
    }
  hwloc_cpuset_foreach_end();

  hwloc_cpuset_free(cpuset);
}


/* Look at Linux' /proc/cpuinfo */
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"
static int
look_cpuinfo(struct hwloc_topology *topology, const char *path,
	     hwloc_cpuset_t online_cpuset,
	     hwloc_cpuset_t admin_disabled_cpus_set)
{
  FILE *fd;
  char str[strlen(PHYSID)+1+9+1+1];
  char *endptr;
  unsigned proc_physids[HWLOC_NBMAXCPUS];
  unsigned osphysids[HWLOC_NBMAXCPUS];
  unsigned proc_coreids[HWLOC_NBMAXCPUS];
  unsigned oscoreids[HWLOC_NBMAXCPUS];
  unsigned proc_osphysids[HWLOC_NBMAXCPUS];
  unsigned core_osphysids[HWLOC_NBMAXCPUS];
  unsigned procid_max=0;
  unsigned numprocs=0;
  unsigned numsockets=0;
  unsigned numcores=0;
  long physid;
  long coreid;
  long processor = -1;
  int i;

  for (i = 0; i < HWLOC_NBMAXCPUS; i++) {
    proc_physids[i] = -1;
    osphysids[i] = -1;
    proc_coreids[i] = -1;
    oscoreids[i] = -1;
    proc_osphysids[i] = -1;
    core_osphysids[i] = -1;
  }

  hwloc_cpuset_zero(online_cpuset);

  if (!(fd=hwloc_fopen(path,"r", topology->backend_params.sysfs.root_fd)))
    {
      hwloc_debug("could not open /proc/cpuinfo\n");
      return -1;
    }

  /* Just record information and count number of sockets and cores */

  hwloc_debug("\n\n * Topology extraction from /proc/cpuinfo *\n\n");
  while (fgets(str,sizeof(str),fd)!=NULL)
    {
#      define getprocnb_begin(field, var)		     \
      if ( !strncmp(field,str,strlen(field)))	     \
	{						     \
	char *c = strchr(str, ':')+1;		     \
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
	hwloc_debug(field " %ld\n", var)
#      define getprocnb_end()			\
      }
      getprocnb_begin(PROCESSOR,processor);
      hwloc_cpuset_set(online_cpuset, processor);
      getprocnb_end() else
      getprocnb_begin(PHYSID,physid);
      proc_osphysids[processor]=physid;
      for (i=0; i<numsockets; i++)
	if (physid == osphysids[i])
	  break;
      proc_physids[processor]=i;
      hwloc_debug("%ld on socket %d (%lx)\n", processor, i, physid);
      if (i==numsockets)
	osphysids[(numsockets)++] = physid;
      getprocnb_end() else
      getprocnb_begin(COREID,coreid);
      for (i=0; i<numcores; i++)
	if (coreid == oscoreids[i] && proc_osphysids[processor] == core_osphysids[i])
	  break;
      proc_coreids[processor]=i;
      hwloc_debug("%ld on core %d (%lx:%x)\n", processor, i, coreid, proc_osphysids[processor]);
      if (i==numcores)
	{
	  core_osphysids[numcores] = proc_osphysids[processor];
	  oscoreids[numcores] = coreid;
	  (numcores)++;
	}
      getprocnb_end()
	if (str[strlen(str)-1]!='\n')
	  {
            /* ignore end of line */
	    if (fscanf(fd,"%*[^\n]") == EOF)
	      break;
	    getc(fd);
	  }
    }
  fclose(fd);

  /* setup the final number of procs */
  procid_max = processor + 1;
  numprocs = hwloc_cpuset_weight(online_cpuset);

  /* clear admin-disabled cpus */
  hwloc_cpuset_foreach_begin(i, online_cpuset) {
    if (hwloc_cpuset_isset(admin_disabled_cpus_set, i)) {
      hwloc_cpuset_clr(online_cpuset, i);
      proc_osphysids[i] = -1;
      proc_physids[i] = -1;
      proc_coreids[i] = -1;
    }
  } hwloc_cpuset_foreach_end();

  hwloc_debug("%u online processors found, with id max %u\n", numprocs, procid_max);
  hwloc_debug_cpuset("online processor cpuset: %s\n",
	     online_cpuset);

  hwloc_debug("\n * Topology summary *\n");
  hwloc_debug("%d processors (%d max id)\n", numprocs, procid_max);

  hwloc_debug("%d sockets\n", numsockets);
  if (numsockets>0)
    hwloc_setup_level(procid_max, numsockets, osphysids, proc_physids, topology, HWLOC_OBJ_SOCKET);

  hwloc_debug("%d cores\n", numcores);
  if (numcores>0)
    hwloc_setup_level(procid_max, numcores, oscoreids, proc_coreids, topology, HWLOC_OBJ_CORE);

  /* setup the cpu level, removing nonfirst threads */
  hwloc_setup_proc_level(topology, numprocs, online_cpuset);

  return 0;
}

static void
hwloc__get_dmi_info(struct hwloc_topology *topology,
		   char **dmi_board_vendor, char **dmi_board_name)
{
#define DMI_BOARD_STRINGS_LEN 50
  char dmi_line[DMI_BOARD_STRINGS_LEN];
  char *tmp;
  FILE *fd;

  dmi_line[0] = '\0';
  fd = hwloc_fopen("/sys/class/dmi/id/board_vendor", "r", topology->backend_params.sysfs.root_fd);
  if (fd) {
    tmp = fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (tmp && dmi_line[0] != '\0') {
      tmp = strchr(dmi_line, '\n');
      if (tmp)
	*tmp = '\0';
      *dmi_board_vendor = strdup(dmi_line);
      hwloc_debug("found DMI board vendor '%s'\n", *dmi_board_vendor);
    }
  }

  dmi_line[0] = '\0';
  fd = hwloc_fopen("/sys/class/dmi/id/board_name", "r", topology->backend_params.sysfs.root_fd);
  if (fd) {
    tmp = fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (tmp && dmi_line[0] != '\0') {
      tmp = strchr(dmi_line, '\n');
      if (tmp)
	*tmp = '\0';
      *dmi_board_name = strdup(dmi_line);
      hwloc_debug("found DMI board name '%s'\n", *dmi_board_name);
    }
  }
}

void
hwloc_look_linux(struct hwloc_topology *topology)
{
  hwloc_cpuset_t admin_disabled_cpus_set, admin_disabled_mems_set, online_set;
  DIR *nodes_dir;
  int err;

  admin_disabled_cpus_set = hwloc_cpuset_alloc();
  admin_disabled_mems_set = hwloc_cpuset_alloc();
  online_set = hwloc_cpuset_alloc();

  nodes_dir = hwloc_opendir("/proc/nodes", topology->backend_params.sysfs.root_fd);
  if (nodes_dir) {
    /* Kerrighed */
    struct dirent *dirent;
    char path[128];
    hwloc_obj_t machine;

    topology->levels[0][0]->attr->system.memory_kB = 0;
    topology->levels[0][0]->attr->system.huge_page_free = 0;
    /* No cpuset support for now.  */
    /* No sys support for now.  */
    while ((dirent = readdir(nodes_dir)) != NULL) {
      unsigned long node;
      if (strncmp(dirent->d_name, "node", 4))
	continue;
      node = strtoul(dirent->d_name+4, NULL, 0);
      snprintf(path, sizeof(path), "/proc/nodes/node%lu/cpuinfo", node);
      err = look_cpuinfo(topology, path, online_set, admin_disabled_cpus_set);
      if (err < 0) {
	fprintf(stderr, "/proc/cpuinfo missing, required for kerrighed support\n");
	abort();
      }
      machine = hwloc_alloc_setup_object(HWLOC_OBJ_MACHINE, node);
      machine->cpuset = hwloc_cpuset_dup(online_set);
      machine->attr->machine.dmi_board_name = NULL;
      machine->attr->machine.dmi_board_vendor = NULL;
      hwloc_debug_1arg_cpuset("machine number %lu has cpuset %s\n",
		 node, online_set);
      hwloc_add_object(topology, machine);

      snprintf(path, sizeof(path), "/proc/nodes/node%lu/meminfo", node);
      /* Compute the machine memory and huge page */
      hwloc_get_procfs_meminfo_info(topology,
				   path,
				   &machine->attr->machine.memory_kB,
				   &machine->attr->machine.huge_page_size_kB,
				   &machine->attr->machine.huge_page_free);
				   /* FIXME: gather page_size_kB as well? MaMI needs it */
      topology->levels[0][0]->attr->system.memory_kB += machine->attr->machine.memory_kB;
      topology->levels[0][0]->attr->system.huge_page_free += machine->attr->machine.huge_page_free;

      /* Gather DMI info */
      /* FIXME: get the right DMI info of each machine */
      hwloc__get_dmi_info(topology,
			 &machine->attr->machine.dmi_board_vendor,
			 &machine->attr->machine.dmi_board_name);
    }
    closedir(nodes_dir);
  } else {
    /* Gather the list of admin-disabled cpus and mems */
    if (!(topology->flags & HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM)) {
      hwloc_admin_disable_set_from_cpuset(topology, "/proc/self/cpuset", "cpus", admin_disabled_cpus_set);
      hwloc_admin_disable_set_from_cpuset(topology, "/proc/self/cpuset", "mems", admin_disabled_mems_set);
    }

    /* Gather NUMA information */
    look_sysfsnode(topology, "/sys/devices/system/node", admin_disabled_cpus_set, admin_disabled_mems_set);

    /* Gather the list of cpus now */
    if (getenv("HWLOC_LINUX_USE_CPUINFO")
	|| hwloc_access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| hwloc_access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| hwloc_access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| hwloc_access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0) {
	/* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
      err = look_cpuinfo(topology, "/proc/cpuinfo", online_set, admin_disabled_cpus_set);
      if (err < 0) {
	fprintf(stderr, "sysfs topology not found, and /proc/cpuinfo missing\n");
	abort();
      }
    } else {
      look_sysfscpu(topology, "/sys/devices/system/cpu", admin_disabled_cpus_set);
    }

    /* Compute the whole system memory and huge page */
    hwloc_get_procfs_meminfo_info(topology,
				 "/proc/meminfo",
				 &topology->levels[0][0]->attr->system.memory_kB,
				 &topology->levels[0][0]->attr->system.huge_page_size_kB,
				 &topology->levels[0][0]->attr->system.huge_page_free);
				 /* FIXME: gather page_size_kB as well? MaMI needs it */

    /* Gather DMI info */
    hwloc__get_dmi_info(topology,
		       &topology->levels[0][0]->attr->system.dmi_board_vendor,
		       &topology->levels[0][0]->attr->system.dmi_board_name);
  }

  hwloc_cpuset_free(admin_disabled_cpus_set);
  hwloc_cpuset_free(admin_disabled_mems_set);
  hwloc_cpuset_free(online_set);
}

void
hwloc_set_linux_hooks(struct hwloc_topology *topology)
{
  topology->set_cpubind = hwloc_linux_set_cpubind;
#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
  topology->set_thread_cpubind = hwloc_linux_set_thread_cpubind;
#endif /* HAVE_DECL_PTHREAD_SETAFFINITY_NP */
  topology->set_thisthread_cpubind = hwloc_linux_set_thisthread_cpubind;
}

/* TODO mbind, setpolicy */
