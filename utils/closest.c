/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COUNT 32

int
main (int argc, char *argv[])
{
  lt_topo_t *topology;
  lt_level_t first;
  lt_level_t closest[COUNT];
  int found, i;
  int err;

  err = lt_topo_init (&topology, NULL);
  if (!err)
    return EXIT_FAILURE;

  /* get the last first object */
  first = &topology->levels[topology->nb_levels-1][topology->level_nbitems[topology->nb_levels-1]-1];

  found = lt_find_closest(topology, first, closest, COUNT);
  printf("looked for %d closest entries, found %d\n", COUNT, found);
  for(i=0; i<found; i++)
    printf("close to type %d number %d\n",
	   closest[i]->type, closest[i]->number);

  lt_topo_fini (topology);

  return EXIT_SUCCESS;
}
