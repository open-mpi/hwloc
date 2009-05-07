/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/private.h>
#include <libtopology/debug.h>

unsigned
topo_get_type_depth (struct topo_topology *topology, enum topo_level_type_e type)
{
  return topology->type_depth[type];
}

enum topo_level_type_e
topo_get_depth_type (topo_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return TOPO_LEVEL_MAX; /* FIXME: add an invalid value ? */
  return topology->levels[depth][0].type;
}

unsigned
topo_get_depth_nbitems (struct topo_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbitems[depth];
}

struct topo_level *
topo_get_level (struct topo_topology *topology, unsigned depth, unsigned index)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (index >= topology->level_nbitems[depth])
    return NULL;
  return &topology->levels[depth][index];
}

struct topo_level * topo_find_common_ancestor_level (struct topo_level *lvl1, struct topo_level *lvl2)
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

int topo_level_is_in_subtree (topo_level_t subtree_root, topo_level_t level)
{
  return topo_cpuset_isincluded(&subtree_root->cpuset, &level->cpuset);
}

int topo_find_closest_levels (struct topo_topology *topology, struct topo_level *src, struct topo_level **lvls, int max)
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
      if (!topo_cpuset_isequal(&parent->cpuset, &nextparent->cpuset))
	break;
      parent = nextparent;
    }

    /* traverse src's level and find objects that are in nextparent and were not in parent */
    for(i=0; i<src_nbitems; i++) {
      if (topo_cpuset_isincluded(&nextparent->cpuset, &src_levels[i].cpuset)
	  && !topo_cpuset_isincluded(&parent->cpuset, &src_levels[i].cpuset)) {
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


const char *
topo_level_string (enum topo_level_type_e l)
{
  switch (l)
    {
    case TOPO_LEVEL_MACHINE: return "Machine";
    case TOPO_LEVEL_FAKE: return "Fake";
    case TOPO_LEVEL_NODE: return "NUMANode";
    case TOPO_LEVEL_DIE: return "Die";
    case TOPO_LEVEL_CACHE: return "Cache";
    case TOPO_LEVEL_CORE: return "Core";
    case TOPO_LEVEL_PROC: return "SMTproc";
    default: return "Unknown";
    }
}

#define topo_memory_size_printf_value(_size) \
  (_size) < (10*1024) ? (_size) : (_size) < (10*1024*1024) ? (_size)>>10 : (_size)>>20
#define topo_memory_size_printf_unit(_size) \
  (_size) < (10*1024) ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

void
topo_print_level (struct topo_topology *topology, struct topo_level *l, FILE *output, int verbose_mode,
		  const char *indexprefix, const char* levelterm)
{
  enum topo_level_type_e type = l->type;
  char physical_index[12] = "";

  if (l->physical_index != -1)
    snprintf(physical_index, 12, "%s%d", indexprefix, l->physical_index);

  switch (type) {
  case TOPO_LEVEL_DIE:
  case TOPO_LEVEL_CORE:
  case TOPO_LEVEL_PROC:
    fprintf(output, "%s%s", topo_level_string(type), physical_index);
    break;
  case TOPO_LEVEL_MACHINE:
    fprintf(output, "%s(%ld%s)", topo_level_string(type),
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  case TOPO_LEVEL_NODE: {
    fprintf(output, "%s%s(%ld%s)", topo_level_string(type), physical_index,
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  }
  case TOPO_LEVEL_CACHE: {
    fprintf(output, "L%d%s%s(%ld%s)", l->cache_depth, topo_level_string(type), physical_index,
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  }
  default:
    break;
  }
  fprintf(output, "%s", levelterm);
}
