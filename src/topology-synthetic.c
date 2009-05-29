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
#include <topology/helper.h>
#include <topology/debug.h>

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

  for (pos = description, count = 0; *pos; pos = next_pos) {
#define TOPO_OBJ_TYPE_UNKNOWN -1
    unsigned type = TOPO_OBJ_TYPE_UNKNOWN;

    while (*pos == ' ')
      pos++;

    if (*pos < '0' || *pos > '9') {
      if (!strncmp(pos, "machines", 1))
	type = TOPO_OBJ_MACHINE;
      else if (!strncmp(pos, "nodes", 1))
	type = TOPO_OBJ_NODE;
      else if (!strncmp(pos, "sockets", 1))
	type = TOPO_OBJ_SOCKET;
      else if (!strncmp(pos, "cores", 2))
	type = TOPO_OBJ_CORE;
      else if (!strncmp(pos, "caches", 2))
	type = TOPO_OBJ_CACHE;
      else if (!strncmp(pos, "procs", 1))
	type = TOPO_OBJ_PROC;
      else if (!strncmp(pos, "fakes", 1))
	type = TOPO_OBJ_FAKE;

      pos = strchr(pos, ':');
      if (!pos)
	return -1;
      pos++;
    }
    item = strtoul(pos, (char **)&next_pos, 0);
    if (next_pos == pos)
      return -1;

    assert(count + 1 < TOPO_SYNTHETIC_MAX_DEPTH);
    assert(item <= UINT_MAX);

    topology->backend_params.synthetic.arity[count] = (unsigned)item;
    topology->backend_params.synthetic.type[count] = type;
    count++;
  }

  if (count <= 0)
    return -1;

  topology->type_depth[TOPO_OBJ_MACHINE] = 0;

  for(i=0; i<count; i++) {
    enum topo_obj_type_e type;

    type = topology->backend_params.synthetic.type[i];

    if (type == TOPO_OBJ_TYPE_UNKNOWN) {
      switch (count-1-i) {
      case 0: type = TOPO_OBJ_PROC; break;
      case 1: type = TOPO_OBJ_CORE; break;
      case 2: type = TOPO_OBJ_CACHE; break;
      case 3: type = TOPO_OBJ_SOCKET; break;
      case 4: type = TOPO_OBJ_NODE; break;
      default: type = TOPO_OBJ_FAKE; break;
      }
      topology->backend_params.synthetic.type[i] = type;
    }

    if (topology->type_depth[type] == TOPO_TYPE_DEPTH_UNKNOWN)
      topology->type_depth[type] = i+1;
    else
      topology->type_depth[type] = TOPO_TYPE_DEPTH_MULTIPLE;
  }

  /* last level must be PROC */
  if (topology->backend_params.synthetic.type[count-1] != TOPO_OBJ_PROC)
    return -1;

  /* their cannot be multiple NODE level (or nbnodes must be fixed) */
  if (topology->type_depth[TOPO_OBJ_NODE] == TOPO_TYPE_DEPTH_MULTIPLE)
    return -1;

  topology->backend_type = TOPO_BACKEND_SYNTHETIC;
  topology->backend_params.synthetic.arity[count] = 0;
  topology->is_fake = 1;

  return 0;
}

void
topo_backend_synthetic_exit(struct topo_topology *topology)
{
  assert(topology->backend_type == TOPO_BACKEND_SYNTHETIC);
  topology->backend_type = TOPO_BACKEND_NONE;
}
/* Allocate COUNT nodes of type TYPE as children of LEVEL, with numbers
   starting from FIRST_NUMBER.  Individual nodes are allocated from
   OBJ_POOL, which must point to an array of at least COUNT objects (children
   N's storage will be NODE_POOL[N]).  */
static void
topo__synthetic_make_children(struct topo_topology *topology,
			      struct topo_obj *obj, unsigned count,
			      enum topo_obj_type_e type, unsigned first_number,
			      struct topo_obj **obj_pool)
{
  unsigned i;
  unsigned physical_index;

  obj->children = calloc(count, sizeof(*obj->children));
  assert(obj->children != NULL);

  for (i = 0; i < count; i++) {
    obj->children[i] = obj_pool[i];
    assert(obj->children[i] != NULL);

    switch(type) {
    case TOPO_OBJ_PROC:
    case TOPO_OBJ_CORE:
    case TOPO_OBJ_CACHE:
    case TOPO_OBJ_SOCKET:
    case TOPO_OBJ_NODE:
      physical_index = first_number + i;
      break;
    default:
      physical_index = -1;
      break;
    }

    obj->arity = count;

    topo_setup_object(obj->children[i], type, physical_index);
    obj->children[i]->father = obj;
    obj->children[i]->index = i;
    obj->children[i]->level = obj->level + 1;
    obj->children[i]->number = first_number + i;

    switch(type) {
    case TOPO_OBJ_FAKE:
      obj->children[i]->attr.fake.depth = 0;
      break;
    default:
      break;
    }
  }
}

