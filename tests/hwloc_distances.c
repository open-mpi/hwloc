/*
 * Copyright © 2010-2011 INRIA.  All rights reserved.
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void print_distances(const struct hwloc_distances_s *distances)
{
  unsigned nbobjs = distances->nbobjs;
  unsigned i, j;

  printf("     ");
  /* column header */
  for(j=0; j<nbobjs; j++)
    printf(" % 5d", (int) j);
  printf("\n");

  /* each line */
  for(i=0; i<nbobjs; i++) {
    /* row header */
    printf("% 5d", (int) i);
    /* each value */
    for(j=0; j<nbobjs; j++)
      printf(" %2.3f", distances->latency[i*nbobjs+j]);
    printf("\n");
  }
}

int main(void)
{
  hwloc_topology_t topology;
  unsigned nbobjs;
  const struct hwloc_distances_s *distances;
  float d1, d2;
  unsigned depth, topodepth;
  int err;
  hwloc_obj_t obj1, obj2;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:4 core:4 pu:1");
  putenv("HWLOC_NUMANode_DISTANCES=0,1,2,3:2*2");
  putenv("HWLOC_PU_DISTANCES=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15:4*2*2");
  hwloc_topology_load(topology);

  topodepth = hwloc_topology_get_depth(topology);

  for(depth=0; depth<topodepth; depth++) {
    distances = hwloc_get_whole_distance_matrix_by_depth(topology, depth);
    if (!distances || !distances->latency) {
      printf("No distance at depth %u\n", depth);
      continue;
    }

    printf("distance matrix for depth %u:\n", depth);
    print_distances(distances);
    nbobjs = distances->nbobjs;

    obj1 = hwloc_get_obj_by_depth(topology, depth, 0);
    obj2 = hwloc_get_obj_by_depth(topology, depth, nbobjs-1);
    err = hwloc_get_latency(topology, obj1, obj2, &d1, &d2);
    assert(!err);
    assert(d1 == distances->latency[0*nbobjs+(nbobjs-1)]);
    assert(d2 == distances->latency[(nbobjs-1)*nbobjs+0]);
  }

  distances = hwloc_get_whole_distance_matrix_by_type(topology, HWLOC_OBJ_NODE);
  if (!distances || !distances->latency) {
    fprintf(stderr, "No NUMA distance matrix!\n");
    return -1;
  }

  printf("distance matrix for NUMA nodes\n");
  print_distances(distances);
  nbobjs = distances->nbobjs;

  obj1 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, 0);
  obj2 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, nbobjs-1);
  err = hwloc_get_latency(topology, obj1, obj2, &d1, &d2);
  assert(!err);
  assert(d1 == distances->latency[0*nbobjs+(nbobjs-1)]);
  assert(d2 == distances->latency[(nbobjs-1)*nbobjs+0]);

  hwloc_topology_destroy(topology);

  return 0;
}
