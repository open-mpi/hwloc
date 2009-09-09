/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <assert.h>

int
topo_get_type_depth (struct topo_topology *topology, topo_obj_type_t type)
{
  return topology->type_depth[type];
}

topo_obj_type_t
topo_get_depth_type (topo_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return TOPO_OBJ_TYPE_MAX; /* FIXME: add an invalid value ? */
  return topology->levels[depth][0]->type;
}

unsigned
topo_get_depth_nbobjs (struct topo_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbobjects[depth];
}

struct topo_obj *
topo_get_obj_by_depth (struct topo_topology *topology, unsigned depth, unsigned index)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (index >= topology->level_nbobjects[depth])
    return NULL;
  return topology->levels[depth][index];
}

int topo_get_closest_objs (struct topo_topology *topology, struct topo_obj *src, struct topo_obj **objs, int max)
{
  struct topo_obj *parent, *nextparent, **src_objs;
  int i,src_nbobjects;
  int stored = 0;

  src_nbobjects = topology->level_nbobjects[src->depth];
  src_objs = topology->levels[src->depth];

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
    for(i=0; i<src_nbobjects; i++) {
      if (topo_cpuset_isincluded(&src_objs[i]->cpuset, &nextparent->cpuset)
	  && !topo_cpuset_isincluded(&src_objs[i]->cpuset, &parent->cpuset)) {
	objs[stored++] = src_objs[i];
	if (stored == max)
	  goto out;
      }
    }
    parent = nextparent;
  }

 out:
  return stored;
}

static int
topo__get_cpuset_objs (struct topo_obj *current, const topo_cpuset_t *set,
		       struct topo_obj ***res, int *max)
{
  int gotten = 0;
  int i;

  /* the caller must ensure this */
  assert(*max > 0);

  if (topo_cpuset_isequal(&current->cpuset, set)) {
    **res = current;
    (*res)++;
    (*max)--;
    return 1;
  }

  for (i=0; i<current->arity; i++) {
    topo_cpuset_t subset = *set;
    int ret;

    /* split out the cpuset part corresponding to this child and see if there's anything to do */
    topo_cpuset_andset(&subset, &current->children[i]->cpuset);
    if (topo_cpuset_iszero(&subset))
      continue;

    ret = topo__get_cpuset_objs (current->children[i], &subset, res, max);
    gotten += ret;

    /* if no more room to store remaining objects, return what we got so far */
    if (!*max)
      break;
  }

  return gotten;
}

int
topo_get_cpuset_objs (struct topo_topology *topology, const topo_cpuset_t *set,
		      struct topo_obj **objs, int max)
{
  struct topo_obj *current = topology->levels[0][0];

  if (!topo_cpuset_isincluded(set, &current->cpuset))
    return -1;

  if (max <= 0)
    return 0;

  return topo__get_cpuset_objs (current, set, &objs, &max);
}

const char *
topo_obj_type_string (topo_obj_type_t obj)
{
  switch (obj)
    {
    case TOPO_OBJ_SYSTEM: return "System";
    case TOPO_OBJ_MACHINE: return "Machine";
    case TOPO_OBJ_MISC: return "Misc";
    case TOPO_OBJ_NODE: return "NUMANode";
    case TOPO_OBJ_SOCKET: return "Socket";
    case TOPO_OBJ_CACHE: return "Cache";
    case TOPO_OBJ_CORE: return "Core";
    case TOPO_OBJ_PROC: return "Proc";
    default: return "Unknown";
    }
}

topo_obj_type_t
topo_obj_type_of_string (const char * string)
{
  if (!strcmp(string, "System")) return TOPO_OBJ_SYSTEM;
  if (!strcmp(string, "Machine")) return TOPO_OBJ_MACHINE;
  if (!strcmp(string, "Misc")) return TOPO_OBJ_MISC;
  if (!strcmp(string, "NUMANode")) return TOPO_OBJ_NODE;
  if (!strcmp(string, "Socket")) return TOPO_OBJ_SOCKET;
  if (!strcmp(string, "Cache")) return TOPO_OBJ_CACHE;
  if (!strcmp(string, "Core")) return TOPO_OBJ_CORE;
  if (!strcmp(string, "Proc")) return TOPO_OBJ_PROC;
  return TOPO_OBJ_TYPE_MAX;
}

