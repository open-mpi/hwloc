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
#include <topology/private.h>
#include <topology/debug.h>

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

#  ifndef CPU_SET
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
#       else
#         error "don't know the syscall number for sched_setaffinity on this architecture"
#       endif
_syscall3(int, sched_setaffinity, pid_t, pid, unsigned int, lg, unsigned long *, mask);
#    endif
#  endif

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

static int
topo_linux_set_tid_cpubind(topo_topology_t topology, pid_t tid, const topo_cpuset_t *topo_set, int strict)
{

  /* TODO Kerrighed: Use
   * int migrate (pid_t pid, int destination_node);
   * int migrate_self (int destination_node);
   * int thread_migrate (int thread_id, int destination_node);
   */

#ifdef CPU_SET
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  topo_cpuset_foreach_begin(cpu, topo_set)
    CPU_SET(cpu, &linux_set);
  topo_cpuset_foreach_end();

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return sched_setaffinity(tid, &linux_set);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return sched_setaffinity(tid, sizeof(linux_set), &linux_set);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#else /* CPU_SET */
  unsigned long mask = topo_cpuset_to_ulong(topo_set);

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return sched_setaffinity(tid, &mask);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return sched_setaffinity(tid, sizeof(mask), &mask);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#endif /* CPU_SET */
}

static int
topo_linux_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_linux_set_tid_cpubind(topology, 0, topo_set, strict);
}

static int
topo_linux_set_thisthread_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_linux_set_tid_cpubind(topology, 0, topo_set, strict);
}

static int
topo_linux_set_proc_cpubind(topo_topology_t topology, pid_t pid, const topo_cpuset_t *topo_set, int strict)
{
  /* XXX: doesn't bind all threads of the processus !! */
  /* Same problem for thisproc */
  return topo_linux_set_tid_cpubind(topology, pid, topo_set, strict);
}

#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
#pragma weak pthread_setaffinity_np

static int
topo_linux_set_thread_cpubind(topo_topology_t topology, pthread_t tid, const topo_cpuset_t *topo_set, int strict)
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

#ifdef CPU_SET
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  topo_cpuset_foreach_begin(cpu, topo_set)
    CPU_SET(cpu, &linux_set);
  topo_cpuset_foreach_end();

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return pthread_setaffinity_np(tid, &linux_set);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return pthread_setaffinity_np(tid, sizeof(linux_set), &linux_set);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#else /* CPU_SET */
  unsigned long mask = topo_cpuset_to_ulong(topo_set);

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return pthread_setaffinity_np(tid, &mask);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return pthread_setaffinity_np(tid, sizeof(mask), &mask);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#endif /* CPU_SET */
}
#endif /* HAVE_DECL_PTHREAD_SETAFFINITY_NP */

static int
topo_linux_fsys_root_set_cpubind(void) {
  return 0;
}

