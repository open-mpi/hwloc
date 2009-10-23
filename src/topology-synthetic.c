/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <limits.h>
#include <assert.h>
#include <strings.h>

/* Read from DESCRIPTION a series of integers describing a symmetrical
   topology and update `topology->synthetic_description' accordingly.  On
   success, return zero.  */
int
hwloc_backend_synthetic_init(struct hwloc_topology *topology, const char *description)
{
  const char *pos, *next_pos;
  unsigned long item, count;
  int i;
  int cache_depth = 0, misc_depth = 0;
  int nb_machine_levels = 0, nb_node_levels = 0;

  assert(topology->backend_type == HWLOC_BACKEND_NONE);

  topology->backend_params.synthetic.type[0] = HWLOC_OBJ_SYSTEM;
  topology->backend_params.synthetic.id[0] = 0;

  for (pos = description, count = 1; *pos; pos = next_pos) {
#define HWLOC_OBJ_TYPE_UNKNOWN -1
    hwloc_obj_type_t type = HWLOC_OBJ_TYPE_UNKNOWN;

    while (*pos == ' ')
      pos++;

    if (!*pos)
      break;

    if (*pos < '0' || *pos > '9') {
      if (!hwloc_strncasecmp(pos, "machines", 2))
	type = HWLOC_OBJ_MACHINE;
      else if (!hwloc_strncasecmp(pos, "nodes", 1))
	type = HWLOC_OBJ_NODE;
      else if (!hwloc_strncasecmp(pos, "sockets", 1))
	type = HWLOC_OBJ_SOCKET;
      else if (!hwloc_strncasecmp(pos, "cores", 2))
	type = HWLOC_OBJ_CORE;
      else if (!hwloc_strncasecmp(pos, "caches", 2))
	type = HWLOC_OBJ_CACHE;
      else if (!hwloc_strncasecmp(pos, "procs", 1))
	type = HWLOC_OBJ_PROC;
      else if (!hwloc_strncasecmp(pos, "misc", 2))
	type = HWLOC_OBJ_MISC;

      next_pos = strchr(pos, ':');
      if (!next_pos) {
	fprintf(stderr,"synthetic string doesn't have a `:' after object type at '%s'\n", pos);
	return -1;
      }
      pos = next_pos + 1;
    }
    item = strtoul(pos, (char **)&next_pos, 0);
    if (next_pos == pos) {
      fprintf(stderr,"synthetic string doesn't have a number of objects at '%s'\n", pos);
      return -1;
    }

    assert(count + 1 < HWLOC_SYNTHETIC_MAX_DEPTH);
    assert(item <= UINT_MAX);

    topology->backend_params.synthetic.arity[count-1] = (unsigned)item;
    topology->backend_params.synthetic.type[count] = type;
    topology->backend_params.synthetic.id[count] = 0;
    count++;
  }

  if (count <= 0) {
    fprintf(stderr,"synthetic string doesn't contain any object\n");
    return -1;
  }

  for(i=0; i<count; i++) {
    hwloc_obj_type_t type;

    type = topology->backend_params.synthetic.type[i];

    if (type == HWLOC_OBJ_TYPE_UNKNOWN) {
      switch (count-1-i) {
      case 0: type = HWLOC_OBJ_PROC; break;
      case 1: type = HWLOC_OBJ_CORE; break;
      case 2: type = HWLOC_OBJ_CACHE; break;
      case 3: type = HWLOC_OBJ_SOCKET; break;
      case 4: type = HWLOC_OBJ_NODE; break;
      case 5: type = HWLOC_OBJ_MACHINE; break;
      default: type = HWLOC_OBJ_MISC; break;
      }
      topology->backend_params.synthetic.type[i] = type;
    }
    switch (type) {
      case HWLOC_OBJ_CACHE:
	cache_depth++;
	break;
      case HWLOC_OBJ_MISC:
	misc_depth++;
	break;
      case HWLOC_OBJ_NODE:
	if (nb_node_levels) {
	    fprintf(stderr,"synthetic string can not have several NUMA node levels\n");
	    return -1;
	}
	nb_node_levels++;
	break;
      case HWLOC_OBJ_MACHINE:
	if (nb_machine_levels) {
	    fprintf(stderr,"synthetic string can not have several machine levels\n");
	    return -1;
	}
	nb_machine_levels++;
	break;
      default:
	break;
    }
  }

  if (cache_depth == 1)
    /* if there is a single cache level, make it L2 */
    cache_depth = 2;

  for (i=0; i<count; i++) {
    hwloc_obj_type_t type = topology->backend_params.synthetic.type[i];

    if (type == HWLOC_OBJ_MISC)
      topology->backend_params.synthetic.depth[i] = misc_depth--;
    else if (type == HWLOC_OBJ_CACHE)
      topology->backend_params.synthetic.depth[i] = cache_depth--;
  }

  /* last level must be PROC */
  if (topology->backend_params.synthetic.type[count-1] != HWLOC_OBJ_PROC) {
    fprintf(stderr,"synthetic string needs to have a number of processors\n");
    return -1;
  }

  topology->backend_type = HWLOC_BACKEND_SYNTHETIC;
  topology->backend_params.synthetic.arity[count-1] = 0;
  topology->is_thissystem = 0;

  return 0;
}

