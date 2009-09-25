/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdio.h>
#include <assert.h>

/* check topo_get_type{,_or_above,_or_below}_depth()
 * and hwloc_get_depth_type()
 */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "machine:3 misc:2 core:3 cache:2 cache:2 2"

int main()
{
  hwloc_topology_t topology;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  hwloc_topology_load(topology);

  assert(hwloc_topology_get_depth(topology) == 7);

  assert(hwloc_get_depth_type(topology, 0) == HWLOC_OBJ_SYSTEM);
  assert(hwloc_get_depth_type(topology, 1) == HWLOC_OBJ_MACHINE);
  assert(hwloc_get_depth_type(topology, 2) == HWLOC_OBJ_MISC);
  assert(hwloc_get_depth_type(topology, 3) == HWLOC_OBJ_CORE);
  assert(hwloc_get_depth_type(topology, 4) == HWLOC_OBJ_CACHE);
  assert(hwloc_get_depth_type(topology, 5) == HWLOC_OBJ_CACHE);
  assert(hwloc_get_depth_type(topology, 6) == HWLOC_OBJ_PROC);

  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_MACHINE) == 1);
  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_MISC) == 2);
  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_CORE) == 3);
  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_PROC) == 6);

  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_NODE) == HWLOC_TYPE_DEPTH_UNKNOWN);
  assert(hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_NODE) == 2);
  assert(hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_NODE) == 3);
  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET) == HWLOC_TYPE_DEPTH_UNKNOWN);
  assert(hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_SOCKET) == 2);
  assert(hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_SOCKET) == 3);
  assert(hwloc_get_type_depth(topology, HWLOC_OBJ_CACHE) == HWLOC_TYPE_DEPTH_MULTIPLE);
  assert(hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_CACHE) == HWLOC_TYPE_DEPTH_MULTIPLE);
  assert(hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CACHE) == HWLOC_TYPE_DEPTH_MULTIPLE);

  hwloc_topology_destroy(topology);

  return EXIT_SUCCESS;
}
