/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <numa.h>
#include <radset.h>
#include <cpuset.h>
#include <sys/mman.h>

/*
 * TODO REPLICATED
 *
 * nsg_init(), nsg_attach_pid(), RAD_MIGRATE/RAD_WAIT
 * assign_pid_to_pset()
 */

static int
prepare_radset(hwloc_topology_t topology, radset_t *radset, hwloc_const_cpuset_t hwloc_set)
{
  unsigned cpu;
  cpuset_t target_cpuset;
  cpuset_t cpuset, xor_cpuset;
  radid_t radid;
  int ret = 0;
  int ret_errno = 0;

  cpusetcreate(&target_cpuset);
  cpuemptyset(target_cpuset);
  hwloc_cpuset_foreach_begin(cpu, hwloc_set)
    cpuaddset(target_cpuset, cpu);
  hwloc_cpuset_foreach_end();

  cpusetcreate(&cpuset);
  cpusetcreate(&xor_cpuset);
  for (radid = 0; radid < topology->backend_params.osf.nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }
    cpuxorset(target_cpuset, cpuset, xor_cpuset);
    if (cpucountset(xor_cpuset) == 0) {
      /* Found it */
      radsetcreate(radset);
      rademptyset(*radset);
      radaddset(*radset, radid);
      ret = 1;
      goto out;
    }
  }
  /* radset containing exactly this set of CPUs not found */
  ret_errno = EXDEV;

out:
  cpusetdestroy(&target_cpuset);
  cpusetdestroy(&cpuset);
  cpusetdestroy(&xor_cpuset);
  errno = ret_errno;
  return ret;
}

/* Note: get_cpubind not available on OSF */

static int
hwloc_osf_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t thread, hwloc_const_cpuset_t hwloc_set, int policy)
{
  radset_t radset;

  if (hwloc_cpuset_isequal(hwloc_set, hwloc_topology_get_complete_cpuset(topology))) {
    if ((errno = pthread_rad_detach(thread)))
      return -1;
    return 0;
  }

  if (!prepare_radset(topology, &radset, hwloc_set))
    return -1;

  if (policy & HWLOC_CPUBIND_STRICT) {
    if ((errno = pthread_rad_bind(thread, radset, RAD_INSIST | RAD_WAIT)))
      return -1;
  } else {
    if ((errno = pthread_rad_attach(thread, radset, RAD_WAIT)))
      return -1;
  }
  radsetdestroy(&radset);

  return 0;
}

static int
hwloc_osf_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t hwloc_set, int policy)
{
  radset_t radset;

  if (hwloc_cpuset_isequal(hwloc_set, hwloc_topology_get_complete_cpuset(topology))) {
    if (rad_detach_pid(pid))
      return -1;
    return 0;
  }

  if (!prepare_radset(topology, &radset, hwloc_set))
    return -1;

  if (policy & HWLOC_CPUBIND_STRICT) {
    if (rad_bind_pid(pid, radset, RAD_INSIST | RAD_WAIT))
      return -1;
  } else {
    if (rad_attach_pid(pid, radset, RAD_WAIT))
      return -1;
  }
  radsetdestroy(&radset);

  return 0;
}

static int
hwloc_osf_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t hwloc_set, int policy)
{
  return hwloc_osf_set_thread_cpubind(topology, pthread_self(), hwloc_set, policy);
}

static int
hwloc_osf_set_thisproc_cpubind(hwloc_topology_t topology, hwloc_const_cpuset_t hwloc_set, int policy)
{
  return hwloc_osf_set_proc_cpubind(topology, getpid(), hwloc_set, policy);
}

static int
hwloc_osf_prepare_mattr(hwloc_topology_t topology __hwloc_attribute_unused, memalloc_attr_t *mattr, hwloc_const_nodeset_t nodeset, int policy) {
  unsigned long osf_policy;
  int node;

  switch (policy & ~(HWLOC_MEMBIND_MIGRATE|HWLOC_MEMBIND_STRICT)) {
    case HWLOC_MEMBIND_FIRSTTOUCH:
      osf_policy = MPOL_THREAD;
      break;
    case HWLOC_MEMBIND_DEFAULT:
    case HWLOC_MEMBIND_BIND:
      osf_policy = MPOL_DIRECTED;
      break;
    case HWLOC_MEMBIND_INTERLEAVE:
      errno = MPOL_STRIPPED;
      break;
    /* TODO: REPLICATED */
    default:
      errno = EINVAL;
      return -1;
  }

  memset(mattr, 0, sizeof(*mattr));
  mattr->mattr_policy = osf_policy;
  mattr->mattr_rad = RAD_NONE;
  radsetcreate(&mattr->mattr_radset);
  rademptyset(mattr->mattr_radset);

  hwloc_cpuset_foreach_begin(node, nodeset)
    radaddset(mattr->mattr_radset, node);
  hwloc_cpuset_foreach_end();
  return 0;
}

