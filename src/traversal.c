/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/private.h>
#include <libtopology/debug.h>

unsigned
topo_get_type_depth (struct topo_topology *topology, enum topo_obj_type_e type)
{
  return topology->type_depth[type];
}

enum topo_obj_type_e
topo_get_depth_type (topo_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return TOPO_OBJ_TYPE_MAX; /* FIXME: add an invalid value ? */
  return topology->levels[depth][0].type;
}

unsigned
topo_get_depth_nbitems (struct topo_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbitems[depth];
}

struct topo_obj *
topo_get_object (struct topo_topology *topology, unsigned depth, unsigned index)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (index >= topology->level_nbitems[depth])
    return NULL;
  return &topology->levels[depth][index];
}

struct topo_obj * topo_find_common_ancestor_object (struct topo_obj *obj1, struct topo_obj *obj2)
{
  while (obj1->level > obj2->level)
    obj1 = obj1->father;
  while (obj2->level > obj1->level)
    obj2 = obj2->father;
  while (obj1 != obj2) {
    obj1 = obj1->father;
    obj2 = obj2->father;
  }
  return obj1;
}

int topo_object_is_in_subtree (topo_obj_t subtree_root, topo_obj_t obj)
{
  return topo_cpuset_isincluded(&subtree_root->cpuset, &obj->cpuset);
}

int topo_find_closest_objects (struct topo_topology *topology, struct topo_obj *src, struct topo_obj **objs, int max)
{
  struct topo_obj *parent, *nextparent, *src_objs;
  int i,src_nbitems;
  int stored = 0;

  src_nbitems = topology->level_nbitems[src->level];
  src_objs = topology->levels[src->level];

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

    /* traverse src's objects and find those that are in nextparent and were not in parent */
    for(i=0; i<src_nbitems; i++) {
      if (topo_cpuset_isincluded(&nextparent->cpuset, &src_objs[i].cpuset)
	  && !topo_cpuset_isincluded(&parent->cpuset, &src_objs[i].cpuset)) {
	objs[stored++] = &src_objs[i];
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
topo_object_type_string (enum topo_obj_type_e l)
{
  switch (l)
    {
    case TOPO_OBJ_MACHINE: return "Machine";
    case TOPO_OBJ_FAKE: return "Fake";
    case TOPO_OBJ_NODE: return "NUMANode";
    case TOPO_OBJ_DIE: return "Die";
    case TOPO_OBJ_CACHE: return "Cache";
    case TOPO_OBJ_CORE: return "Core";
    case TOPO_OBJ_PROC: return "SMTproc";
    default: return "Unknown";
    }
}

#define topo_memory_size_printf_value(_size) \
  (_size) < (10*1024) ? (_size) : (_size) < (10*1024*1024) ? (_size)>>10 : (_size)>>20
#define topo_memory_size_printf_unit(_size) \
  (_size) < (10*1024) ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

void
topo_print_object(struct topo_topology *topology, struct topo_obj *l, FILE *output, int verbose_mode,
		  const char *indexprefix, const char* term)
{
  enum topo_obj_type_e type = l->type;
  char physical_index[12] = "";

  if (l->physical_index != -1)
    snprintf(physical_index, 12, "%s%d", indexprefix, l->physical_index);

  switch (type) {
  case TOPO_OBJ_DIE:
  case TOPO_OBJ_CORE:
  case TOPO_OBJ_PROC:
  case TOPO_OBJ_FAKE:
    fprintf(output, "%s%s", topo_object_type_string(type), physical_index);
    break;
  case TOPO_OBJ_MACHINE:
    fprintf(output, "%s(%lu%s)", topo_object_type_string(type),
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  case TOPO_OBJ_NODE: {
    fprintf(output, "%s%s(%lu%s)", topo_object_type_string(type), physical_index,
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  }
  case TOPO_OBJ_CACHE: {
    fprintf(output, "L%u%s%s(%lu%s)", l->cache_depth, topo_object_type_string(type), physical_index,
	    topo_memory_size_printf_value(l->memory_kB),
	    topo_memory_size_printf_unit(l->memory_kB));
    break;
  }
  default:
    break;
  }
  fprintf(output, "%s", term);
}
