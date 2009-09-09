/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_get_shared_cache_above() */

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
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX/2/3);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check no shared cache above the L2 cache */
  obj = topo_get_obj_by_depth(topology, 3, 0);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  /* check no shared cache above the node */
  obj = topo_get_obj_by_depth(topology, 1, 0);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);


  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION_NONSHARED);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  obj = topo_get_obj_by_depth(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX/2/1);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check no shared-cache above the core */
  obj = topo_get_obj_by_depth(topology, 4, CPUINDEX/2);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
