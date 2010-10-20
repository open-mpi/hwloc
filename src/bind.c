/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <hwloc/helper.h>
#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* TODO: HWLOC_GNU_SYS, HWLOC_IRIX_SYS,
 *
 * IRIX: see MP_MUSTRUN / _DSM_MUSTRUN, pthread_setrunon_np, /hw, procss_cpulink, numa_create
 *
 * We could use glibc's sched_setaffinity generically when it is available
 *
 * Darwin and OpenBSD don't seem to have binding facilities.
 */

static hwloc_const_bitmap_t
hwloc_fix_cpubind(hwloc_topology_t topology, hwloc_const_bitmap_t set)
{
  hwloc_const_bitmap_t topology_set = hwloc_topology_get_topology_cpuset(topology);
  hwloc_const_bitmap_t complete_set = hwloc_topology_get_complete_cpuset(topology);

  if (!topology_set) {
    /* The topology is composed of several systems, the cpuset is ambiguous. */
    errno = EXDEV;
    return NULL;
  }

  if (!hwloc_bitmap_isincluded(set, complete_set)) {
    errno = EINVAL;
    return NULL;
  }

  if (hwloc_bitmap_isincluded(topology_set, set))
    set = complete_set;

  return set;
}

int
hwloc_set_cpubind(hwloc_topology_t topology, hwloc_const_bitmap_t set, int flags)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (flags & HWLOC_CPUBIND_PROCESS) {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, flags);
  } else if (flags & HWLOC_CPUBIND_THREAD) {
    if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, flags);
  } else {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, flags);
    else if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, flags);
  }

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_cpubind(hwloc_topology_t topology, hwloc_bitmap_t set, int flags)
{
  if (flags & HWLOC_CPUBIND_PROCESS) {
    if (topology->get_thisproc_cpubind)
      return topology->get_thisproc_cpubind(topology, set, flags);
  } else if (flags & HWLOC_CPUBIND_THREAD) {
    if (topology->get_thisthread_cpubind)
      return topology->get_thisthread_cpubind(topology, set, flags);
  } else {
    if (topology->get_thisproc_cpubind)
      return topology->get_thisproc_cpubind(topology, set, flags);
    else if (topology->get_thisthread_cpubind)
      return topology->get_thisthread_cpubind(topology, set, flags);
  }

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_bitmap_t set, int flags)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (topology->set_proc_cpubind)
    return topology->set_proc_cpubind(topology, pid, set, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_bitmap_t set, int flags)
{
  if (topology->get_proc_cpubind)
    return topology->get_proc_cpubind(topology, pid, set, flags);

  errno = ENOSYS;
  return -1;
}

#ifdef hwloc_thread_t
int
hwloc_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_const_bitmap_t set, int flags)
{
  set = hwloc_fix_cpubind(topology, set);
  if (!set)
    return -1;

  if (topology->set_thread_cpubind)
    return topology->set_thread_cpubind(topology, tid, set, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_bitmap_t set, int flags)
{
  if (topology->get_thread_cpubind)
    return topology->get_thread_cpubind(topology, tid, set, flags);

  errno = ENOSYS;
  return -1;
}
#endif

static hwloc_const_bitmap_t
hwloc_fix_cpumembind_cpuset(hwloc_topology_t topology, hwloc_const_cpuset_t set)
{
  hwloc_const_bitmap_t topology_set = hwloc_topology_get_topology_cpuset(topology);
  hwloc_const_bitmap_t complete_set = hwloc_topology_get_complete_cpuset(topology);

  if (!topology_set) {
    /* The topology is composed of several systems, the cpuset is thus
     * ambiguous. */
    errno = EXDEV;
    return NULL;
  }

  if (!hwloc_get_root_obj(topology)->complete_nodeset) {
    /* There is no NUMA node */
    errno = ENODEV;
    return NULL;
  }

  if (!hwloc_bitmap_isincluded(set, complete_set)) {
    errno = EINVAL;
    return NULL;
  }

  if (hwloc_bitmap_isincluded(topology_set, set))
    set = complete_set;

  return set;
}

int
hwloc_set_cpumembind(hwloc_topology_t topology, hwloc_const_cpuset_t cpuset, hwloc_membind_policy_t mempolicy, int flags)
{
  if (topology->set_cpumembind) {
    cpuset = hwloc_fix_cpumembind_cpuset(topology, cpuset);
    if (!cpuset)
      return -1;
    return topology->set_cpumembind(topology, cpuset, mempolicy, flags);
  } else {
    int err;
    err = hwloc_set_cpubind(topology, cpuset, flags);
    if (err < 0)
      return err;
    return hwloc_set_membind(topology, cpuset, mempolicy, flags);
  }
}

int
hwloc_set_proc_cpumembind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t cpuset, hwloc_membind_policy_t mempolicy, int flags)
{
  if (topology->set_proc_cpumembind) {
    cpuset = hwloc_fix_cpumembind_cpuset(topology, cpuset);
    if (!cpuset)
      return -1;
    return topology->set_proc_cpumembind(topology, pid, cpuset, mempolicy, flags);
  } else {
    int err;
    err = hwloc_set_proc_cpubind(topology, pid, cpuset, flags);
    if (err < 0)
      return err;
    return hwloc_set_proc_membind(topology, pid, cpuset, mempolicy, flags);
  }
}