int
topo_backend_sysfs_init(struct topo_topology *topology, const char *fsys_root_path)
{
#ifdef HAVE_OPENAT
  int root;

  assert(topology->backend_type == TOPO_BACKEND_NONE);

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
topo_read_cpuset_mask(const char *filename, const char *type, char *info, int infomax, int fsys_root_fd)
{
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
				   const char *path,
				   const char *name,
				   topo_cpuset_t *admin_disabled_set)
{
#define CPUSET_MASK_LEN 64
  char cpuset_mask[CPUSET_MASK_LEN];
  char *current, *comma, *tmp;
  int prevlast, nextfirst, nextlast; /* beginning/end of enabled-segments */
  int ret;

  ret = topo_read_cpuset_mask(path, name, cpuset_mask, CPUSET_MASK_LEN, topology->backend_params.sysfs.root_fd);
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
			     const char *path,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_size_kB,
			     unsigned long *huge_page_free)
{
  char string[64];
  FILE *fd;

  fd = topo_fopen(path, "r", topology->backend_params.sysfs.root_fd);
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
topo_sysfs_node_meminfo_info(struct topo_topology *topology,
			     const char *syspath,
			     int node,
			     unsigned long *mem_size_kB,
			     unsigned long *huge_page_free)
{
  char path[SYSFS_NUMA_NODE_PATH_LEN];
  char string[64];
  FILE *fd;

  sprintf(path, "%s/node%d/meminfo", syspath, node);
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
	       const char *path,
	       topo_cpuset_t *admin_disabled_mems_set)
{
  unsigned osnode;
  unsigned nbnodes = 1;
  DIR *dir;
  struct dirent *dirent;
  topo_obj_t node;

  dir = topo_opendir(path, topology->backend_params.sysfs.root_fd);
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

  for (osnode=0; osnode < nbnodes; osnode++)
    {
      char nodepath[SYSFS_NUMA_NODE_PATH_LEN];
      topo_cpuset_t cpuset;
      unsigned long size = -1;
      unsigned long hpfree = -1;

      sprintf(nodepath, "%s/node%u/cpumap", path, osnode);
      if (topo_parse_cpumap(nodepath, &cpuset, topology->backend_params.sysfs.root_fd) < 0)
	continue;

      if (topo_cpuset_isset(admin_disabled_mems_set, osnode)) {
	size = 0; hpfree = 0;
      } else
	topo_sysfs_node_meminfo_info(topology, path, osnode, &size, &hpfree);

      node = topo_alloc_setup_object(TOPO_OBJ_NODE, osnode);
      node->attr->node.memory_kB = size;
      node->attr->node.huge_page_free = hpfree;
      node->cpuset = cpuset;

      topo_debug("os node %u has cpuset %"TOPO_PRIxCPUSET"\n",
		 osnode, TOPO_CPUSET_PRINTF_VALUE(&node->cpuset));
      topo_add_object(topology, node);
    }
}

/* Look at Linux' /sys/devices/system/cpu/cpu%d/topology/ */
static void
look_sysfscpu(struct topo_topology *topology, const char *path,
	      topo_cpuset_t *admin_disabled_cpus_set)
{
  topo_cpuset_t cpuset;
#define CPU_TOPOLOGY_STR_LEN 128
  char string[CPU_TOPOLOGY_STR_LEN];
  DIR *dir;
  int i,j;
  FILE *fd;

  /* fill the cpuset of interesting cpus */
  topo_cpuset_zero(&cpuset);
  dir = topo_opendir(path, topology->backend_params.sysfs.root_fd);
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
	topo_debug("os proc %lu is disabled by the administrator\n", cpu);
	continue;
      }

      /* check whether the kernel exports topology information for this cpu */
      sprintf(string, "%s/cpu%lu/topology", path, cpu);
      if (topo_access(string, X_OK, topology->backend_params.sysfs.root_fd) < 0 && errno == ENOENT) {
	topo_debug("os proc %lu has no accessible %s/cpu%lu/topology\n",
		   cpu, path, cpu);
	continue;
      }

      /* check whether this processor is offline */
      sprintf(string, "%s/cpu%lu/online", path, cpu);
      fd = topo_fopen(string, "r", topology->backend_params.sysfs.root_fd);
      if (fd) {
	if (fgets(online, sizeof(online), fd)) {
	  fclose(fd);
	  if (atoi(online)) {
	    topo_debug("os proc %lu is online\n", cpu);
	  } else {
	    topo_debug("os proc %lu is offline\n", cpu);
	    continue;
	  }
	} else {
	  fclose(fd);
	}
      }

      topo_cpuset_set(&cpuset, cpu);
    }
    closedir(dir);
  }

  topo_debug("found %d cpus, cpuset %" TOPO_PRIxCPUSET "\n",
	     topo_cpuset_weight(&cpuset), TOPO_CPUSET_PRINTF_VALUE(&cpuset));

  topo_cpuset_foreach_begin(i, &cpuset)
    {
      struct topo_obj *socket, *core, *thread;
      topo_cpuset_t socketset, coreset, threadset;
      unsigned mysocketid, mycoreid;

      /* look at the socket */
      mysocketid = 0; /* shut-up the compiler */
      sprintf(string, "%s/cpu%d/topology/physical_package_id", path, i);
      topo_parse_sysfs_unsigned(string, &mysocketid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "%s/cpu%d/topology/core_siblings", path, i);
      topo_parse_cpumap(string, &socketset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&socketset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&socketset) >= 1);

      if (topo_cpuset_first(&socketset) == i) {
	/* first cpu in this socket, add the socket */
	socket = topo_alloc_setup_object(TOPO_OBJ_SOCKET, mysocketid);
	socket->cpuset = socketset;
	topo_debug("os socket %u has cpuset %"TOPO_PRIxCPUSET"\n",
		   mysocketid, TOPO_CPUSET_PRINTF_VALUE(&socketset));
	topo_add_object(topology, socket);
      }

      /* look at the core */
      mycoreid = 0; /* shut-up the compiler */
      sprintf(string, "%s/cpu%d/topology/core_id", path, i);
      topo_parse_sysfs_unsigned(string, &mycoreid, topology->backend_params.sysfs.root_fd);

      sprintf(string, "%s/cpu%d/topology/thread_siblings", path, i);
      topo_parse_cpumap(string, &coreset, topology->backend_params.sysfs.root_fd);
      topo_cpuset_clearset(&coreset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&coreset) >= 1);

      if (topo_cpuset_first(&coreset) == i) {
	core = topo_alloc_setup_object(TOPO_OBJ_CORE, mycoreid);
	core->cpuset = coreset;
	topo_debug("os core %u has cpuset %"TOPO_PRIxCPUSET"\n",
		   mycoreid, TOPO_CPUSET_PRINTF_VALUE(&coreset));
	topo_add_object(topology, core);
      }

      /* look at the thread */
      topo_cpuset_cpu(&threadset, i);
      topo_cpuset_clearset(&threadset, admin_disabled_cpus_set);
      assert(topo_cpuset_weight(&threadset) == 1);

      /* add the thread */
      thread = topo_alloc_setup_object(TOPO_OBJ_PROC, i);
      thread->cpuset = threadset;
      topo_debug("thread %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 i, TOPO_CPUSET_PRINTF_VALUE(&threadset));
      topo_add_object(topology, thread);

      /* look at the caches */
      for(j=0; j<10; j++) {
#define SHARED_CPU_MAP_STRLEN 128
	char mappath[SHARED_CPU_MAP_STRLEN];
	char string[20]; /* enough for a level number (one digit) or a type (Data/Instruction/Unified) */
	struct topo_obj *cache;
	topo_cpuset_t cacheset;
	unsigned long kB = 0;
	int depth; /* 0 for L1, .... */
	FILE * fd;

	/* get the cache level depth */
	sprintf(mappath, "%s/cpu%d/cache/index%d/level", path, i, j);
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
	sprintf(mappath, "%s/cpu%d/cache/index%d/type", path, i, j);
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
	sprintf(mappath, "%s/cpu%d/cache/index%d/size", path, i, j);
	fd = topo_fopen(mappath, "r", topology->backend_params.sysfs.root_fd);
	if (fd) {
	  if (fgets(string,sizeof(string), fd))
	    kB = atol(string); /* in kB */
	  fclose(fd);
	}

	sprintf(mappath, "%s/cpu%d/cache/index%d/shared_cpu_map", path, i, j);
	topo_parse_cpumap(mappath, &cacheset, topology->backend_params.sysfs.root_fd);
	topo_cpuset_clearset(&cacheset, admin_disabled_cpus_set);
	if (topo_cpuset_weight(&cacheset) < 1)
	  /* mask is wrong (happens on ia64), assumes it's not shared */
	  topo_cpuset_cpu(&cacheset, i);

	if (topo_cpuset_first(&cacheset) == i) {
	  /* first cpu in this cache, add the cache */
	  cache = topo_alloc_setup_object(TOPO_OBJ_CACHE, -1);
	  cache->attr->cache.memory_kB = kB;
	  cache->attr->cache.depth = depth+1;
	  cache->cpuset = cacheset;
	  topo_debug("cache depth %d has cpuset %"TOPO_PRIxCPUSET"\n",
		     depth, TOPO_CPUSET_PRINTF_VALUE(&cacheset));
	  topo_add_object(topology, cache);
	}
      }
    }
  topo_cpuset_foreach_end();
}


