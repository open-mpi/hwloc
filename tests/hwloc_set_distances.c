/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
  hwloc_topology_t topology;
  unsigned indexes[16];
  float distances[16*16];
  unsigned i, j;
  unsigned depth;
  unsigned width;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:2 core:8 pu:1");

  /* default 2*8*1 */
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

  /* 2*8*1 and group 8cores as 2*2*2 */
  for(i=0; i<16; i++) {
    indexes[i] = i;
    for(j=0; j<16; j++)
      if (i==j)
        distances[i+16*j] = distances[j+16*i] = 3;
      else if (i/2==j/2)
        distances[i+16*j] = distances[j+16*i] = 5;
      else if (i/4==j/4)
        distances[i+16*j] = distances[j+16*i] = 7;
      else
        distances[i+16*j] = distances[j+16*i] = 9;
  }
  hwloc_topology_set_distance_matrix(topology, HWLOC_OBJ_CORE, 16, indexes, distances);
  hwloc_topology_load(topology);
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

  /* revert to default 2*8*1 */
  hwloc_topology_set_distance_matrix(topology, HWLOC_OBJ_CORE, 0, NULL, NULL);
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

  /* default 2*4*4 */
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

  /* 2*4*4 and group 4cores as 2*2 */
  putenv("HWLOC_Core_DISTANCES=0,1,2,3,4,5,6,7:4*2");
  hwloc_topology_load(topology);
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

  /* 2*4*4 and group 4cores as 2*2 and 4PUs as 2*2 */
  putenv("HWLOC_PU_DISTANCES=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31:16*2");
  hwloc_topology_load(topology);
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

  /* replace previous core distances with useless ones (grouping as the existing numa nodes) */
  /* 2*4*4 and group 4PUs as 2*2 */
  putenv("HWLOC_Core_DISTANCES=0,1,2,3,4,5,6,7:2*4");
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 5);
  width = hwloc_get_nbobjs_by_depth(topology, 0);
  assert(width == 1);
  width = hwloc_get_nbobjs_by_depth(topology, 1);
  assert(width == 2);
  width = hwloc_get_nbobjs_by_depth(topology, 2);
  assert(width == 8);
  width = hwloc_get_nbobjs_by_depth(topology, 3);
  assert(width == 16);
  width = hwloc_get_nbobjs_by_depth(topology, 4);
  assert(width == 32);

  hwloc_topology_destroy(topology);

  return 0;
}
