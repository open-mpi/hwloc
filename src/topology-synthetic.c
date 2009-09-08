/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <topology.h>
#include <private/private.h>
#include <private/debug.h>

#include <limits.h>
#include <assert.h>

/* Read from DESCRIPTION a series of integers describing a symmetrical
   topology and update `topology->synthetic_description' accordingly.  On
   success, return zero.  */
int
topo_backend_synthetic_init(struct topo_topology *topology, const char *description)
{
  const char *pos, *next_pos;
  unsigned long item, count;
  int i;

  assert(topology->backend_type == TOPO_BACKEND_NONE);

  topology->backend_params.synthetic.type[0] = TOPO_OBJ_SYSTEM;
  topology->backend_params.synthetic.id[0] = 0;

  for (pos = description, count = 1; *pos; pos = next_pos) {
#define TOPO_OBJ_TYPE_UNKNOWN -1
    topo_obj_type_t type = TOPO_OBJ_TYPE_UNKNOWN;

    while (*pos == ' ')
      pos++;

    if (!*pos)
      break;

    if (*pos < '0' || *pos > '9') {
      if (!strncasecmp(pos, "machines", 2))
	type = TOPO_OBJ_MACHINE;
      else if (!strncasecmp(pos, "nodes", 1))
	type = TOPO_OBJ_NODE;
      else if (!strncasecmp(pos, "sockets", 1))
	type = TOPO_OBJ_SOCKET;
      else if (!strncasecmp(pos, "cores", 2))
	type = TOPO_OBJ_CORE;
      else if (!strncasecmp(pos, "caches", 2))
	type = TOPO_OBJ_CACHE;
      else if (!strncasecmp(pos, "procs", 1))
	type = TOPO_OBJ_PROC;
      else if (!strncasecmp(pos, "misc", 2))
	type = TOPO_OBJ_MISC;

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

    assert(count + 1 < TOPO_SYNTHETIC_MAX_DEPTH);
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

  int cache_depth = 0, misc_depth = 0;
  int nb_machine_levels = 0, nb_node_levels = 0;

  for(i=0; i<count; i++) {
    topo_obj_type_t type;

    type = topology->backend_params.synthetic.type[i];

    if (type == TOPO_OBJ_TYPE_UNKNOWN) {
      switch (count-1-i) {
      case 0: type = TOPO_OBJ_PROC; break;
      case 1: type = TOPO_OBJ_CORE; break;
      case 2: type = TOPO_OBJ_CACHE; break;
      case 3: type = TOPO_OBJ_SOCKET; break;
      case 4: type = TOPO_OBJ_NODE; break;
      case 5: type = TOPO_OBJ_MACHINE; break;
      default: type = TOPO_OBJ_MISC; break;
      }
      topology->backend_params.synthetic.type[i] = type;
    }
    switch (type) {
      case TOPO_OBJ_CACHE:
	cache_depth++;
	break;
      case TOPO_OBJ_MISC:
	misc_depth++;
	break;
      case TOPO_OBJ_NODE:
	if (nb_node_levels) {
	    fprintf(stderr,"synthetic string can not have several NUMA node levels\n");
	    return -1;
	}
	nb_node_levels++;
	break;
      case TOPO_OBJ_MACHINE:
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
    topo_obj_type_t type = topology->backend_params.synthetic.type[i];

    if (type == TOPO_OBJ_MISC)
      topology->backend_params.synthetic.depth[i] = misc_depth--;
    else if (type == TOPO_OBJ_CACHE)
      topology->backend_params.synthetic.depth[i] = cache_depth--;
  }

  /* last level must be PROC */
  if (topology->backend_params.synthetic.type[count-1] != TOPO_OBJ_PROC) {
    fprintf(stderr,"synthetic string needs to have a number of processors\n");
    return -1;
  }

  topology->backend_type = TOPO_BACKEND_SYNTHETIC;
  topology->backend_params.synthetic.arity[count-1] = 0;
  topology->is_fake = 1;

  return 0;
}

void
topo_backend_synthetic_exit(struct topo_topology *topology)
{
  assert(topology->backend_type == TOPO_BACKEND_SYNTHETIC);
  topology->backend_type = TOPO_BACKEND_NONE;
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
topo__look_synthetic(struct topo_topology *topology,
    int level, unsigned first_cpu,
    unsigned long *parent_memory_kB,
    topo_cpuset_t *parent_cpuset)
{
  unsigned long my_memory = 0, *pmemory = parent_memory_kB;
  topo_obj_t obj;
  unsigned i;
  topo_obj_type_t type = topology->backend_params.synthetic.type[level];

  /* pre-hooks */
  switch (type) {
    case TOPO_OBJ_MISC:
      break;
    case TOPO_OBJ_SYSTEM:
      /* Shouldn't happen.  */
      abort();
      break;
    case TOPO_OBJ_MACHINE:
      /* Gather memory size from memory nodes for this machine */
      pmemory = &my_memory;
      break;
    case TOPO_OBJ_NODE:
      break;
    case TOPO_OBJ_SOCKET:
      break;
    case TOPO_OBJ_CACHE:
      break;
    case TOPO_OBJ_CORE:
      break;
    case TOPO_OBJ_PROC:
      break;
  }

  obj = topo_alloc_setup_object(type, topology->backend_params.synthetic.id[level]++);

  if (type == TOPO_OBJ_PROC) {
    assert(topology->backend_params.synthetic.arity[level] == 0);
    topo_cpuset_set(&obj->cpuset, first_cpu++);
  } else {
    assert(topology->backend_params.synthetic.arity[level] != 0);
    for (i = 0; i < topology->backend_params.synthetic.arity[level]; i++)
      first_cpu = topo__look_synthetic(topology, level + 1, first_cpu, pmemory, &obj->cpuset);
  }

  topo_add_object(topology, obj);

  topo_cpuset_orset(parent_cpuset, &obj->cpuset);

  /* post-hooks */
  switch (type) {
    case TOPO_OBJ_MISC:
      obj->attr->misc.depth = topology->backend_params.synthetic.depth[level];
      break;
    case TOPO_OBJ_SYSTEM:
      abort();
      break;
    case TOPO_OBJ_MACHINE:
      obj->attr->machine.memory_kB = my_memory;
      obj->attr->machine.dmi_board_vendor = NULL;
      obj->attr->machine.dmi_board_name = NULL;
      obj->attr->machine.huge_page_size_kB = 0;
      obj->attr->machine.huge_page_free = 0;
      *parent_memory_kB += obj->attr->machine.memory_kB;
      break;
    case TOPO_OBJ_NODE:
      /* 1GB in memory nodes.  */
      obj->attr->node.memory_kB = 1024*1024;
      *parent_memory_kB += obj->attr->node.memory_kB;
      obj->attr->node.huge_page_free = 0;
      break;
    case TOPO_OBJ_SOCKET:
      break;
    case TOPO_OBJ_CACHE:
      obj->attr->cache.depth = topology->backend_params.synthetic.depth[level];
      if (obj->attr->cache.depth == 1)
	/* 32Kb in L1 */
	obj->attr->cache.memory_kB = 32*1024;
      else
	/* *4 at each level, starting from 1MB for L2 */
	obj->attr->cache.memory_kB = 256*1024 << (2*obj->attr->cache.depth);
      break;
    case TOPO_OBJ_CORE:
      break;
    case TOPO_OBJ_PROC:
      break;
  }

  return first_cpu;
}

static int
topo_synthetic_set_cpubind(void) {
  return 0;
}

void
topo_look_synthetic(struct topo_topology *topology)
{
  topo_cpuset_t cpuset;
  unsigned first_cpu = 0, i;

  topology->set_cpubind = (void*) topo_synthetic_set_cpubind;
  topology->set_thisproc_cpubind = (void*) topo_synthetic_set_cpubind;
  topology->set_thisthread_cpubind = (void*) topo_synthetic_set_cpubind;
  topology->set_proc_cpubind = (void*) topo_synthetic_set_cpubind;
  topology->set_thread_cpubind = (void*) topo_synthetic_set_cpubind;

  for (i = 0; i < topology->backend_params.synthetic.arity[0]; i++)
    first_cpu = topo__look_synthetic(topology, 1, first_cpu, &topology->levels[0][0]->attr->system.memory_kB, &cpuset);
}