/* Look at Linux' /proc/cpuinfo */
#      define PROCESSOR	"processor"
#      define PHYSID "physical id"
#      define COREID "core id"
static int
look_cpuinfo(struct topo_topology *topology, const char *path,
	     topo_cpuset_t *online_cpuset,
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
  unsigned procid_max=0;
  unsigned numprocs=0;
  unsigned numsockets=0;
  unsigned numcores=0;
  long physid;
  long coreid;
  long processor = -1;
  int i;

  topo_cpuset_zero(online_cpuset);

  if (!(fd=topo_fopen(path,"r", topology->backend_params.sysfs.root_fd)))
    {
      topo_debug("could not open /proc/cpuinfo\n");
      return -1;
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
      topo_cpuset_set(online_cpuset, processor);
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
  numprocs = topo_cpuset_weight(online_cpuset);

  /* clear admin-disabled cpus */
  topo_cpuset_foreach_begin(i, online_cpuset) {
    if (topo_cpuset_isset(admin_disabled_cpus_set, i)) {
      topo_cpuset_clr(online_cpuset, i);
      proc_osphysids[i] = -1;
      proc_physids[i] = -1;
      proc_oscoreids[i] = -1;
      proc_coreids[i] = -1;
    }
  } topo_cpuset_foreach_end();

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
  topo_setup_proc_level(topology, numprocs, online_cpuset);

  return 0;
}

static void
topo__get_dmi_info(struct topo_topology *topology,
		   char **dmi_board_vendor, char **dmi_board_name)
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
      *dmi_board_vendor = strdup(dmi_line);
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
      *dmi_board_name = strdup(dmi_line);
      topo_debug("found DMI board name '%s'\n", topology->dmi_board_name);
    }
  }
}

