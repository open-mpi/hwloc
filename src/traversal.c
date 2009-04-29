/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/private.h>
#include <libtopology/debug.h>

unsigned
topo_topology_get_type_depth (struct topo_topology *topology, enum topo_level_type_e type)
{
  return topology->type_depth[type];
}

unsigned
topo_topology_get_depth_nbitems (struct topo_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbitems[depth];
}

struct topo_level *
topo_topology_get_level(struct topo_topology *topology, unsigned depth, unsigned index)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (index >= topology->level_nbitems[depth])
    return NULL;
  return &topology->levels[depth][index];
}

enum topo_level_type_e
topo_topology_get_depth_type (topo_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return TOPO_LEVEL_MAX; /* FIXME: add an invalid value ? */
  return topology->levels[depth][0].type;
}

struct topo_level * lt_find_common_ancestor (struct topo_level *lvl1, struct topo_level *lvl2)
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

int lt_is_in_subtree (topo_level_t subtree_root, topo_level_t level)
{
  return lt_cpuset_isincluded(&subtree_root->cpuset, &level->cpuset);
}

int lt_find_closest(struct topo_topology *topology, struct topo_level *src, struct topo_level **lvls, int max)
{
  struct topo_level *parent, *nextparent, *src_levels;
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
