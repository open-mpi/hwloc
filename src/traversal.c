/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <assert.h>

int
hwloc_get_type_depth (struct hwloc_topology *topology, hwloc_obj_type_t type)
{
  return topology->type_depth[type];
}

hwloc_obj_type_t
hwloc_get_depth_type (hwloc_topology_t topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return HWLOC_OBJ_TYPE_MAX; /* FIXME: add an invalid value ? */
  return topology->levels[depth][0]->type;
}

unsigned
hwloc_get_nbobjs_by_depth (struct hwloc_topology *topology, unsigned depth)
{
  if (depth >= topology->nb_levels)
    return 0;
  return topology->level_nbobjects[depth];
}

struct hwloc_obj *
hwloc_get_obj_by_depth (struct hwloc_topology *topology, unsigned depth, unsigned idx)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (idx >= topology->level_nbobjects[depth])
    return NULL;
  return topology->levels[depth][idx];
}

int hwloc_get_closest_objs (struct hwloc_topology *topology, struct hwloc_obj *src, struct hwloc_obj **objs, int max)
{
  struct hwloc_obj *parent, *nextparent, **src_objs;
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
      if (!hwloc_cpuset_isequal(parent->cpuset, nextparent->cpuset))
	break;
      parent = nextparent;
    }

    /* traverse src's objects and find those that are in nextparent and were not in parent */
    for(i=0; i<src_nbobjects; i++) {
      if (hwloc_cpuset_isincluded(src_objs[i]->cpuset, nextparent->cpuset)
	  && !hwloc_cpuset_isincluded(src_objs[i]->cpuset, parent->cpuset)) {
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
hwloc__get_largest_objs_inside_cpuset (struct hwloc_obj *current, hwloc_cpuset_t set,
				       struct hwloc_obj ***res, int *max)
{
  int gotten = 0;
  int i;

  /* the caller must ensure this */
  assert(*max > 0);

  if (hwloc_cpuset_isequal(current->cpuset, set)) {
    **res = current;
    (*res)++;
    (*max)--;
    return 1;
  }

  for (i=0; i<current->arity; i++) {
    hwloc_cpuset_t subset = hwloc_cpuset_dup(set);
    int ret;

    /* split out the cpuset part corresponding to this child and see if there's anything to do */
    hwloc_cpuset_andset(subset, current->children[i]->cpuset);
    if (hwloc_cpuset_iszero(subset)) {
      free(subset);
      continue;
    }

    ret = hwloc__get_largest_objs_inside_cpuset (current->children[i], subset, res, max);
    gotten += ret;
    free(subset);

    /* if no more room to store remaining objects, return what we got so far */
    if (!*max)
      break;
  }

  return gotten;
}

int
hwloc_get_largest_objs_inside_cpuset (struct hwloc_topology *topology, hwloc_cpuset_t set,
				      struct hwloc_obj **objs, int max)
{
  struct hwloc_obj *current = topology->levels[0][0];

  if (!hwloc_cpuset_isincluded(set, current->cpuset))
    return -1;

  if (max <= 0)
    return 0;

  return hwloc__get_largest_objs_inside_cpuset (current, set, &objs, &max);
}

const char *
hwloc_obj_type_string (hwloc_obj_type_t obj)
{
  switch (obj)
    {
    case HWLOC_OBJ_SYSTEM: return "System";
    case HWLOC_OBJ_MACHINE: return "Machine";
    case HWLOC_OBJ_MISC: return "Misc";
    case HWLOC_OBJ_NODE: return "NUMANode";
    case HWLOC_OBJ_SOCKET: return "Socket";
    case HWLOC_OBJ_CACHE: return "Cache";
    case HWLOC_OBJ_CORE: return "Core";
    case HWLOC_OBJ_PROC: return "Proc";
    default: return "Unknown";
    }
}

hwloc_obj_type_t
hwloc_obj_type_of_string (const char * string)
{
  if (!strcmp(string, "System")) return HWLOC_OBJ_SYSTEM;
  if (!strcmp(string, "Machine")) return HWLOC_OBJ_MACHINE;
  if (!strcmp(string, "Misc")) return HWLOC_OBJ_MISC;
  if (!strcmp(string, "NUMANode")) return HWLOC_OBJ_NODE;
  if (!strcmp(string, "Socket")) return HWLOC_OBJ_SOCKET;
  if (!strcmp(string, "Cache")) return HWLOC_OBJ_CACHE;
  if (!strcmp(string, "Core")) return HWLOC_OBJ_CORE;
  if (!strcmp(string, "Proc")) return HWLOC_OBJ_PROC;
  return HWLOC_OBJ_TYPE_MAX;
}

#define hwloc_memory_size_printf_value(_size) \
  (_size) < (10*1024) ? (_size) : (_size) < (10*1024*1024) ? (_size)>>10 : (_size)>>20
#define hwloc_memory_size_printf_unit(_size) \
  (_size) < (10*1024) ? "KB" : (_size) < (10*1024*1024) ? "MB" : "GB"

int
hwloc_obj_snprintf(char *string, size_t size,
		  struct hwloc_topology *topology, struct hwloc_obj *l, const char *_indexprefix, int verbose)
{
  hwloc_obj_type_t type = l->type;
  const char *indexprefix = _indexprefix ? _indexprefix : "#";
  char os_index[12] = "";

  if (l->os_index != -1)
    snprintf(os_index, 12, "%s%d", indexprefix, l->os_index);

  switch (type) {
  case HWLOC_OBJ_SOCKET:
  case HWLOC_OBJ_CORE:
    return hwloc_snprintf(string, size, "%s%s", hwloc_obj_type_string(type), os_index);
  case HWLOC_OBJ_MISC:
	  /* TODO: more pretty presentation? */
    return hwloc_snprintf(string, size, "%s%u%s", hwloc_obj_type_string(type), l->attr->misc.depth, os_index);
  case HWLOC_OBJ_PROC:
    return hwloc_snprintf(string, size, "P%s", os_index);
  case HWLOC_OBJ_SYSTEM:
    if (verbose)
      return hwloc_snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->system.memory_kB),
		      hwloc_memory_size_printf_unit(l->attr->system.memory_kB),
		      l->attr->system.huge_page_free, l->attr->system.huge_page_size_kB,
		      l->attr->system.dmi_board_vendor?l->attr->system.dmi_board_vendor:"",
		      l->attr->system.dmi_board_name?l->attr->system.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%s(%lu%s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->system.memory_kB),
		      hwloc_memory_size_printf_unit(l->attr->system.memory_kB));
  case HWLOC_OBJ_MACHINE:
    if (verbose)
      return hwloc_snprintf(string, size, "%s(%lu%s HP=%lu*%lukB %s %s)", hwloc_obj_type_string(type),
		      hwloc_memory_size_printf_value(l->attr->machine.memory_kB),
		      hwloc_memory_size_printf_unit(l->attr->machine.memory_kB),
		      l->attr->machine.huge_page_free, l->attr->machine.huge_page_size_kB,
		      l->attr->machine.dmi_board_vendor?l->attr->machine.dmi_board_vendor:"",
		      l->attr->machine.dmi_board_name?l->attr->machine.dmi_board_name:"");
    else
      return hwloc_snprintf(string, size, "%s%s(%lu%s)", hwloc_obj_type_string(type), os_index,
		      hwloc_memory_size_printf_value(l->attr->machine.memory_kB),
		      hwloc_memory_size_printf_unit(l->attr->machine.memory_kB));
  case HWLOC_OBJ_NODE:
    return hwloc_snprintf(string, size, "%s%s(%lu%s)",
		    verbose ? hwloc_obj_type_string(type) : "Node", os_index,
		    hwloc_memory_size_printf_value(l->attr->node.memory_kB),
		    hwloc_memory_size_printf_unit(l->attr->node.memory_kB));
  case HWLOC_OBJ_CACHE:
    return hwloc_snprintf(string, size, "L%u%s%s(%lu%s)", l->attr->cache.depth,
		      verbose ? hwloc_obj_type_string(type) : "", os_index,
		    hwloc_memory_size_printf_value(l->attr->node.memory_kB),
		    hwloc_memory_size_printf_unit(l->attr->node.memory_kB));
  default:
    *string = '\0';
    return 0;
  }
}

int hwloc_obj_cpuset_snprintf(char *str, size_t size, size_t nobj, struct hwloc_obj * const *objs)
{
  hwloc_cpuset_t set = hwloc_cpuset_alloc();
  int res;
  int i;

  hwloc_cpuset_zero(set);
  for(i=0; i<nobj; i++)
    hwloc_cpuset_orset(set, objs[i]->cpuset);

  res = hwloc_cpuset_snprintf(str, size, set);
  free(set);
  return res;
}
