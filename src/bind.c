/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <hwloc/helper.h>

#include <errno.h>

/* TODO: HWLOC_GNU_SYS, HWLOC_FREEBSD_SYS, HWLOC_DARWIN_SYS, HWLOC_IRIX_SYS, HWLOC_HPUX_SYS
 * IRIX: see _DSM_MUSTRUN */

int
hwloc_set_cpubind(hwloc_topology_t topology, hwloc_cpuset_t set, int policy)
{
  hwloc_cpuset_t system_set = hwloc_get_system_obj(topology)->cpuset;

  if (hwloc_cpuset_isfull(set))
    set = system_set;

  if (!hwloc_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (policy & HWLOC_CPUBIND_PROCESS) {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, policy);
  } else if (policy & HWLOC_CPUBIND_THREAD) {
    if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, policy);
  } else {
    if (topology->set_cpubind)
      return topology->set_cpubind(topology, set, policy);
  }

  errno = ENOSYS;
  return -1;
}

hwloc_cpuset_t
hwloc_get_cpubind(hwloc_topology_t topology, int policy)
{
  if (policy & HWLOC_CPUBIND_PROCESS) {
    if (topology->get_thisproc_cpubind)
      return topology->get_thisproc_cpubind(topology, policy);
  } else if (policy & HWLOC_CPUBIND_THREAD) {
    if (topology->get_thisthread_cpubind)
      return topology->get_thisthread_cpubind(topology, policy);
  } else {
    if (topology->get_cpubind)
      return topology->get_cpubind(topology, policy);
  }

  errno = ENOSYS;
  return NULL;
}

int
hwloc_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int policy)
{
  hwloc_cpuset_t system_set = hwloc_get_system_obj(topology)->cpuset;

  if (hwloc_cpuset_isfull(set))
    set = hwloc_get_system_obj(topology)->cpuset;

  if (!hwloc_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (topology->set_proc_cpubind)
    return topology->set_proc_cpubind(topology, pid, set, policy);

  errno = ENOSYS;
  return -1;
}

hwloc_cpuset_t
hwloc_get_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, int policy)
{
  if (topology->get_proc_cpubind)
    return topology->get_proc_cpubind(topology, pid, policy);

  errno = ENOSYS;
  return NULL;
}

#ifdef hwloc_thread_t
int
hwloc_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_cpuset_t set, int policy)
{
  hwloc_cpuset_t system_set = hwloc_get_system_obj(topology)->cpuset;

  if (hwloc_cpuset_isfull(set))
    set = hwloc_get_system_obj(topology)->cpuset;

  if (!hwloc_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (topology->set_thread_cpubind)
    return topology->set_thread_cpubind(topology, tid, set, policy);

  errno = ENOSYS;
  return -1;
}

hwloc_cpuset_t
hwloc_get_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, int policy)
{
  if (topology->get_thread_cpubind)
    return topology->get_thread_cpubind(topology, tid, policy);

  errno = ENOSYS;
  return NULL;
}
#endif

/* TODO: memory bind */