static hwloc_const_nodeset_t
hwloc_fix_membind(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset)
{
  hwloc_const_bitmap_t topology_nodeset = hwloc_get_root_obj(topology)->nodeset;
  hwloc_const_bitmap_t complete_nodeset = hwloc_get_root_obj(topology)->complete_nodeset;

  if (!hwloc_topology_get_topology_cpuset(topology)) {
    /* The topology is composed of several systems, the nodeset is thus
     * ambiguous. */
    errno = EXDEV;
    return NULL;
  }

  if (!complete_nodeset) {
    /* There is no NUMA node */
    errno = ENODEV;
    return NULL;
  }

  if (!hwloc_bitmap_isincluded(nodeset, complete_nodeset)) {
    errno = EINVAL;
    return NULL;
  }

  if (hwloc_bitmap_isincluded(topology_nodeset, nodeset))
    return complete_nodeset;

  return nodeset;
}

static int
hwloc_fix_membind_cpuset(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_const_cpuset_t cpuset)
{
  hwloc_const_bitmap_t topology_set = hwloc_topology_get_topology_cpuset(topology);
  hwloc_const_bitmap_t complete_set = hwloc_topology_get_complete_cpuset(topology);
  hwloc_const_bitmap_t complete_nodeset = hwloc_get_root_obj(topology)->complete_nodeset;

  if (!topology_set) {
    /* The topology is composed of several systems, the cpuset is thus
     * ambiguous. */
    errno = EXDEV;
    return -1;
  }

  if (!complete_nodeset) {
    /* There is no NUMA node */
    errno = ENODEV;
    return -1;
  }

  if (!hwloc_bitmap_isincluded(cpuset, complete_set)) {
    errno = EINVAL;
    return -1;
  }

  if (hwloc_bitmap_isincluded(topology_set, cpuset)) {
    hwloc_bitmap_copy(nodeset, complete_nodeset);
    return 0;
  }

  hwloc_cpuset_to_nodeset(topology, cpuset, nodeset);
  return 0;
}

