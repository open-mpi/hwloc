/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_find_shared_cache_above() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION_SHARED "6 5 4 3 2" /* 736bits wide topology */
#define SYNTHETIC_TOPOLOGY_DESCRIPTION_NONSHARED "6 5 4 1 2" /* 736bits wide topology */

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t obj, cache;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION_SHARED);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  obj = topo_get_object(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_find_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->number == CPUINDEX/2/3);
  assert(topo_object_is_in_subtree(cache, obj));

  /* check no shared cache above the L2 cache */
  obj = topo_get_object(topology, 3, 0);
  assert(obj);
  cache = topo_find_shared_cache_above(topology, obj);
  assert(!cache);

  /* check no shared cache above the node */
  obj = topo_get_object(topology, 1, 0);
  assert(obj);
  cache = topo_find_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);


  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION_NONSHARED);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  obj = topo_get_object(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_find_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->number == CPUINDEX/2/1);
  assert(topo_object_is_in_subtree(cache, obj));

  /* check no shared-cache above the core */
  obj = topo_get_object(topology, 4, CPUINDEX/2);
  assert(obj);
  cache = topo_find_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
