/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <topology.h>
#include <topology/glibc-sched.h>

/* check the linux libnuma helpers */

int main(void)
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t toposet;
  cpu_set_t schedset;
  topo_obj_t obj;
  int err;

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  toposet = topo_get_system_obj(topology)->cpuset;
  topo_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  topo_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  topo_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(topo_cpuset_isequal(&toposet, &topo_get_system_obj(topology)->cpuset));

  obj = topo_get_obj_by_depth(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1) - 1);
  assert(obj);
  assert(obj->type == TOPO_OBJ_PROC);

  toposet = obj->cpuset;
  topo_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  topo_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  topo_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(topo_cpuset_isequal(&toposet, &obj->cpuset));

  topo_topology_destroy(topology);
  return 0;
}