int
hwloc_set_membind_nodeset(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  nodeset = hwloc_fix_membind(topology, nodeset);
  if (!nodeset)
    return -1;

  if (topology->set_membind)
    return topology->set_membind(topology, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_membind(hwloc_topology_t topology, hwloc_const_cpuset_t set, hwloc_membind_policy_t policy, int flags)
{
  hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
  int ret;

  if (!hwloc_fix_membind_cpuset(topology, nodeset, set))
    ret = -1;
  else
    ret = hwloc_set_membind_nodeset(topology, nodeset, policy, flags);

  hwloc_bitmap_free(nodeset);
  return ret;
}

int
hwloc_get_membind_nodeset(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  if (topology->get_membind)
    return topology->get_membind(topology, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_membind(hwloc_topology_t topology, hwloc_cpuset_t set, hwloc_membind_policy_t * policy, int flags)
{
  hwloc_nodeset_t nodeset;
  int ret;

  nodeset = hwloc_bitmap_alloc();
  ret = hwloc_get_membind_nodeset(topology, nodeset, policy, flags);

  if (!ret)
    hwloc_cpuset_from_nodeset(topology, set, nodeset);

  return ret;
}

int
hwloc_set_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  nodeset = hwloc_fix_membind(topology, nodeset);
  if (!nodeset)
    return -1;

  if (topology->set_proc_membind)
    return topology->set_proc_membind(topology, pid, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t set, hwloc_membind_policy_t policy, int flags)
{
  hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
  int ret;

  if (!hwloc_fix_membind_cpuset(topology, nodeset, set))
    ret = -1;
  else
    ret = hwloc_set_proc_membind_nodeset(topology, pid, set, policy, flags);

  hwloc_bitmap_free(nodeset);
  return ret;
}

int
hwloc_get_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  if (topology->get_proc_membind)
    return topology->get_proc_membind(topology, pid, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, hwloc_membind_policy_t * policy, int flags)
{
  hwloc_nodeset_t nodeset;
  int ret;

  nodeset = hwloc_bitmap_alloc();
  ret = hwloc_get_proc_membind_nodeset(topology, pid, nodeset, policy, flags);

  if (!ret)
    hwloc_cpuset_from_nodeset(topology, set, nodeset);

  return ret;
}

int
hwloc_set_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  nodeset = hwloc_fix_membind(topology, nodeset);
  if (!nodeset)
    return -1;

  if (topology->set_area_membind)
    return topology->set_area_membind(topology, addr, len, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_set_area_membind(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_cpuset_t set, hwloc_membind_policy_t policy, int flags)
{
  hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
  int ret;

  if (!hwloc_fix_membind_cpuset(topology, nodeset, set))
    ret = -1;
  else
    ret = hwloc_set_area_membind_nodeset(topology, addr, len, nodeset, policy, flags);

  hwloc_bitmap_free(nodeset);
  return ret;
}

int
hwloc_get_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  if (topology->get_area_membind)
    return topology->get_area_membind(topology, addr, len, nodeset, policy, flags);

  errno = ENOSYS;
  return -1;
}

int
hwloc_get_area_membind(hwloc_topology_t topology, const void *addr, size_t len, hwloc_cpuset_t set, hwloc_membind_policy_t * policy, int flags)
{
  hwloc_nodeset_t nodeset;
  int ret;

  nodeset = hwloc_bitmap_alloc();
  ret = hwloc_get_area_membind_nodeset(topology, addr, len, nodeset, policy, flags);

  if (!ret)
    hwloc_cpuset_from_nodeset(topology, set, nodeset);

  return ret;
}

void *
hwloc_alloc_membind_nodeset(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  nodeset = hwloc_fix_membind(topology, nodeset);
  if (!nodeset)
    return NULL;
  if (flags & HWLOC_MEMBIND_MIGRATE) {
    errno = EINVAL;
    return NULL;
  }

  if (topology->alloc_membind)
    return topology->alloc_membind(topology, len, nodeset, policy, flags);
  else if (topology->set_area_membind) {
    void *p;
#if defined(HAVE_GETPAGESIZE) && defined(HAVE_POSIX_MEMALIGN)
    errno = posix_memalign(&p, getpagesize(), len);
    if (errno)
      p = NULL;
#elif defined(HAVE_GETPAGESIZE) && defined(HAVE_MEMALIGN)
    p = memalign(getpagesize(), len);
#else
    p = malloc(len);
#endif
    if (!topology->set_area_membind(topology, p, len, nodeset, policy, flags)) {
      int error = errno;
      free(p);
      errno = error;
      return NULL;
    }
    return p;
  }

  errno = ENOSYS;
  return NULL;
}

void *
hwloc_alloc_membind(hwloc_topology_t topology, size_t len, hwloc_const_cpuset_t set, hwloc_membind_policy_t policy, int flags)
{
  hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
  void *ret;

  if (!hwloc_fix_membind_cpuset(topology, nodeset, set))
    ret = NULL;
  else
    ret = hwloc_alloc_membind_nodeset(topology, len, nodeset, policy, flags);

  hwloc_bitmap_free(nodeset);
  return ret;
}

int
hwloc_free_membind(hwloc_topology_t topology, void *addr, size_t len)
{
  if (topology->free_membind)
    return topology->free_membind(topology, addr, len);
  else if (topology->set_area_membind)
    free(addr);

  errno = ENOSYS;
  return -1;
}
