/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <hwloc.h>
#include <hwloc/glibc-sched.h>

/* check the linux libnuma helpers */

int main(void)
{
  hwloc_topology_t topology;
  struct hwloc_topology_info topoinfo;
  hwloc_cpuset_t toposet;
  cpu_set_t schedset;
  hwloc_obj_t obj;
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  hwloc_topology_get_info(topology, &topoinfo);

  toposet = hwloc_get_system_obj(topology)->cpuset;
  hwloc_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  hwloc_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  hwloc_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(hwloc_cpuset_isequal(&toposet, &hwloc_get_system_obj(topology)->cpuset));

  obj = hwloc_get_obj_by_depth(topology, topoinfo.depth-1, hwloc_get_depth_nbobjs(topology, topoinfo.depth-1) - 1);
  assert(obj);
  assert(obj->type == HWLOC_OBJ_PROC);

  toposet = obj->cpuset;
  hwloc_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  hwloc_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  hwloc_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(hwloc_cpuset_isequal(&toposet, &obj->cpuset));

  hwloc_topology_destroy(topology);
  return 0;
}
