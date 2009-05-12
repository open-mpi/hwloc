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
  topo_obj_t first;
  topo_obj_t closest[COUNT];
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
  first = topo_get_object(topology, info.depth-1, info.nb_processors-1);

  found = topo_find_closest_objects (topology, first, closest, COUNT);
  printf("looked for %d closest entries, found %d\n", COUNT, found);
  for(i=0; i<found; i++)
    printf("close to type %u number %u physical number %d\n",
	   closest[i]->type, closest[i]->number, closest[i]->physical_index);

  if (found) {
    topo_obj_t ancestor = topo_find_common_ancestor_object (first, closest[found-1]);
    assert(topo_object_is_in_subtree(ancestor, first));
    assert(topo_object_is_in_subtree(ancestor, closest[found-1]));
    printf("ancestor type %u number %u\n",
	   ancestor->type, ancestor->number);
  }

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