/* Recursively populate the topology starting from LEVEL according to
   LEVEL_BREADTH, and number levels starting at FIRST_INDEX.
   Return the total number of levels beneath LEVEL.  */
static unsigned
topo__synthetic_populate_topology(struct topo_topology *topology, struct topo_obj *obj,
				  const unsigned *level_breadth, unsigned first_index)
{
  unsigned i, count;
  enum topo_obj_type_e type;

  /* Recursion ends when *LEVEL_BREADTH is zero, meaning that OBJ has no
     children.  */
  if (*level_breadth > 0) {
    unsigned siblings;

    /* Processors don't have children.  */
    assert(obj->type != TOPO_OBJ_PROC);

    /* Determine the children object type, either from the given string, or use a default type.  */
    type = topology->backend_params.synthetic.type[obj->level];

    /* Current number of siblings on our children's object to our left.  */
    siblings = topology->level_nbobjects[obj->level + 1];

    topo__synthetic_make_children(topology, obj, *level_breadth, type, siblings,
				  &topology->levels[obj->level + 1][siblings]);

    /* Increment the total breadth for this level.  */
    topology->level_nbobjects[obj->level + 1] += *level_breadth;

    for (i = 0, count = 0; i < *level_breadth; i++)
      count += topo__synthetic_populate_topology(topology,
						    obj->children[i],
						    level_breadth + 1,
						    first_index + count);

    /* Aggregate the cpusets of our kids.  */
    topo_cpuset_zero(&obj->cpuset);
    for (i = 0; i < *level_breadth; i++)
      topo_cpuset_orset(&obj->cpuset, &obj->children[i]->cpuset);

  } else {
    /* Only processors have no children.  */
    assert(obj->type == TOPO_OBJ_PROC);

    topo_cpuset_zero(&obj->cpuset);
    topo_cpuset_set(&obj->cpuset, first_index);

    count = 1;
  }

  return count;
}

/* Allocate an array for each topology level described by TOPOLOGY. */
static void
topo__synthetic_allocate_topology_levels(struct topo_topology *topology,
					 const unsigned *breadths)
{
  const unsigned *level_breadth;
  unsigned level, total_level_breadth;

  for (level = 1, level_breadth = breadths, total_level_breadth = 1;
       *level_breadth > 0;
       level++, level_breadth++) {
    unsigned i;
    topo_obj_t *objs;

    total_level_breadth *= *level_breadth;

    topo_debug("synthetic topology: creating level %u with breadth %u (%u children per father)\n",
	       topology->nb_levels, total_level_breadth, *level_breadth);
    objs = calloc(total_level_breadth+1, sizeof(struct topo_obj *));
    assert(objs != NULL);

    for(i=0; i<total_level_breadth; i++) {
      objs[i] = malloc(sizeof(struct topo_obj));
      assert(objs[i]);
    }

    /* Update the level type to level mapping.  */
    topology->levels[level] = objs;

    topology->nb_levels++;
  }

  assert(topology->nb_levels > 1);

  topo_debug("synthetic topology: total number of levels: %u\n", topology->nb_levels);
}

void
topo_synthetic_load (struct topo_topology *topology)
{
  struct topo_obj *root;
  int node_level, cache_level;

  topo__synthetic_allocate_topology_levels(topology, topology->backend_params.synthetic.arity);

  root = topology->levels[0][0];

  topo__synthetic_populate_topology(topology, root, topology->backend_params.synthetic.arity, 0);
  assert(root->arity == *topology->backend_params.synthetic.arity);

  topology->nb_processors = topology->level_nbobjects[topology->nb_levels-1];

  node_level = topology->type_depth[TOPO_OBJ_NODE];
  assert(node_level != TOPO_TYPE_DEPTH_MULTIPLE);
  if (node_level != TOPO_TYPE_DEPTH_UNKNOWN) {
    /* Assume this level is the node level.  */
    int i;

    topology->nb_nodes = topology->level_nbobjects[node_level];

    for(i=0 ; i<topology->nb_nodes ; i++)
      topology->levels[node_level][i]->attr.node.memory_kB = 1024*1024;

  } else {
    topology->nb_nodes = 1;
  }

  for(cache_level=topology->nb_levels-1; cache_level>=0; cache_level--) {
    int i;

    if (topology->levels[cache_level][0]->type != TOPO_OBJ_CACHE)
      continue;

    for(i=0 ; i<topology->level_nbobjects[cache_level] ; i++) {
      topology->levels[cache_level][i]->attr.cache.memory_kB = 4*1024;
      topology->levels[cache_level][i]->attr.cache.depth = 2;
    }
  }

  topo_debug("synthetic topology: %u levels, %u processors, %u nodes\n",
	     topology->nb_levels, topology->nb_processors, topology->nb_nodes);
}
