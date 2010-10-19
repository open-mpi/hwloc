/*
 * Copyright © 2010 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
  hwloc_topology_t topology;
  unsigned nbobjs;
  const struct hwloc_distances_s *distances;
  float d1, d2;
  unsigned depth, topodepth;
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  topodepth = hwloc_topology_get_depth(topology);

  for(depth=0; depth<topodepth; depth++) {
    unsigned i, j;
    hwloc_obj_t obj1, obj2;

    distances = hwloc_get_whole_distance_matrix(topology, depth);
    if (!distances || !distances->latency) {
      printf("No distance at depth %u\n", depth);
      continue;
    }
    nbobjs = distances->nbobjs;

    /* column header */
    printf("distance matrix for depth %u:\n     ", depth);
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

    obj1 = hwloc_get_obj_by_depth(topology, depth, 0);
    obj2 = hwloc_get_obj_by_depth(topology, depth, nbobjs-1);
    err = hwloc_get_latency(topology, obj1, obj2, &d1, &d2);
    assert(!err);
    assert(d1 == distances->latency[0*nbobjs+(nbobjs-1)]);
    assert(d2 == distances->latency[(nbobjs-1)*nbobjs+0]);
  }

  hwloc_topology_destroy(topology);

  return 0;
}
