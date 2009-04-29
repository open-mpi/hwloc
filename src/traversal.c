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
  return lt_cpuset_isincluded(&subtree_root->cpuset, &level->cpuset);
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


const char *
topo_level_string (enum topo_level_type_e l)
{
  switch (l)
    {
    case TOPO_LEVEL_MACHINE: return "Machine";
    case TOPO_LEVEL_FAKE: return "Fake";
    case TOPO_LEVEL_NODE: return "NUMANode";
    case TOPO_LEVEL_DIE: return "Die";
    case TOPO_LEVEL_L3: return "L3Cache";
    case TOPO_LEVEL_L2: return "L2Cache";
    case TOPO_LEVEL_CORE: return "Core";
    case TOPO_LEVEL_L1: return "L1Cache";
    case TOPO_LEVEL_PROC: return "SMTproc";
    default: return "Unknown";
    }
}

static void
topo_print_level_description (struct topo_level *l, FILE *output, int verbose_mode)
{
  unsigned long type = l->type;
  const char * separator = " + ";
  const char * current_separator = ""; /* not prefix for the first one */

#define topo_print_level_description_level(_l) \
    {									\
      if (type == _l) \
	{	      \
	  fprintf(output, "%s%s", current_separator, topo_level_string(_l)); \
	  current_separator = separator;				\
	}								\
    }

  topo_print_level_description_level(TOPO_LEVEL_MACHINE)
  topo_print_level_description_level(TOPO_LEVEL_FAKE)
  topo_print_level_description_level(TOPO_LEVEL_NODE)
  topo_print_level_description_level(TOPO_LEVEL_DIE)
  topo_print_level_description_level(TOPO_LEVEL_L3)
  topo_print_level_description_level(TOPO_LEVEL_L2)
  topo_print_level_description_level(TOPO_LEVEL_CORE)
  topo_print_level_description_level(TOPO_LEVEL_L1)
  topo_print_level_description_level(TOPO_LEVEL_PROC)
}

#define topo_memory_size_printf_value(_size) \
  (_size) < (10*1024) ? (_size) : (_size) < (10*1024*1024) ? (_size)>>10 : (_size)>>20
#define topo_memory_size_printf_unit(_size) \
  (_size) < (10*1024) ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

void
topo_print_level (struct topo_topology *topology, struct topo_level *l, FILE *output, int verbose_mode, const char *separator,
		  const char *indexprefix, const char* labelseparator, const char* levelterm)
{
  enum topo_level_type_e type = l->type;
  topo_print_level_description(l, output, verbose_mode);
  fprintf(output, "%s", labelseparator);
  switch (type) {
  case TOPO_LEVEL_DIE:
  case TOPO_LEVEL_CORE:
  case TOPO_LEVEL_PROC:
    fprintf(output, "%s%s%s%u", separator, topo_level_string(type), indexprefix, l->physical_index[type]);
    break;
  case TOPO_LEVEL_MACHINE:
    fprintf(output, "%s%s(%ld%s)", separator, topo_level_string(type),
	    topo_memory_size_printf_value(l->memory_kB[TOPO_LEVEL_MEMORY_MACHINE]),
	    topo_memory_size_printf_unit(l->memory_kB[TOPO_LEVEL_MEMORY_MACHINE]));
    break;
  case TOPO_LEVEL_NODE:
  case TOPO_LEVEL_L3:
  case TOPO_LEVEL_L2:
  case TOPO_LEVEL_L1: {
    enum topo_level_memory_type_e mtype =
      (type == TOPO_LEVEL_NODE) ? TOPO_LEVEL_MEMORY_NODE
      : (type == TOPO_LEVEL_L3) ? TOPO_LEVEL_MEMORY_L3
      : (type == TOPO_LEVEL_L2) ? TOPO_LEVEL_MEMORY_L2
      : TOPO_LEVEL_MEMORY_L1;
    unsigned long memory_kB = l->memory_kB[mtype];
    fprintf(output, "%s%s%s%u(%ld%s)", separator, topo_level_string(type), indexprefix, l->physical_index[type],
	    topo_memory_size_printf_value(memory_kB),
	    topo_memory_size_printf_unit(memory_kB));
    break;
  }
  default:
    break;
  }
  if (l->level == topology->nb_levels-1) {
    fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  fprintf(output, "%s", levelterm);
}
