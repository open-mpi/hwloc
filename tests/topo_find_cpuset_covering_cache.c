/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_find_cpuset_covering_cache() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  char string[TOPO_CPUSET_STRING_LENGTH+1];
  topo_obj_t obj, cache;
  topo_cpuset_t set;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  topo_cpuset_zero(&set);
  obj = topo_get_object(topology, 5, CPUINDEX);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_find_cpuset_covering_cache(topology, &set);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->number == CPUINDEX/2/3);
  assert(topo_object_is_in_subtree(cache, obj));

  /* check the cache above two nearby cpus */
#define CPUINDEX1 180
#define CPUINDEX2 183
  topo_cpuset_zero(&set);
  obj = topo_get_object(topology, 5, CPUINDEX1);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  obj = topo_get_object(topology, 5, CPUINDEX2);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_find_cpuset_covering_cache(topology, &set);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->number == CPUINDEX1/2/3);
  assert(cache->number == CPUINDEX2/2/3);
  assert(topo_object_is_in_subtree(cache, obj));

  /* check no cache above two distant cpus */
#undef CPUINDEX1 180
#define CPUINDEX1 300
  topo_cpuset_zero(&set);
  obj = topo_get_object(topology, 5, CPUINDEX1);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  obj = topo_get_object(topology, 5, CPUINDEX2);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_find_cpuset_covering_cache(topology, &set);
  assert(!cache);

  /* check no cache above higher level */
  topo_cpuset_zero(&set);
  obj = topo_get_object(topology, 2, 0);
  assert(obj);
  topo_cpuset_orset(&set, &obj->cpuset);
  cache = topo_find_cpuset_covering_cache(topology, &set);
  assert(!cache);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
