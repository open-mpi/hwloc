/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * check topo_get_next_obj_above_cpuset_by_depth()
 * and topo_get_next_obj_above_cpuset()
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  topo_cpuset_t set;
  topo_obj_t obj;
  int depth;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  topo_cpuset_from_string("00008f18", &set);

  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, NULL);
  assert(obj == topo_get_obj_by_depth(topology, 1, 1));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 2));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 4));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 5));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 7));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(!obj);

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  topo_topology_set_synthetic (topology, "ndoes:2 socket:5 cores:3 4");
  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  topo_cpuset_from_string("0ff08000", &set);

  depth = topo_get_type_depth(topology, TOPO_OBJ_SOCKET);
  assert(depth == 2);
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, NULL);
  assert(obj == topo_get_obj_by_depth(topology, depth, 1));
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, obj);
  assert(obj == topo_get_obj_by_depth(topology, depth, 2));
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, obj);
  assert(!obj);

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
