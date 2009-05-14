/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#include <assert.h>

/* Read from DESCRIPTION a series of integers describing a symmetrical
   topology and update `topology->synthetic_description' accordingly.  On
   success, return zero.  */
int
topo_backend_synthetic_init(struct topo_topology *topology, const char *description)
{
  const char *pos, *next_pos;
  unsigned long item, count;

  assert(topology->backend_type == TOPO_BACKEND_NONE);

  for (pos = description, count = 0; *pos; pos = next_pos) {
    item = strtoul(pos, (char **)&next_pos, 0);
    if (next_pos == pos)
      return -1;

    assert(count + 1 < TOPO_SYNTHETIC_MAX_DEPTH);
    assert(item <= UINT_MAX);

    topology->backend_params.synthetic.description[count++] = (unsigned)item;
  }

  if (count > 0) {
    topology->backend_type = TOPO_BACKEND_SYNTHETIC;
    topology->backend_params.synthetic.description[count] = 0;
    topology->is_fake = 1;
  }

  return (count > 0) ? 0 : -1;
}

void
topo_backend_synthetic_exit(struct topo_topology *topology)
{
  assert(topology->backend_type == TOPO_BACKEND_SYNTHETIC);
  topology->backend_type = TOPO_BACKEND_NONE;
}

/* Determine a level type for for the level whose description is pointed to
   by LEVEL_BREADTH.  */
static enum topo_obj_type_e
topo__synthetic_object_type (const unsigned *level_breadth)
{
  return (*(level_breadth + 1) == 0
	 ? TOPO_OBJ_PROC
	 : (*(level_breadth + 2) == 0
	   ? TOPO_OBJ_CORE
	   : (*(level_breadth + 3) == 0
	     ? TOPO_OBJ_CACHE
	     : (*(level_breadth + 4) == 0
	       ? TOPO_OBJ_DIE
	       : (*(level_breadth + 5) == 0
	         ? TOPO_OBJ_NODE
	         : TOPO_OBJ_FAKE)))));
}

/* Allocate COUNT nodes of type TYPE as children of LEVEL, with numbers
   starting from FIRST_NUMBER.  Individual nodes are allocated from
   OBJ_POOL, which must point to an array of at least COUNT items (children
   N's storage will be NODE_POOL[N]).  */
static void
topo__synthetic_make_children(struct topo_topology *topology,
			      struct topo_obj *obj, unsigned count,
			      enum topo_obj_type_e type, unsigned first_number,
			      struct topo_obj *obj_pool)
{
  unsigned i;
  unsigned physical_index;

  obj->children = calloc(count, sizeof(*obj->children));
  assert(obj->children != NULL);

  for (i = 0; i < count; i++) {
    obj->children[i] = &obj_pool[i];
    assert(obj->children[i] != NULL);

    switch(type) {
    case TOPO_OBJ_PROC:
    case TOPO_OBJ_CORE:
    case TOPO_OBJ_CACHE:
    case TOPO_OBJ_DIE:
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

    /* Determine the children object type.  */
    type = topo__synthetic_object_type (level_breadth);

    /* Current number of siblings on our children's object to our left.  */
    siblings = topology->level_nbitems[obj->level + 1];

    topo__synthetic_make_children(topology, obj, *level_breadth, type, siblings,
				  &topology->levels[obj->level + 1][siblings]);

    /* Increment the total breadth for this level.  */
    topology->level_nbitems[obj->level + 1] += *level_breadth;

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
    unsigned count;

    total_level_breadth *= *level_breadth;

    count = total_level_breadth + 1;

    topo_debug("synthetic topology: creating level %u with breadth %u (%u children per father)\n",
	       topology->nb_levels, total_level_breadth, *level_breadth);
    topology->levels[level] = malloc(count * sizeof(struct topo_obj));

    assert(topology->levels[level] != NULL);

    /* Each level is terminated by an item with zeroed cpuset.  */
    topo_cpuset_zero(&topology->levels[level][total_level_breadth].cpuset);

    /* Update the level type to level mapping.  */
    topology->type_depth[topo__synthetic_object_type (level_breadth)] = level;

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

  topo__synthetic_allocate_topology_levels(topology, topology->backend_params.synthetic.description);

  root = &topology->levels[0][0];

  topo__synthetic_populate_topology(topology, root, topology->backend_params.synthetic.description, 0);
  assert(root->arity == *topology->backend_params.synthetic.description);

  topology->nb_processors = topology->level_nbitems[topology->nb_levels-1];

  node_level = topology->type_depth[TOPO_OBJ_NODE];
  if (node_level != -1) {
    /* Assume this level is the node level.  */
    int i;

    topology->nb_nodes = topology->level_nbitems[node_level];

    for(i=0 ; i<topology->nb_nodes ; i++)
      topology->levels[node_level][i].memory_kB = 1024*1024;

  } else {
    topology->nb_nodes = 1;
  }

  cache_level = topology->type_depth[TOPO_OBJ_CACHE];
  if (cache_level != -1) {
    int i;

    for(i=0 ; i<topology->level_nbitems[cache_level] ; i++) {
      topology->levels[cache_level][i].memory_kB = 4*1024;
      topology->levels[cache_level][i].cache_depth = 2;
    }
  }

  topo_debug("synthetic topology: %u levels, %u processors, %u nodes\n",
	     topology->nb_levels, topology->nb_processors, topology->nb_nodes);
}
