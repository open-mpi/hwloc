/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <topology.h>
#include <private/private.h>
#include <topology/helper.h>

#include <errno.h>

/* TODO: GNU_SYS, FREEBSD_SYS, DARWIN_SYS, IRIX_SYS, HPUX_SYS
 * IRIX: see _DSM_MUSTRUN */

int
topo_set_cpubind(topo_topology_t topology, const topo_cpuset_t *set,
		 int policy)
{
  int strict = !!(policy & TOPO_CPUBIND_STRICT);
  topo_cpuset_t *system_set = &topo_get_system_obj(topology)->cpuset;

  if (topo_cpuset_isfull(set))
    set = system_set;

  if (!topo_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (policy & TOPO_CPUBIND_PROCESS) {
    if (topology->set_thisproc_cpubind)
      return topology->set_thisproc_cpubind(topology, set, strict);
  } else if (policy & TOPO_CPUBIND_THREAD) {
    if (topology->set_thisthread_cpubind)
      return topology->set_thisthread_cpubind(topology, set, strict);
  } else {
    if (topology->set_cpubind)
      return topology->set_cpubind(topology, set, strict);
  }

  errno = ENOSYS;
  return -1;
}

int
topo_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *set, int policy)
{
  int strict = !!(policy & TOPO_CPUBIND_STRICT);
  topo_cpuset_t *system_set = &topo_get_system_obj(topology)->cpuset;

  if (topo_cpuset_isfull(set))
    set = &topo_get_system_obj(topology)->cpuset;

  if (!topo_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (topology->set_proc_cpubind)
    return topology->set_proc_cpubind(topology, pid, set, strict);

  errno = ENOSYS;
  return -1;
}

int
topo_set_thread_cpubind(topo_topology_t topology, topo_thread_t tid, const topo_cpuset_t *set, int policy)
{
  int strict = !!(policy & TOPO_CPUBIND_STRICT);
  topo_cpuset_t *system_set = &topo_get_system_obj(topology)->cpuset;

  if (topo_cpuset_isfull(set))
    set = &topo_get_system_obj(topology)->cpuset;

  if (!topo_cpuset_isincluded(set, system_set)) {
    errno = EINVAL;
    return -1;
  }

  if (topology->set_thread_cpubind)
    return topology->set_thread_cpubind(topology, tid, set, strict);

  errno = ENOSYS;
  return -1;
}

/* TODO: memory bind */
