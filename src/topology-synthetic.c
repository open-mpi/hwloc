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
topo_synthetic_parse_description(struct topo_topology *topology, const char *description)
{
  const char *pos, *next_pos;
  unsigned long item, count;

  for (pos = description, count = 0; *pos; pos = next_pos) {
    item = strtoul(pos, (char **)&next_pos, 0);
    if (next_pos == pos)
      return -1;

    assert(count + 1 < TOPO_SYNTHETIC_MAX_DEPTH);
    assert(item <= UINT_MAX);

    topology->synthetic_description[count++] = (unsigned)item;
  }

  if (count > 0) {
    topology->synthetic_description[count + 1] = 0;
    topology->use_synthetic = 1;
    topology->is_fake = 1;
  }

  return (count > 0) ? 0 : -1;
}

/* Determine a level type for for the level whose description is pointed to
   by LEVEL_BREADTH.  */
static enum topo_level_type_e
topo__synthetic_level_type (const unsigned *level_breadth)
{
  return (*(level_breadth + 1) == 0
	 ? TOPO_LEVEL_PROC
	 : (*(level_breadth + 2) == 0
	   ? TOPO_LEVEL_CORE
	   : (*(level_breadth + 3) == 0
	     ? TOPO_LEVEL_DIE
	     : (*(level_breadth + 4) == 0
	       ? TOPO_LEVEL_NODE
	       : TOPO_LEVEL_FAKE))));
}

/* Allocate COUNT nodes of type TYPE as children of LEVEL, with numbers
   starting from FIRST_NUMBER.  Individual nodes are allocated from
   NODE_POOL, which must point to an array of at least COUNT items (children
   N's storage will be NODE_POOL[N]).  */
static void
topo__synthetic_make_children(struct topo_topology *topology,
			      struct topo_level *level, unsigned count,
			      enum topo_level_type_e type, unsigned first_number,
			      struct topo_level *node_pool)
{
  unsigned i;

  level->children = calloc(count, sizeof(*level->children));
  assert(level->children != NULL);

  for (i = 0; i < count; i++) {
    level->children[i] = &node_pool[i];
    assert(level->children[i] != NULL);

    level->arity = count;

    topo_setup_level(level->children[i], type);
    level->children[i]->father = level;
    level->children[i]->index = i;
    level->children[i]->level = level->level + 1;
    level->children[i]->number = first_number + i;

    switch(type) {
    case TOPO_LEVEL_PROC:
    case TOPO_LEVEL_CORE:
    case TOPO_LEVEL_DIE:
    case TOPO_LEVEL_NODE:
      topo_set_os_numbers(level->children[i], type, first_number + i);
      break;
    default:
      topo_set_empty_os_numbers(level->children[i]);
    }
  }
}

/* Recursively populate the topology starting from LEVEL according to
   LEVEL_BREADTH, and number VPs starting at FIRST_VP.  Return the total
   number of VPs beneath LEVEL.  */
static unsigned
topo__synthetic_populate_topology(struct topo_topology *topology, struct topo_level *level,
				  const unsigned *level_breadth, unsigned first_vp)
{
  unsigned i, vp_count;
  enum topo_level_type_e type;

  /* Recursion ends when *LEVEL_BREADTH is zero, meaning that LEVEL has no
     children.  */
  if (*level_breadth > 0) {
    unsigned siblings;

    /* Processors don't have children.  */
    assert(level->type != TOPO_LEVEL_PROC);

    /* Determine the children level type.  */
    type = topo__synthetic_level_type (level_breadth);

    /* Current number of siblings on our children's level to our left.  */
    siblings = topology->level_nbitems[level->level + 1];

    topo__synthetic_make_children(topology, level, *level_breadth, type, siblings,
				  &topology->levels[level->level + 1][siblings]);

    /* Increment the total breadth for this level.  */
    topology->level_nbitems[level->level + 1] += *level_breadth;

    for (i = 0, vp_count = 0; i < *level_breadth; i++)
      vp_count += topo__synthetic_populate_topology(topology,
						    level->children[i],
						    level_breadth + 1,
						    first_vp + vp_count);

    /* Aggregate the VP sets of our kids.  */
    topo_cpuset_zero(&level->cpuset);
    for (i = 0; i < *level_breadth; i++)
      topo_cpuset_orset(&level->cpuset, &level->children[i]->cpuset);

  } else {
    /* Only processors have no children.  */
    assert(level->type == TOPO_LEVEL_PROC);

    topo_cpuset_zero(&level->cpuset);
    topo_cpuset_set(&level->cpuset, first_vp);

    vp_count = 1;
  }

  return vp_count;
}

/* Allocate an array for each topology level described by TOPOLOGY.  These
   arrays are stored in the global `marcel_topo_levels' variable.  */
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
    topology->levels[level] = malloc(count * sizeof(struct topo_level));

    assert(topology->levels[level] != NULL);

    /* Each level is terminated by an item with zeroed VP sets.  */
    topo_cpuset_zero(&topology->levels[level][total_level_breadth].cpuset);

    /* Update the level type to level mapping.  */
    topology->type_depth[topo__synthetic_level_type (level_breadth)] = level;

    topology->nb_levels++;
  }

  assert(topology->nb_levels > 1);

  topo_debug("synthetic topology: total number of levels: %u\n", topology->nb_levels);
}

void
topo_synthetic_load (struct topo_topology *topology)
{
  struct topo_level *root;
  unsigned node_level;

  topo__synthetic_allocate_topology_levels(topology, topology->synthetic_description);

  root = &topology->levels[0][0];

  topo__synthetic_populate_topology(topology, root, topology->synthetic_description, 0);
  assert(root->arity == *topology->synthetic_description);

  topology->nb_processors = topology->level_nbitems[topology->nb_levels-1];

  node_level = topology->type_depth[TOPO_LEVEL_NODE];
  if (node_level != -1) {
    /* Assume this level is the node level.  */
    int i;

    topology->nb_nodes = topology->level_nbitems[node_level];

    for(i=0 ; i<topology->nb_nodes ; i++)
      topology->levels[node_level][i].memory_kB[TOPO_LEVEL_MEMORY_NODE] = 1024*1024;

  } else {
    topology->nb_nodes = 1;
  }

  topo_debug("synthetic topology: %u levels, %u processors, %u nodes\n",
	     topology->nb_levels, topology->nb_processors, topology->nb_nodes);
}
