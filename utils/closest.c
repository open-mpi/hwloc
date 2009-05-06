/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COUNT 32

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info info;
  topo_level_t first;
  topo_level_t closest[COUNT];
  int found, i;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;
  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  err = topo_topology_get_info(topology, &info);

  /* get the last first object */
  first = topo_get_level(topology, info.depth-1, info.nb_processors-1);

  found = topo_find_closest_levels (topology, first, closest, COUNT);
  printf("looked for %d closest entries, found %d\n", COUNT, found);
  for(i=0; i<found; i++)
    printf("close to type %d number %d physical number %d\n",
	   closest[i]->type, closest[i]->number, closest[i]->physical_index);

  if (found) {
    topo_level_t ancestor = topo_find_common_ancestor_level (first, closest[found-1]);
    assert(topo_level_is_in_subtree(ancestor, first));
    assert(topo_level_is_in_subtree(ancestor, closest[found-1]));
    printf("ancestor type %d number %d\n",
	   ancestor->type, ancestor->number);
  }

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