void
hwloc_backend_synthetic_exit(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_SYNTHETIC);
  topology->backend_type = HWLOC_BACKEND_NONE;
}

/*
 * Recursively build objects whose cpu start at first_cpu
 * - level gives where to look in the type, arity and id arrays
 * - the id array is used as a variable to get unique IDs for a given level.
 * - generated memory should be added to *memory_kB.
 * - generated cpus should be added to parent_cpuset.
 * - next cpu number to be used should be returned.
 */
static unsigned
hwloc__look_synthetic(struct hwloc_topology *topology,
    int level, unsigned first_cpu,
    unsigned long *parent_memory_kB,
    hwloc_cpuset_t parent_cpuset)
{
  unsigned long my_memory = 0, *pmemory = parent_memory_kB;
  hwloc_obj_t obj;
  unsigned i;
  hwloc_obj_type_t type = topology->backend_params.synthetic.type[level];

  /* pre-hooks */
  switch (type) {
    case HWLOC_OBJ_MISC:
      break;
    case HWLOC_OBJ_SYSTEM:
      /* Shouldn't happen.  */
      abort();
      break;
    case HWLOC_OBJ_MACHINE:
      /* Gather memory size from memory nodes for this machine */
      pmemory = &my_memory;
      break;
    case HWLOC_OBJ_NODE:
      break;
    case HWLOC_OBJ_SOCKET:
      break;
    case HWLOC_OBJ_CACHE:
      break;
    case HWLOC_OBJ_CORE:
      break;
    case HWLOC_OBJ_PROC:
      break;
  }

  obj = hwloc_alloc_setup_object(type, topology->backend_params.synthetic.id[level]++);
  obj->cpuset = hwloc_cpuset_alloc();

  if (type == HWLOC_OBJ_PROC) {
    assert(topology->backend_params.synthetic.arity[level] == 0);
    hwloc_cpuset_set(obj->cpuset, first_cpu++);
  } else {
    assert(topology->backend_params.synthetic.arity[level] != 0);
    for (i = 0; i < topology->backend_params.synthetic.arity[level]; i++)
      first_cpu = hwloc__look_synthetic(topology, level + 1, first_cpu, pmemory, obj->cpuset);
  }

  hwloc_add_object(topology, obj);

  hwloc_cpuset_orset(parent_cpuset, obj->cpuset);

  /* post-hooks */
  switch (type) {
    case HWLOC_OBJ_MISC:
      obj->attr->misc.depth = topology->backend_params.synthetic.depth[level];
      break;
    case HWLOC_OBJ_SYSTEM:
      abort();
      break;
    case HWLOC_OBJ_MACHINE:
      obj->attr->machine.memory_kB = my_memory;
      obj->attr->machine.dmi_board_vendor = NULL;
      obj->attr->machine.dmi_board_name = NULL;
      obj->attr->machine.huge_page_size_kB = 0;
      obj->attr->machine.huge_page_free = 0;
      *parent_memory_kB += obj->attr->machine.memory_kB;
      break;
    case HWLOC_OBJ_NODE:
      /* 1GB in memory nodes.  */
      obj->attr->node.memory_kB = 1024*1024;
      *parent_memory_kB += obj->attr->node.memory_kB;
      obj->attr->node.huge_page_free = 0;
      break;
    case HWLOC_OBJ_SOCKET:
      break;
    case HWLOC_OBJ_CACHE:
      obj->attr->cache.depth = topology->backend_params.synthetic.depth[level];
      if (obj->attr->cache.depth == 1)
	/* 32Kb in L1 */
	obj->attr->cache.memory_kB = 32*1024;
      else
	/* *4 at each level, starting from 1MB for L2 */
	obj->attr->cache.memory_kB = 256*1024 << (2*obj->attr->cache.depth);
      break;
    case HWLOC_OBJ_CORE:
      break;
    case HWLOC_OBJ_PROC:
      break;
  }

  return first_cpu;
}

void
hwloc_look_synthetic(struct hwloc_topology *topology)
{
  hwloc_cpuset_t cpuset = hwloc_cpuset_alloc();
  unsigned first_cpu = 0, i;

  for (i = 0; i < topology->backend_params.synthetic.arity[0]; i++)
    first_cpu = hwloc__look_synthetic(topology, 1, first_cpu, &topology->levels[0][0]->attr->system.memory_kB, cpuset);

  free(cpuset);
}
