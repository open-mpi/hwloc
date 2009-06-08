/* 
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#include <config.h>
#include <topology.h>
#include <topology/private.h>
#include <topology/debug.h>

#include <assert.h>

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
topo_get_obj (struct topo_topology *topology, unsigned depth, unsigned index)
{
  if (depth >= topology->nb_levels)
    return NULL;
  if (index >= topology->level_nbobjects[depth])
    return NULL;
  return topology->levels[depth][index];
}

int topo_find_closest_objs (struct topo_topology *topology, struct topo_obj *src, struct topo_obj **objs, int max)
{
  struct topo_obj *parent, *nextparent, **src_objs;
  int i,src_nbobjects;
  int stored = 0;

  src_nbobjects = topology->level_nbobjects[src->level];
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

struct topo_obj *
topo_find_cpuset_covering_obj (struct topo_topology *topology, const topo_cpuset_t *set)
{
  struct topo_obj *current = topology->levels[0][0];
  int i;

  if (!topo_cpuset_isincluded(set, &current->cpuset))
    return NULL;

 children:
  for(i=0; i<current->arity; i++) {
    if (topo_cpuset_isincluded(set, &current->children[i]->cpuset)) {
      current = current->children[i];
      goto children;
    }
  }

  return current;
}

static int
topo__find_cpuset_objs (struct topo_obj *current, const topo_cpuset_t *set,
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

    ret = topo__find_cpuset_objs (current->children[i], &subset, res, max);
    gotten += ret;

    /* if no more room to store remaining objects, return what we got so far */
    if (!*max)
      break;
  }

  return gotten;
}

int
topo_find_cpuset_objs (struct topo_topology *topology, const topo_cpuset_t *set,
		       struct topo_obj **objs, int max)
{
  struct topo_obj *current = topology->levels[0][0];

  if (!topo_cpuset_isincluded(set, &current->cpuset))
    return -1;

  if (max <= 0)
    return 0;

  return topo__find_cpuset_objs (current, set, &objs, &max);
}

const char *
topo_obj_type_string (enum topo_obj_type_e l)
{
  switch (l)
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

enum topo_obj_type_e
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
  enum topo_obj_type_e type = l->type;
  char physical_index[12] = "";

  if (l->physical_index != -1)
    snprintf(physical_index, 12, "%s%d", indexprefix, l->physical_index);

  switch (type) {
  case TOPO_OBJ_SOCKET:
  case TOPO_OBJ_CORE:
    return snprintf(string, size, "%s%s", topo_obj_type_string(type), physical_index);
  case TOPO_OBJ_MISC:
	  /* TODO: more pretty presentation? */
    return snprintf(string, size, "%s%u%s", topo_obj_type_string(type), l->attr.misc.depth, physical_index);
  case TOPO_OBJ_PROC:
    return snprintf(string, size, "P%s", physical_index);
  case TOPO_OBJ_SYSTEM:
    return snprintf(string, size, "%s(%lu%s)", topo_obj_type_string(type),
		    topo_memory_size_printf_value(l->attr.system.memory_kB),
		    topo_memory_size_printf_unit(l->attr.system.memory_kB));
  case TOPO_OBJ_MACHINE:
    return snprintf(string, size, "%s%s(%lu%s)", topo_obj_type_string(type), physical_index,
		    topo_memory_size_printf_value(l->attr.machine.memory_kB),
		    topo_memory_size_printf_unit(l->attr.machine.memory_kB));
  case TOPO_OBJ_NODE:
    return snprintf(string, size, "%s%s(%lu%s)",
		    verbose ? topo_obj_type_string(type) : "node", physical_index,
		    topo_memory_size_printf_value(l->attr.node.memory_kB),
		    topo_memory_size_printf_unit(l->attr.node.memory_kB));
  case TOPO_OBJ_CACHE:
    return snprintf(string, size, "L%u%s%s(%lu%s)", l->attr.cache.depth,
		      verbose ? topo_obj_type_string(type) : "", physical_index,
		    topo_memory_size_printf_value(l->attr.node.memory_kB),
		    topo_memory_size_printf_unit(l->attr.node.memory_kB));
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
