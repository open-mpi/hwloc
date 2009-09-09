/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_get_cpuset_objs() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */
#define GIVEN_HARD_CPUSET_STRING "07ff,ffffffff,e0000000"

#define OBJ_MAX 16

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t objs[OBJ_MAX];
  topo_obj_t obj;
  topo_cpuset_t set;
  int ret;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* just get the system object */
  obj = topo_get_system_obj(topology);
  ret = topo_get_cpuset_objs(topology, &obj->cpuset, objs, 1);
  assert(ret == 1);
  assert(objs[0] == obj);

  /* just get the very last object */
  obj = topo_get_obj_by_depth(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1)-1);
  ret = topo_get_cpuset_objs(topology, &obj->cpuset, objs, 1);
  assert(ret == 1);
  assert(objs[0] == obj);

  /* try an empty one */
  topo_cpuset_zero(&set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == 0);

  /* try an impossible one */
  topo_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == -1);

  /* try a harder one with 1 obj instead of 2 needed */
  topo_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == 1);
  assert(objs[0] == topo_get_obj_by_depth(topology, topoinfo.depth-1, 0));
  /* try a harder one with lots of objs instead of 2 needed */
  ret = topo_get_cpuset_objs(topology, &set, objs, 2);
  assert(ret == 2);
  assert(objs[0] == topo_get_obj_by_depth(topology, topoinfo.depth-1, 0));
  assert(objs[1] == topo_get_obj_by_depth(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1)-1));

  /* try a very hard one */
  topo_cpuset_from_string(GIVEN_HARD_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, OBJ_MAX);
  assert(objs[0] == topo_get_obj_by_depth(topology, 5, 29));
  assert(objs[1] == topo_get_obj_by_depth(topology, 3, 5));
  assert(objs[2] == topo_get_obj_by_depth(topology, 3, 6));
  assert(objs[3] == topo_get_obj_by_depth(topology, 3, 7));
  assert(objs[4] == topo_get_obj_by_depth(topology, 2, 2));
  assert(objs[5] == topo_get_obj_by_depth(topology, 4, 36));
  assert(objs[6] == topo_get_obj_by_depth(topology, 5, 74));

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
