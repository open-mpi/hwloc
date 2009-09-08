/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_get_cpuset_covering_cache() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t obj, cache;
  topo_cpuset_t set;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  topo_cpuset_zero(&set);
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_get_cpuset_covering_cache(topology, &set);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX/2/3);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check the cache above two nearby cpus */
#define CPUINDEX1 180
#define CPUINDEX2 183
  topo_cpuset_zero(&set);
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX1);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX2);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_get_cpuset_covering_cache(topology, &set);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX1/2/3);
  assert(cache->logical_index == CPUINDEX2/2/3);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check no cache above two distant cpus */
#undef CPUINDEX1
#define CPUINDEX1 300
  topo_cpuset_zero(&set);
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX1);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX2);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_get_cpuset_covering_cache(topology, &set);
  assert(!cache);

  /* check no cache above higher level */
  topo_cpuset_zero(&set);
  obj = topo_get_obj_by_depth(topology, 2, 0);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_get_cpuset_covering_cache(topology, &set);
  assert(!cache);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
