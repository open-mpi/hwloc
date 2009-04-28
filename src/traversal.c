/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/private.h>
#include <libtopology/debug.h>

struct lt_level * lt_find_common_ancestor (struct lt_level *lvl1, struct lt_level *lvl2)
{
  while (lvl1->level > lvl2->level)
    lvl1 = lvl1->father;
  while (lvl2->level > lvl1->level)
    lvl2 = lvl2->father;
  while (lvl1 != lvl2) {
    lvl1 = lvl1->father;
    lvl2 = lvl2->father;
  }
  return lvl1;
}

int lt_is_in_subtree (lt_level_t subtree_root, lt_level_t level)
{
  return lt_cpuset_isincluded(&subtree_root->cpuset, &level->cpuset);
}

int lt_find_closest(struct topo_topology *topology, struct lt_level *src, struct lt_level **lvls, int max)
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