void
topo_look_linux(struct topo_topology *topology)
{
  topo_cpuset_t admin_disabled_cpus_set, admin_disabled_mems_set, online_set;
  DIR *nodes_dir;
  int err;

  topo_cpuset_zero(&admin_disabled_cpus_set);
  topo_cpuset_zero(&admin_disabled_mems_set);

  if (!topology->is_fake) {
    topology->set_cpubind = topo_linux_set_cpubind;
#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
    topology->set_thread_cpubind = topo_linux_set_thread_cpubind;
#endif /* HAVE_DECL_PTHREAD_SETAFFINITY_NP */
    topology->set_thisthread_cpubind = topo_linux_set_thisthread_cpubind;
  } else {
    topology->set_cpubind = (void*) topo_linux_fsys_root_set_cpubind;
    topology->set_thread_cpubind = (void*) topo_linux_fsys_root_set_cpubind;
    topology->set_thisthread_cpubind = (void*) topo_linux_fsys_root_set_cpubind;
  }

  nodes_dir = topo_opendir("/proc/nodes", topology->backend_params.sysfs.root_fd);
  if (nodes_dir) {
    /* Kerrighed */
    struct dirent *dirent;
    char path[128];
    topo_obj_t machine;

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
      err = look_cpuinfo(topology, path, &online_set, &admin_disabled_cpus_set);
      if (err < 0) {
	fprintf(stderr, "/proc/cpuinfo missing, required for kerrighed support\n");
	abort();
      }
      machine = topo_alloc_setup_object(TOPO_OBJ_MACHINE, node);
      machine->cpuset = online_set;
      machine->attr->machine.dmi_board_name = NULL;
      machine->attr->machine.dmi_board_vendor = NULL;
      topo_debug("machine number %lu has cpuset %"TOPO_PRIxCPUSET"\n",
		 node, TOPO_CPUSET_PRINTF_VALUE(&online_set));
      topo_add_object(topology, machine);

      snprintf(path, sizeof(path), "/proc/nodes/node%lu/meminfo", node);
      /* Compute the machine memory and huge page */
      topo_get_procfs_meminfo_info(topology,
				   path,
				   &machine->attr->machine.memory_kB,
				   &machine->attr->machine.huge_page_size_kB,
				   &machine->attr->machine.huge_page_free);
				   /* FIXME: gather page_size_kB as well? MaMI needs it */
      topology->levels[0][0]->attr->system.memory_kB += machine->attr->machine.memory_kB;
      topology->levels[0][0]->attr->system.huge_page_free += machine->attr->machine.huge_page_free;

      /* Gather DMI info */
      /* FIXME: get the right DMI info of each machine */
      topo__get_dmi_info(topology,
			 &machine->attr->machine.dmi_board_vendor,
			 &machine->attr->machine.dmi_board_name);
    }
    closedir(nodes_dir);
  } else {
    /* Gather the list of admin-disabled cpus and mems */
    if (!(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM)) {
      topo_admin_disable_set_from_cpuset(topology, "/proc/self/cpuset", "cpus", &admin_disabled_cpus_set);
      topo_admin_disable_set_from_cpuset(topology, "/proc/self/cpuset", "mems", &admin_disabled_mems_set);
    }

    /* Gather NUMA information */
    look_sysfsnode(topology, "/sys/devices/system/node", &admin_disabled_mems_set);

    /* Gather the list of cpus now */
    if (getenv("TOPO_LINUX_USE_CPUINFO")
	|| topo_access("/sys/devices/system/cpu/cpu0/topology/core_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| topo_access("/sys/devices/system/cpu/cpu0/topology/core_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| topo_access("/sys/devices/system/cpu/cpu0/topology/physical_package_id", R_OK, topology->backend_params.sysfs.root_fd) < 0
	|| topo_access("/sys/devices/system/cpu/cpu0/topology/thread_siblings", R_OK, topology->backend_params.sysfs.root_fd) < 0) {
	/* revert to reading cpuinfo only if /sys/.../topology unavailable (before 2.6.16) */
      err = look_cpuinfo(topology, "/proc/cpuinfo", &online_set, &admin_disabled_cpus_set);
      if (err < 0) {
	fprintf(stderr, "sysfs topology not found, and /proc/cpuinfo missing\n");
	abort();
      }
    } else {
      look_sysfscpu(topology, "/sys/devices/system/cpu", &admin_disabled_cpus_set);
    }

    /* Compute the whole system memory and huge page */
    topo_get_procfs_meminfo_info(topology,
				 "/proc/meminfo",
				 &topology->levels[0][0]->attr->system.memory_kB,
				 &topology->levels[0][0]->attr->system.huge_page_size_kB,
				 &topology->levels[0][0]->attr->system.huge_page_free);
				 /* FIXME: gather page_size_kB as well? MaMI needs it */

    /* Gather DMI info */
    topo__get_dmi_info(topology,
		       &topology->levels[0][0]->attr->system.dmi_board_vendor,
		       &topology->levels[0][0]->attr->system.dmi_board_name);
  }
}

/* TODO mbind, setpolicy */
