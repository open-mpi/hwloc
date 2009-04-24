/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/debug.h>

int lt_find_closest(lt_topo_t *topology, struct lt_level *src, struct lt_level **lvls, int max)
{
  struct lt_level *parent, *nextparent, *src_levels;
  int i,src_nbitems;
  int stored = 0;

  src_nbitems = topology->level_nbitems[src->level];
  src_levels = topology->levels[src->level];

  parent = src;
  while (stored < max) {
    while (1) {
      nextparent = parent->father;
      if (!nextparent)
	goto out;
      if (!lt_cpuset_isequal(&parent->cpuset, &nextparent->cpuset))
	break;
      parent = nextparent;
    }

    /* traverse src's level and find objects that are in nextparent and were not in parent */
    for(i=0; i<src_nbitems; i++) {
      if (lt_cpuset_isincluded(&nextparent->cpuset, &src_levels[i].cpuset)
	  && !lt_cpuset_isincluded(&parent->cpuset, &src_levels[i].cpuset)) {
	lvls[stored++] = &src_levels[i];
	if (stored == max)
	  goto out;
      }
    }
    parent = nextparent;
  }

 out:
  return stored;
}