static int
hwloc_osf_set_area_membind(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_nodeset_t nodeset, int policy) {
  memalloc_attr_t mattr;
  int behavior = 0;
  int ret;

  if (policy & HWLOC_MEMBIND_MIGRATE)
    behavior |= MADV_CURRENT;
  if (policy & HWLOC_MEMBIND_STRICT)
    behavior |= MADV_INSIST;

  if (hwloc_osf_prepare_mattr(topology, &mattr, nodeset, policy))
    return -1;

  ret = nmadvise(addr, len, MADV_CURRENT, &mattr);
  radsetdestroy(&mattr.mattr_radset);
  return ret;
}

static void *
hwloc_osf_alloc_membind(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, int policy) {
  memalloc_attr_t mattr;
  void *ptr;

  if (hwloc_osf_prepare_mattr(topology, &mattr, nodeset, policy))
    return NULL;

  /* TODO: rather use acreate/amalloc ? */
  ptr = nmmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
               0, &mattr);
  radsetdestroy(&mattr.mattr_radset);
  return ptr;
}

static int
hwloc_osf_free_membind(hwloc_topology_t topology __hwloc_attribute_unused, void *addr, size_t len) {
  return munmap(addr, len);
}

void
hwloc_look_osf(struct hwloc_topology *topology)
{
  cpu_cursor_t cursor;
  unsigned nbnodes;
  radid_t radid, radid2;
  radset_t radset, radset2;
  cpuid_t cpuid;
  cpuset_t cpuset;
  struct hwloc_obj *obj;
  unsigned distance;

  topology->backend_params.osf.nbnodes = nbnodes = rad_get_num();

  cpusetcreate(&cpuset);
  radsetcreate(&radset);
  radsetcreate(&radset2);
  {
    hwloc_obj_t nodes[nbnodes];
    unsigned distances[nbnodes][nbnodes];
    unsigned nfound;
    numa_attr_t attr = {
      .nattr_type = R_RAD,
      .nattr_descr = { .rd_radset = radset },
      .nattr_flags = 0,
    };

    for (radid = 0; radid < (radid_t) nbnodes; radid++) {
      rademptyset(radset);
      radaddset(radset, radid);
      cpuemptyset(cpuset);
      if (rad_get_cpus(radid, cpuset)==-1) {
	fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
	continue;
      }

      nodes[radid] = obj = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, radid);
      obj->cpuset = hwloc_cpuset_alloc();
      obj->memory.local_memory = rad_get_physmem(radid) * getpagesize();
      obj->memory.page_types_len = 2;
      obj->memory.page_types = malloc(2*sizeof(*obj->memory.page_types));
      memset(obj->memory.page_types, 0, 2*sizeof(*obj->memory.page_types));
      obj->memory.page_types[0].size = getpagesize();
#ifdef HAVE__SC_LARGE_PAGESIZE
      obj->memory.page_types[1].size = sysconf(_SC_LARGE_PAGESIZE);
#endif

      cursor = SET_CURSOR_INIT;
      while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
	hwloc_cpuset_set(obj->cpuset, cpuid);

      hwloc_debug_1arg_cpuset("node %d has cpuset %s\n",
		 radid, obj->cpuset);

      hwloc_insert_object_by_cpuset(topology, obj);

      nfound = 0;
      for (radid2 = 0; radid2 < (radid_t) nbnodes; radid2++)
	distances[radid][radid2] = RAD_DIST_REMOTE;
      for (distance = RAD_DIST_LOCAL; distance < RAD_DIST_REMOTE; distance++) {
	attr.nattr_distance = distance;
	/* get set of NUMA nodes at distance <= DISTANCE */
	if (nloc(&attr, radset2)) {
	  fprintf(stderr,"nloc failed: %s\n", strerror(errno));
	  continue;
	}
	cursor = SET_CURSOR_INIT;
	while ((radid2 = rad_foreach(radset2, 0, &cursor)) != RAD_NONE) {
	  if (distances[radid][radid2] == RAD_DIST_REMOTE) {
	    distances[radid][radid2] = distance;
	    nfound++;
	  }
	}
	if (nfound == nbnodes)
	  /* Finished finding distances, no need to go up to RAD_DIST_REMOTE */
	  break;
      }
    }
    hwloc_setup_misc_level_from_distances(topology, nbnodes, nodes, (unsigned*) distances);
  }
  radsetdestroy(&radset2);
  radsetdestroy(&radset);
  cpusetdestroy(&cpuset);

  /* add PU objects */
  hwloc_setup_pu_level(topology, hwloc_fallback_nbprocessors(topology));

  hwloc_add_object_info(topology->levels[0][0], "Backend", "OSF");
}

void
hwloc_set_osf_hooks(struct hwloc_topology *topology)
{
  topology->set_thread_cpubind = hwloc_osf_set_thread_cpubind;
  topology->set_thisthread_cpubind = hwloc_osf_set_thisthread_cpubind;
  topology->set_proc_cpubind = hwloc_osf_set_proc_cpubind;
  topology->set_thisproc_cpubind = hwloc_osf_set_thisproc_cpubind;
  topology->set_area_membind = hwloc_osf_set_area_membind;
  topology->alloc_membind = hwloc_osf_alloc_membind;
  topology->free_membind = hwloc_osf_free_membind;
}
