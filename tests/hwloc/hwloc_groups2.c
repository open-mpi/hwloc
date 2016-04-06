/*
 * Copyright © 2011-2016 Inria.  All rights reserved.
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* testing of adding/replacing/removing distance matrices
   with set_distance_matrix or the environment,
   grouping with/without accuracy
 */


int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t objs[32];
  float values[32*32];
  unsigned i, j;
  unsigned depth;
  unsigned width;

  /* grouping matrix 4*2*2 */
  for(i=0; i<16; i++) {
    for(j=0; j<16; j++)
      if (i==j)
        values[i+16*j] = values[j+16*i] = 3.f;
      else if (i/2==j/2)
        values[i+16*j] = values[j+16*i] = 5.f;
      else if (i/4==j/4)
        values[i+16*j] = values[j+16*i] = 7.f;
      else
        values[i+16*j] = values[j+16*i] = 9.f;
  }

  /* default 2*8*1 */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 16);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 16);
  /* group 8cores as 2*2*2 */
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 6);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 4);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 8);
  width = hwloc_get_nbobjs_by_depth(topology, 4);
  assert(width == 16);
  hwloc_topology_destroy(topology);

  /* play with accuracy */
  values[0] = 2.9f; /* diagonal, instead of 3 (0.0333% error) */
  values[1] = 5.1f; values[16] = 5.2f; /* smallest group, instead of 5 (0.02% error) */
  putenv("HWLOC_GROUPING_ACCURACY=0.1"); /* ok */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP|HWLOC_DISTANCES_FLAG_GROUP_INACCURATE));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 6);
  hwloc_topology_destroy(topology);

  putenv("HWLOC_GROUPING_ACCURACY=try"); /* ok */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP|HWLOC_DISTANCES_FLAG_GROUP_INACCURATE));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 6);
  hwloc_topology_destroy(topology);

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP /* no inaccurate flag, cannot group */));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  hwloc_topology_destroy(topology);

  putenv("HWLOC_GROUPING_ACCURACY=0.01"); /* too small, cannot group */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP|HWLOC_DISTANCES_FLAG_GROUP_INACCURATE));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  hwloc_topology_destroy(topology);

  putenv("HWLOC_GROUPING_ACCURACY=0"); /* full accuracy, cannot group */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");
  hwloc_topology_load(topology);
  for(i=0; i<16; i++)
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
  assert(!hwloc_distances_add(topology, 16, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP|HWLOC_DISTANCES_FLAG_GROUP_INACCURATE));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  hwloc_topology_destroy(topology);

  /* default 2*4*4 */
  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:4 pu:4");
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 8);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 32);
  /* apply useless core distances */
  for(i=0; i<8; i++) {
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
    for(j=0; j<8; j++)
      if (i==j)
        values[i+8*j] = values[j+8*i] = 1.f;
      else if (i/4==j/4)
        values[i+8*j] = values[j+8*i] = 4.f;
      else
        values[i+8*j] = values[j+8*i] = 8.f;
  }
  assert(!hwloc_distances_add(topology, 8, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  /* group 4cores as 2*2 */
  for(i=0; i<8; i++) {
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
    for(j=0; j<8; j++)
      if (i==j)
        values[i+8*j] = values[j+8*i] = 1.f;
      else if (i/2==j/2)
        values[i+8*j] = values[j+8*i] = 4.f;
      else
        values[i+8*j] = values[j+8*i] = 8.f;
  }
  assert(!hwloc_distances_add(topology, 8, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 5);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 4);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 8);
  width = hwloc_get_nbobjs_by_depth(topology, 4);
  assert(width == 32);
  /* group 4PUs as 2*2 */
  for(i=0; i<32; i++) {
    objs[i] = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    for(j=0; j<32; j++)
      if (i==j)
        values[i+32*j] = values[j+32*i] = 1.f;
      else if (i/2==j/2)
        values[i+32*j] = values[j+32*i] = 4.f;
      else
        values[i+32*j] = values[j+32*i] = 8.f;
  }
  assert(!hwloc_distances_add(topology, 32, objs, values,
			      HWLOC_DISTANCES_KIND_MEANS_LATENCY|HWLOC_DISTANCES_KIND_FROM_USER,
			      HWLOC_DISTANCES_FLAG_GROUP));
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 6);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 4);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 8);
  width = hwloc_get_nbobjs_by_depth(topology, 4);
  assert(width == 16);
  width = hwloc_get_nbobjs_by_depth(topology, 5);
  assert(width == 32);
  hwloc_topology_destroy(topology);

  return 0;
}
