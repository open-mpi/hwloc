/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 
 * - get the last object of the last level
 * - get all closest objects
 * - get the common ancestor of last level and its less close object.
 * - check that the ancestor is the machine level
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info info;
  topo_obj_t last;
  topo_obj_t *closest;
  int found;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "2 3 4 5");

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  err = topo_topology_get_info(topology, &info);
  if (err)
    return EXIT_FAILURE;

  /* get the last object of last level */
  last = topo_get_object(topology, info.depth-1, info.nb_processors-1);

  /* allocate the array of closest objects */
  closest = malloc(info.nb_processors * sizeof(*closest));
  assert(closest);

  /* get closest levels */
  found = topo_find_closest_objects (topology, last, closest, info.nb_processors);
  printf("looked for %d closest entries, found %d\n", info.nb_processors, found);
  assert(found == info.nb_processors-1);
  assert(found);

  /* get ancestor of last and less close object */
  topo_obj_t ancestor = topo_find_common_ancestor_object (last, closest[found-1]);
  assert(topo_object_is_in_subtree(ancestor, last));
  assert(topo_object_is_in_subtree(ancestor, closest[found-1]));
  assert(ancestor == topo_get_machine_object (topology));
  printf("ancestor type %u depth %u number %u is machine level\n",
	 ancestor->type, ancestor->level, ancestor->number);

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
