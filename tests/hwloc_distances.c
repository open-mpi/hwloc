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
  const unsigned *distances;
  unsigned d1, d2;
  unsigned depth, topodepth;
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  topodepth = hwloc_topology_get_depth(topology);

  for(depth=0; depth<topodepth; depth++) {
    unsigned i, j;

    err = hwloc_get_distances(topology, depth, &nbobjs, &distances);
    if (err < 0) {
      printf("No distance at depth %u\n", depth);
      continue;
    }

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
	printf(" % 5d", (int) distances[i+nbobjs*j]);
      printf("\n");
    }

    err = hwloc_get_distance(topology, depth, -1, -1, &d1, &d2);
    assert(err < 0);
    err = hwloc_get_distance(topology, depth, 0, -1, &d1, &d2);
    assert(err < 0);
    err = hwloc_get_distance(topology, depth, 0, nbobjs-1, &d1, &d2);
    assert(!err);
    assert(d1 == distances[0+nbobjs*(nbobjs-1)]);
    assert(d2 == distances[nbobjs-1]);
  }

  hwloc_topology_destroy(topology);

  return 0;
}
