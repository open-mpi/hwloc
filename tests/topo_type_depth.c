/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdio.h>
#include <assert.h>

/* check topo_get_type{,_or_above,_or_below}_depth()
 * and topo_get_depth_type()
 */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "machine:3 misc:2 core:3 cache:2 cache:2 2"

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  assert(topoinfo.depth == 7);

  assert(topo_get_depth_type(topology, 0) == TOPO_OBJ_SYSTEM);
  assert(topo_get_depth_type(topology, 1) == TOPO_OBJ_MACHINE);
  assert(topo_get_depth_type(topology, 2) == TOPO_OBJ_MISC);
  assert(topo_get_depth_type(topology, 3) == TOPO_OBJ_CORE);
  assert(topo_get_depth_type(topology, 4) == TOPO_OBJ_CACHE);
  assert(topo_get_depth_type(topology, 5) == TOPO_OBJ_CACHE);
  assert(topo_get_depth_type(topology, 6) == TOPO_OBJ_PROC);

  assert(topo_get_type_depth(topology, TOPO_OBJ_MACHINE) == 1);
  assert(topo_get_type_depth(topology, TOPO_OBJ_MISC) == 2);
  assert(topo_get_type_depth(topology, TOPO_OBJ_CORE) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_PROC) == 6);

  assert(topo_get_type_depth(topology, TOPO_OBJ_NODE) == TOPO_TYPE_DEPTH_UNKNOWN);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_NODE) == 2);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_NODE) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_SOCKET) == TOPO_TYPE_DEPTH_UNKNOWN);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_SOCKET) == 2);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_SOCKET) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
