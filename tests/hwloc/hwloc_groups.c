/*
 * Copyright © 2011-2016 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* intensive testing of two grouping cases (2+1 and 2+2+1) */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t obj;
  hwloc_obj_t objs[5];
  uint64_t values[5*5];
  unsigned depth;
  unsigned width;
  unsigned i;
  int err;

  /* group 3 numa nodes as 1 group of 2 and 1 on the side */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:3 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<3; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
  values[0] = 1; values[1] = 4; values[2] = 4;
  values[3] = 4; values[4] = 1; values[5] = 2;
  values[6] = 4; values[7] = 2; values[8] = 1;
  err = hwloc_distances_add(topology, 3, objs, values,
			    HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			    HWLOC_DISTANCES_FLAG_GROUP);
  assert(!err);
  /* 2 groups at depth 1 */
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_GROUP);
  assert(depth == 1);
  width = hwloc_get_nbobjs_by_depth(topology, depth);
  assert(width == 1);
  /* 3 node at depth 2 */
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);
  assert(depth == 2);
  width = hwloc_get_nbobjs_by_depth(topology, depth);
  assert(width == 3);
  /* find the root obj */
  obj = hwloc_get_root_obj(topology);
  assert(obj->arity == 2);
  /* check its children */
  assert(obj->children[0]->type == HWLOC_OBJ_NUMANODE);
  assert(obj->children[0]->depth == 2);
  assert(obj->children[0]->arity == 1);
  assert(obj->children[1]->type == HWLOC_OBJ_GROUP);
  assert(obj->children[1]->depth == 1);
  assert(obj->children[1]->arity == 2);
  hwloc_topology_destroy(topology);

  /* group 5 packages as 2 group of 2 and 1 on the side, all of them below a common node object */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:1 pack:5 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<5; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PACKAGE, i);
  values[ 0] = 1; values[ 1] = 2; values[ 2] = 4; values[ 3] = 4; values[ 4] = 4;
  values[ 5] = 2; values[ 6] = 1; values[ 7] = 4; values[ 8] = 4; values[ 9] = 4;
  values[10] = 4; values[11] = 4; values[12] = 1; values[13] = 4; values[14] = 4;
  values[15] = 4; values[16] = 4; values[17] = 4; values[18] = 1; values[19] = 2;
  values[20] = 4; values[21] = 4; values[22] = 4; values[23] = 2; values[24] = 1;
  err = hwloc_distances_add(topology, 5, objs, values,
			    HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			    HWLOC_DISTANCES_FLAG_GROUP);
  assert(!err);
  /* 1 node at depth 1 */
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE);
  assert(depth == 1);
  width = hwloc_get_nbobjs_by_depth(topology, depth);
  assert(width == 1);
  /* 2 groups at depth 2 */
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_GROUP);
  assert(depth == 2);
  width = hwloc_get_nbobjs_by_depth(topology, depth);
  assert(width == 2);
  /* 5 packages at depth 3 */
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PACKAGE);
  assert(depth == 3);
  width = hwloc_get_nbobjs_by_depth(topology, depth);
  assert(width == 5);
  /* find the node obj */
  obj = hwloc_get_root_obj(topology);
  assert(obj->arity == 1);
  obj = obj->children[0];
  assert(obj->type == HWLOC_OBJ_NUMANODE);
  assert(obj->arity == 3);
  /* check its children */
  assert(obj->children[0]->type == HWLOC_OBJ_GROUP);
  assert(obj->children[0]->depth == 2);
  assert(obj->children[0]->arity == 2);
  assert(obj->children[1]->type == HWLOC_OBJ_PACKAGE);
  assert(obj->children[1]->depth == 3);
  assert(obj->children[1]->arity == 1);
  assert(obj->children[2]->type == HWLOC_OBJ_GROUP);
  assert(obj->children[2]->depth == 2);
  assert(obj->children[2]->arity == 2);
  hwloc_topology_destroy(topology);

  return 0;
}
