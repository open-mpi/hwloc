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
  lt_level_t first;
  lt_level_t closest[COUNT];
  int found, i;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;
  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  /* get the last first object */
  first = &topology->levels[topology->nb_levels-1][topology->level_nbitems[topology->nb_levels-1]-1];

  found = lt_find_closest(topology, first, closest, COUNT);
  printf("looked for %d closest entries, found %d\n", COUNT, found);
  for(i=0; i<found; i++)
    printf("close to type %d number %d physical number %d\n",
	   closest[i]->type, closest[i]->number, closest[i]->physical_index[closest[i]->type]);

  if (found) {
    lt_level_t ancestor = lt_find_common_ancestor(first, closest[found-1]);
    assert(lt_is_in_subtree(ancestor, first));
    assert(lt_is_in_subtree(ancestor, closest[found-1]));
    printf("ancestor type %d number %d\n",
	   ancestor->type, ancestor->number);
  }

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