#define topo_memory_size_printf_value(_size) \
  (_size) < (10*1024) ? (_size) : (_size) < (10*1024*1024) ? (_size)>>10 : (_size)>>20
#define topo_memory_size_printf_unit(_size) \
  (_size) < (10*1024) ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

int
topo_obj_snprintf(char *string, size_t size,
		  struct topo_topology *topology, struct topo_obj *l, const char *indexprefix, int verbose)
{
  topo_obj_type_t type = l->type;
  char os_index[12] = "";

  if (l->os_index != -1)
    snprintf(os_index, 12, "%s%d", indexprefix, l->os_index);

  switch (type) {
  case TOPO_OBJ_SOCKET:
  case TOPO_OBJ_CORE:
    return snprintf(string, size, "%s%s", topo_obj_type_string(type), os_index);
  case TOPO_OBJ_MISC:
	  /* TODO: more pretty presentation? */
    return snprintf(string, size, "%s%u%s", topo_obj_type_string(type), l->attr->misc.depth, os_index);
  case TOPO_OBJ_PROC:
    return snprintf(string, size, "P%s", os_index);
  case TOPO_OBJ_SYSTEM:
    if (verbose)
      return snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", topo_obj_type_string(type),
		      topo_memory_size_printf_value(l->attr->system.memory_kB),
		      topo_memory_size_printf_unit(l->attr->system.memory_kB),
		      l->attr->system.huge_page_free, l->attr->system.huge_page_size_kB,
		      l->attr->system.dmi_board_vendor?:"", l->attr->system.dmi_board_name?:"");
    else
      return snprintf(string, size, "%s(%lu%s)", topo_obj_type_string(type),
		      topo_memory_size_printf_value(l->attr->system.memory_kB),
		      topo_memory_size_printf_unit(l->attr->system.memory_kB));
  case TOPO_OBJ_MACHINE:
    if (verbose)
      return snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", topo_obj_type_string(type),
		      topo_memory_size_printf_value(l->attr->machine.memory_kB),
		      topo_memory_size_printf_unit(l->attr->machine.memory_kB),
		      l->attr->machine.huge_page_free, l->attr->machine.huge_page_size_kB,
		      l->attr->machine.dmi_board_vendor?:"", l->attr->machine.dmi_board_name?:"");
    else
      return snprintf(string, size, "%s%s(%lu%s)", topo_obj_type_string(type), os_index,
		      topo_memory_size_printf_value(l->attr->machine.memory_kB),
		      topo_memory_size_printf_unit(l->attr->machine.memory_kB));
  case TOPO_OBJ_NODE:
    return snprintf(string, size, "%s%s(%lu%s)",
		    verbose ? topo_obj_type_string(type) : "Node", os_index,
		    topo_memory_size_printf_value(l->attr->node.memory_kB),
		    topo_memory_size_printf_unit(l->attr->node.memory_kB));
  case TOPO_OBJ_CACHE:
    return snprintf(string, size, "L%u%s%s(%lu%s)", l->attr->cache.depth,
		      verbose ? topo_obj_type_string(type) : "", os_index,
		    topo_memory_size_printf_value(l->attr->node.memory_kB),
		    topo_memory_size_printf_unit(l->attr->node.memory_kB));
  default:
    *string = '\0';
    return 0;
  }
}

int topo_obj_cpuset_snprintf(char *str, size_t size, size_t nobj, struct topo_obj * const *objs)
{
  topo_cpuset_t set;
  int i;

  topo_cpuset_zero(&set);
  for(i=0; i<nobj; i++)
    topo_cpuset_orset(&set, &objs[i]->cpuset);

  return topo_cpuset_snprintf(str, size, &set);
}
