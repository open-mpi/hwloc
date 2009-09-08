/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * check topo_get_obj_below_cpuset*()
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info info;
  topo_obj_t obj, root;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "nodes:2 sockets:3 caches:4 cores:5 6");

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  err = topo_topology_get_info(topology, &info);
  if (err)
    return EXIT_FAILURE;

  /* there is no second system object */
  root = topo_get_system_obj (topology);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, 1);
  assert(!obj);

  /* first system object is the top-level object of the topology */
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, 0);
  assert(obj == topo_get_system_obj(topology));

  /* first next-object object is the top-level object of the topology */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, NULL);
  assert(obj == topo_get_system_obj(topology));
  /* there is no next object after the system object */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, obj);
  assert(!obj);

  /* check last proc */
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, 2*3*4*5*6-1);
  assert(obj == topo_get_obj_by_depth(topology, 5, 2*3*4*5*6-1));
  /* there is no next proc after the last one */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, obj);
  assert(!obj);


  /* check there are 20 cores below first socket */
  root = topo_get_obj_by_depth(topology, 2, 0);
  assert(topo_get_nbobjs_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CORE) == 20);

  /* check there are 12 caches below last node */
  root = topo_get_obj_by_depth(topology, 1, 1);
  assert(topo_get_nbobjs_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CACHE) == 12);


  /* check first proc of second socket */
  root = topo_get_obj_by_depth(topology, 2, 1);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, 0);
  assert(obj == topo_get_obj_by_depth(topology, 5, 4*5*6));

  /* check third core of third socket */
  root = topo_get_obj_by_depth(topology, 2, 2);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CORE, 2);
  assert(obj == topo_get_obj_by_depth(topology, 4, 2*4*5+2));

  /* check first socket of second node */
  root = topo_get_obj_by_depth(topology, 1, 1);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SOCKET, 0);
  assert(obj == topo_get_obj_by_depth(topology, 2, 3));

  /* there is no node below sockets */
  root = topo_get_obj_by_depth(topology, 2, 0);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_NODE, 0);
  assert(!obj);

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
