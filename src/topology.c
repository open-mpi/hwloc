/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>

#define _ATFILE_SOURCE
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HAVE_MACH_MACH_INIT_H
#include <mach/mach_init.h>
#endif
#ifdef HAVE_MACH_MACH_HOST_H
#include <mach/mach_host.h>
#endif

#ifdef WIN_SYS
#include <windows.h>
#endif

/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `hwloc_set_fsroot ()' to
   have the desired effect.  */
unsigned
hwloc_fallback_nbprocessors(void) {
#if HAVE_DECL__SC_NPROCESSORS_ONLN
  return sysconf(_SC_NPROCESSORS_ONLN);
#elif HAVE_DECL__SC_NPROC_ONLN
  return sysconf(_SC_NPROC_ONLN);
#elif HAVE_DECL__SC_NPROCESSORS_CONF
  return sysconf(_SC_NPROCESSORS_CONF);
#elif HAVE_DECL__SC_NPROC_CONF
  return sysconf(_SC_NPROC_CONF);
#elif defined(HAVE_HOST_INFO) && HAVE_HOST_INFO
  struct host_basic_info info;
  mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
  host_info(mach_host_self(), HOST_BASIC_INFO, (integer_t*) &info, &count);
  return info.avail_cpus;
#elif defined(WIN_SYS)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#else
#ifdef __GNUC__
#warning No known way to discover number of available processors on this system
#warning hwloc_fallback_nbprocessors will default to 1
#endif
  return 1;
#endif
}

/*
 * Place objects in groups if they are in complete graphs with minimal distances.
 * Return how many groups were created, or 0 if some incomplete distance graphs were found.
 */
static unsigned
hwloc_setup_group_from_min_distance_clique(unsigned nbobjs,
					  struct hwloc_obj **objs,
					  unsigned *_distances,
					  unsigned *groupids)
{
  unsigned (*distances)[nbobjs][nbobjs] = (unsigned (*)[nbobjs][nbobjs])_distances;
  unsigned groupid = 0;
  unsigned i,j,k;

  memset(groupids, 0, nbobjs*sizeof(*groupids));

  /* try to find complete graphs */
  for(i=0; i<nbobjs; i++) {
    hwloc_cpuset_t closest_objs_set = hwloc_cpuset_alloc();
    unsigned min_distance = UINT_MAX;
    unsigned size = 1; /* current object i */

    /* if already grouped, skip */
    if (groupids[i])
      continue;

    /* find closest nodes */
    for(j=i+1; j<nbobjs; j++) {
      if ((*distances)[i][j] < min_distance) {
	/* reset the closest set and use new min_distance */
	hwloc_cpuset_cpu(closest_objs_set, j);
	min_distance = (*distances)[i][j];
	size = 2; /* current objects i and j */
      } else if ((*distances)[i][j] == min_distance) {
	/* add object to current closest set */
	hwloc_cpuset_set(closest_objs_set, j);
	size++;
      }
    }
    /* check that we actually have a complete graph between these closest objects */
    for (j=i+1; j<nbobjs; j++)
      for (k=j+1; k<nbobjs; k++)
	if (hwloc_cpuset_isset(closest_objs_set, j) &&
	    hwloc_cpuset_isset(closest_objs_set, k) &&
	    (*distances)[j][k] != min_distance) {
	  /* the minimal-distance graph is not complete. abort */
	  hwloc_debug("found incomplete minimal-distance graph, aborting\n");
	  return 0;
	}

    /* fill a new group */
    groupid++;
    groupids[i] = groupid;
    for(j=i+1; j<nbobjs; j++)
      if (hwloc_cpuset_isset(closest_objs_set, j))
	groupids[j] = groupid;
    hwloc_debug("found complete graph with %u objects with minimal distance %u\n",
	       size, min_distance);
    free(closest_objs_set);
  }

  /* return the last id, since it's also the number of used group ids */
  return groupid;
}

/*
 * Place objects in groups if they are in a transitive graph of minimal distances.
 * Return how many groups were created, or 0 if some incomplete distance graphs were found.
 */
static unsigned
hwloc_setup_group_from_min_distance_transitivity(unsigned nbobjs,
						struct hwloc_obj **objs,
						unsigned *_distances,
						unsigned *groupids)
{
  unsigned (*distances)[nbobjs][nbobjs] = (unsigned (*)[nbobjs][nbobjs])_distances;
  unsigned groupid = 0;
  unsigned i,j,k;

  memset(groupids, 0, nbobjs*sizeof(*groupids));

  /* try to find complete graphs */
  for(i=0; i<nbobjs; i++) {
    hwloc_cpuset_t closest_objs_set = hwloc_cpuset_alloc();
    unsigned min_distance = UINT_MAX;
    unsigned size = 1; /* current object i */

    hwloc_cpuset_zero(closest_objs_set);

    /* if already grouped, skip */
    if (groupids[i])
      continue;

    /* find closest nodes */
    for(j=i+1; j<nbobjs; j++) {
      if ((*distances)[i][j] < min_distance) {
	/* reset the closest set and use new min_distance */
	hwloc_cpuset_cpu(closest_objs_set, j);
	min_distance = (*distances)[i][j];
	size = 2; /* current objects i and j */
      } else if ((*distances)[i][j] == min_distance) {
	/* add object to current closest set */
	hwloc_cpuset_set(closest_objs_set, j);
	size++;
      }
    }
    /* find close objs by transitivity */
    while (1) {
      unsigned found = 0;
      for(j=i+1; j<nbobjs; j++)
	for(k=j+1; k<nbobjs; k++)
	  if ((*distances)[j][k] <= min_distance
	      && hwloc_cpuset_isset(closest_objs_set, j)
	      && !hwloc_cpuset_isset(closest_objs_set, k)) {
	    hwloc_cpuset_set(closest_objs_set, k);
	    size++;
	    found = 1;
	  }
      if (!found)
	break;
    }

    /* fill a new group */
    groupid++;
    groupids[i] = groupid;
    for(j=i+1; j<nbobjs; j++)
      if (hwloc_cpuset_isset(closest_objs_set, j))
	groupids[j] = groupid;
    hwloc_debug("found transitive graph with %u objects with minimal distance %u\n",
	       size, min_distance);
    free(closest_objs_set);
  }

  /* return the last id, since it's also the number of used group ids */
  return groupid;
}

/*
 * Look at object physical distances to group them,
 * after having done some basic sanity checks.
 */
static void
hwloc__setup_misc_level_from_distances(struct hwloc_topology *topology,
				      unsigned nbobjs,
				      struct hwloc_obj **objs,
				      unsigned *_distances,
				      int depth)
{
  unsigned (*distances)[nbobjs][nbobjs] = (unsigned (*)[nbobjs][nbobjs])_distances;
  unsigned groupids[nbobjs];
  int nbgroups;
  unsigned i,j;

  hwloc_debug("trying to group %s objects into misc objects according to physical distances\n",
	     hwloc_obj_type_string(objs[0]->type));

  if (nbobjs <= 2)
    return;

  nbgroups = hwloc_setup_group_from_min_distance_clique(nbobjs, objs, _distances, groupids);
  if (!nbgroups) {
    nbgroups = hwloc_setup_group_from_min_distance_transitivity(nbobjs, objs, _distances, groupids);
    if (!nbgroups)
      return;
  }

  if (nbgroups == 1) {
    hwloc_debug("ignoring misc object with all objects\n");
    return;
  }

  /* For convenience, put these declarations inside a block.  Saves us
     from a bunch of mallocs, particularly with the 2D array. */
  {
      hwloc_obj_t groupobjs[nbgroups];
      unsigned groupsizes[nbgroups];
      unsigned groupdistances[nbgroups][nbgroups];
      /* create new misc objects and record their size */
      memset(groupsizes, 0, sizeof(groupsizes));
      for(i=0; i<nbgroups; i++) {
          /* create the misc object */
          hwloc_obj_t misc_obj;
          misc_obj = hwloc_alloc_setup_object(HWLOC_OBJ_MISC, -1);
          misc_obj->cpuset = hwloc_cpuset_alloc();
          hwloc_cpuset_zero(misc_obj->cpuset);
          misc_obj->attr->misc.depth = depth;
          for (j=0; j<nbobjs; j++)
              if (groupids[j] == i+1) {
                  hwloc_cpuset_orset(misc_obj->cpuset, objs[j]->cpuset);
                  groupsizes[i]++;
              }
          hwloc_debug_1arg_cpuset("adding misc object with %u objects and cpuset %s\n",
                                  groupsizes[i], misc_obj->cpuset);
          hwloc_add_object(topology, misc_obj);
          groupobjs[i] = misc_obj;
      }
      
      /* factorize distances */
      memset(groupdistances, 0, sizeof(groupdistances));
      for(i=0; i<nbobjs; i++)
          for(j=0; j<nbobjs; j++)
              groupdistances[groupids[i]-1][groupids[j]-1] += (*distances)[i][j];
      for(i=0; i<nbgroups; i++)
          for(j=0; j<nbgroups; j++)
              groupdistances[i][j] /= groupsizes[i]*groupsizes[j];
#ifdef HWLOC_DEBUG
      hwloc_debug("group distances:\n");
      for(i=0; i<nbgroups; i++) {
          for(j=0; j<nbgroups; j++)
              hwloc_debug("%u ", groupdistances[i][j]);
          hwloc_debug("\n");
      }
#endif
      
      hwloc__setup_misc_level_from_distances(topology, nbgroups, groupobjs, (unsigned*) groupdistances, depth + 1);
  }
}

/*
 * Look at object physical distances to group them.
 */
void
hwloc_setup_misc_level_from_distances(struct hwloc_topology *topology,
				     unsigned nbobjs,
				     struct hwloc_obj **objs,
				     unsigned *_distances)
{
  unsigned (*distances)[nbobjs][nbobjs] = (unsigned (*)[nbobjs][nbobjs])_distances;
  unsigned i,j;

  if (getenv("HWLOC_IGNORE_DISTANCES"))
    return;

#ifdef HWLOC_DEBUG
  hwloc_debug("node distance matrix:\n");
  hwloc_debug("   ");
  for(j=0; j<nbobjs; j++)
    hwloc_debug(" %3d", j);
  hwloc_debug("\n");

  for(i=0; i<nbobjs; i++) {
    hwloc_debug("%3d", i);
    for(j=0; j<nbobjs; j++)
      hwloc_debug(" %3d", (*distances)[i][j]);
    hwloc_debug("\n");
  }
#endif

  /* check that the matrix is ok */
  for(i=0; i<nbobjs; i++) {
    for(j=i+1; j<nbobjs; j++) {
      /* should be symmetric */
      if ((*distances)[i][j] != (*distances)[j][i]) {
	hwloc_debug("distance matrix asymmetric ([%u,%u]=%u != [%u,%u]=%u), aborting\n",
		   i, j, (*distances)[i][j], j, i, (*distances)[j][i]);
	return;
      }
      /* diagonal is smaller than everything else */
      if ((*distances)[i][j] <= (*distances)[i][i]) {
	hwloc_debug("distance to self not strictly minimal ([%u,%u]=%u <= [%u,%u]=%u), aborting\n",
		   i, j, (*distances)[i][j], i, i, (*distances)[i][i]);
	return;
      }
    }
  }

  hwloc__setup_misc_level_from_distances(topology, nbobjs, objs, _distances, 0);
}

/*
 * Use the given number of processors and the optional online cpuset if given
 * to set a Proc level.
 */
void
hwloc_setup_proc_level(struct hwloc_topology *topology,
		      unsigned nb_processors,
		      hwloc_cpuset_t online_cpuset)
{
  struct hwloc_obj *obj;
  unsigned oscpu,cpu;

  hwloc_debug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<nb_processors; oscpu++)
    {
      if (online_cpuset && !hwloc_cpuset_isset(online_cpuset, oscpu))
       continue;

      obj = hwloc_alloc_setup_object(HWLOC_OBJ_PROC, oscpu);
      obj->cpuset = hwloc_cpuset_alloc();
      hwloc_cpuset_cpu(obj->cpuset, oscpu);

      hwloc_debug_2args_cpuset("cpu %d (os %d) has cpuset %s\n",
		 cpu, oscpu, obj->cpuset);
      hwloc_add_object(topology, obj);

      cpu++;
    }
}

/* Just for debugging.  */
static void
print_objects(struct hwloc_topology *topology, int indent, hwloc_obj_t obj)
{
  char line[256], *cpuset = NULL;
  hwloc_debug("%*s", 2*indent, "");
  hwloc_obj_snprintf(line, sizeof(line), topology, obj, "#", 1);
  hwloc_debug("%s", line);
  hwloc_cpuset_asprintf(&cpuset, obj->cpuset);
  hwloc_debug(" %s", cpuset);
  free(cpuset);
  if (obj->arity)
    hwloc_debug(" arity %d", obj->arity);
  hwloc_debug("\n");
  for (obj = obj->first_child; obj; obj = obj->next_sibling)
    print_objects(topology, indent + 1, obj);
}

/* Free an object and all its content.  */
static void
free_object(hwloc_obj_t obj)
{
  switch (obj->type) {
  case HWLOC_OBJ_SYSTEM:
    free(obj->attr->system.dmi_board_vendor);
    free(obj->attr->system.dmi_board_name);
    break;
  case HWLOC_OBJ_MACHINE:
    free(obj->attr->machine.dmi_board_vendor);
    free(obj->attr->machine.dmi_board_name);
    break;
  default:
    break;
  }
  free(obj->attr);
  free(obj->children);
  free(obj->name);
  free(obj->cpuset);
  free(obj);
}

/*
 * How to compare objects based on types.
 *
 * Note that HIGHER/LOWER is only a (consistent) heuristic, used to sort
 * objects with same cpuset consistently.
 * Only EQUAL / not EQUAL can be relied upon.
 */

enum hwloc_type_cmp_e {
  HWLOC_TYPE_HIGHER,
  HWLOC_TYPE_DEEPER,
  HWLOC_TYPE_EQUAL,
};

static const int obj_type_order[] = {
  [HWLOC_OBJ_SYSTEM] = 0,
  [HWLOC_OBJ_MACHINE] = 1,
  [HWLOC_OBJ_MISC] = 2,
  [HWLOC_OBJ_NODE] = 3,
  [HWLOC_OBJ_SOCKET] = 4,
  [HWLOC_OBJ_CACHE] = 5,
  [HWLOC_OBJ_CORE] = 6,
  [HWLOC_OBJ_PROC] = 7,
};

static const hwloc_obj_type_t obj_order_type[] = {
  [0] = HWLOC_OBJ_SYSTEM,
  [1] = HWLOC_OBJ_MACHINE,
  [2] = HWLOC_OBJ_MISC,
  [3] = HWLOC_OBJ_NODE,
  [4] = HWLOC_OBJ_SOCKET,
  [5] = HWLOC_OBJ_CACHE,
  [6] = HWLOC_OBJ_CORE,
  [7] = HWLOC_OBJ_PROC,
};

static int hwloc_get_type_order(hwloc_obj_type_t type)
{
  return obj_type_order[type];
}

static hwloc_obj_type_t hwloc_get_order_type(int order)
{
  return obj_order_type[order];
}

int hwloc_compare_types (hwloc_obj_type_t type1, hwloc_obj_type_t type2)
{
  return hwloc_get_type_order(type1) - hwloc_get_type_order(type2);
}

static enum hwloc_type_cmp_e
hwloc_type_cmp(hwloc_obj_t obj1, hwloc_obj_t obj2)
{
  if (hwloc_compare_types(obj1->type, obj2->type) > 0)
    return HWLOC_TYPE_DEEPER;
  if (hwloc_compare_types(obj1->type, obj2->type) < 0)
    return HWLOC_TYPE_HIGHER;

  /* Caches have the same types but can have different depths.  */
  if (obj1->type == HWLOC_OBJ_CACHE) {
    if (obj1->attr->cache.depth < obj2->attr->cache.depth)
      return HWLOC_TYPE_DEEPER;
    else if (obj1->attr->cache.depth > obj2->attr->cache.depth)
      return HWLOC_TYPE_HIGHER;
  }

  /* Misc objects have the same types but can have different depths.  */
  if (obj1->type == HWLOC_OBJ_MISC) {
    if (obj1->attr->misc.depth < obj2->attr->misc.depth)
      return HWLOC_TYPE_DEEPER;
    else if (obj1->attr->misc.depth > obj2->attr->misc.depth)
      return HWLOC_TYPE_HIGHER;
  }

  return HWLOC_TYPE_EQUAL;
}

/*
 * How to compare objects based on cpusets.
 */

enum hwloc_obj_cmp_e {
  HWLOC_OBJ_EQUAL,	/**< \brief Equal */
  HWLOC_OBJ_INCLUDED,	/**< \brief Strictly included into */
  HWLOC_OBJ_CONTAINS,	/**< \brief Strictly contains */
  HWLOC_OBJ_INTERSECTS,	/**< \brief Intersects, but no inclusion! */
  HWLOC_OBJ_DIFFERENT,	/**< \brief No intersection */
};

static int
hwloc_obj_cmp(hwloc_obj_t obj1, hwloc_obj_t obj2)
{
  if (hwloc_cpuset_iszero(obj1->cpuset) || hwloc_cpuset_iszero(obj2->cpuset))
    return HWLOC_OBJ_DIFFERENT;

  if (hwloc_cpuset_isequal(obj1->cpuset, obj2->cpuset)) {

    /* Same cpuset, subsort by type to have a consistent ordering.  */

    switch (hwloc_type_cmp(obj1, obj2)) {
      case HWLOC_TYPE_DEEPER:
	return HWLOC_OBJ_INCLUDED;
      case HWLOC_TYPE_HIGHER:
	return HWLOC_OBJ_CONTAINS;
      case HWLOC_TYPE_EQUAL:
	/* Same level cpuset and type!  Let's hope it's coherent.  */
	return HWLOC_OBJ_EQUAL;
    }

    /* For dumb compilers */
    abort();

  } else {

    /* Different cpusets, sort by inclusion.  */

    if (hwloc_cpuset_isincluded(obj1->cpuset, obj2->cpuset))
      return HWLOC_OBJ_INCLUDED;

    if (hwloc_cpuset_isincluded(obj2->cpuset, obj1->cpuset))
      return HWLOC_OBJ_CONTAINS;

    if (hwloc_cpuset_intersects(obj1->cpuset, obj2->cpuset))
      return HWLOC_OBJ_INTERSECTS;

    return HWLOC_OBJ_DIFFERENT;
  }
}

/*
 * How to insert objects into the topology.
 *
 * Note: during detection, only the first_child and next_sibling pointers are
 * kept up to date.  Others are computed only once topology detection is
 * complete.
 */

#define merge_sizes(new, old, field) \
  if (!(old)->field) \
    (old)->field = (new)->field;
#define check_sizes(new, old, field) \
  merge_sizes(new, old, field) \
  if ((new)->field) \
    assert((old)->field == (new)->field)

/* Try to insert OBJ in CUR, recurse if needed */
static void
add_object(struct hwloc_topology *topology, hwloc_obj_t cur, hwloc_obj_t obj)
{
  hwloc_obj_t child, container, *cur_children, *obj_children, next_child;
  int put;

  /* Make sure we haven't gone too deep.  */
  assert(hwloc_cpuset_isincluded(obj->cpuset, cur->cpuset));

  /* Check whether OBJ is included in some child.  */
  container = NULL;
  for (child = cur->first_child; child; child = child->next_sibling) {
    switch (hwloc_obj_cmp(obj, child)) {
      case HWLOC_OBJ_EQUAL:
	assert(hwloc_cpuset_isequal(obj->cpuset, child->cpuset));
	assert(obj->os_index == child->os_index);
	switch(obj->type) {
	  case HWLOC_OBJ_NODE:
	    /* Do not check these, it may change between calls */
	    merge_sizes(obj, child, attr->node.memory_kB);
	    merge_sizes(obj, child, attr->node.huge_page_free);
	    break;
	  case HWLOC_OBJ_CACHE:
	    check_sizes(obj, child, attr->cache.memory_kB);
	    break;
	  default:
	    break;
	}
	/* Already present, no need to insert.  */
	return;
      case HWLOC_OBJ_INCLUDED:
	if (container) {
	  /* TODO: how to report?  */
	  fprintf(stderr, "object included in several different objects!\n");
	  /* We can't handle that.  */
	  return;
	}
	/* This child contains OBJ.  */
	container = child;
	break;
      case HWLOC_OBJ_INTERSECTS:
	/* TODO: how to report?  */
	fprintf(stderr, "object intersection without inclusion!\n");
	/* We can't handle that.  */
	return;
      case HWLOC_OBJ_CONTAINS:
	/* OBJ will be above CHILD.  */
	break;
      case HWLOC_OBJ_DIFFERENT:
	/* OBJ will be alongside CHILD.  */
	break;
    }
  }

  if (container) {
    /* OBJ is strictly contained is some child of CUR, go deeper.  */
    add_object(topology, container, obj);
    return;
  }

  /*
   * Children of CUR are either completely different from or contained into
   * OBJ. Take those that are contained (keeping sorting order), and sort OBJ
   * along those that are different.
   */

  /* OBJ is not put yet.  */
  put = 0;

  /* These will always point to the pointer to their next last child. */
  cur_children = &cur->first_child;
  obj_children = &obj->first_child;

  /* Construct CUR's and OBJ's children list.  */

  /* Iteration with prefetching to be completely safe against CHILD removal.  */
  for (child = cur->first_child, child ? next_child = child->next_sibling : NULL;
       child;
       child = next_child, child ? next_child = child->next_sibling : NULL) {

    switch (hwloc_obj_cmp(obj, child)) {

      case HWLOC_OBJ_DIFFERENT:
	/* Leave CHILD in CUR.  */
	if (!put && hwloc_cpuset_compar_first(obj->cpuset, child->cpuset) < 0) {
	  /* Sort children by cpuset: put OBJ before CHILD in CUR's children.  */
	  *cur_children = obj;
	  cur_children = &obj->next_sibling;
	  put = 1;
	}
	/* Now put CHILD in CUR's children.  */
	*cur_children = child;
	cur_children = &child->next_sibling;
	break;

      case HWLOC_OBJ_CONTAINS:
	/* OBJ contains CHILD, put the latter in the former.  */
	*obj_children = child;
	obj_children = &child->next_sibling;
	break;

      case HWLOC_OBJ_EQUAL:
      case HWLOC_OBJ_INCLUDED:
      case HWLOC_OBJ_INTERSECTS:
	/* Shouldn't ever happen as we have handled them above.  */
	abort();
    }
  }

  /* Put OBJ last in CUR's children if not already done so.  */
  if (!put) {
    *cur_children = obj;
    cur_children = &obj->next_sibling;
  }

  /* Close children lists.  */
  *obj_children = NULL;
  *cur_children = NULL;
}

void
hwloc_add_object(struct hwloc_topology *topology, hwloc_obj_t obj)
{
  /* Start at the top.  */
  add_object(topology, topology->levels[0][0], obj);
}

/*
 * traverse the whole tree in a deletion-safe way, calling node_before at
 * nodes, leaf at leaves, and node_after when back at nodes, passing data along
 * the way through nodes. data returned by leaf() is ignored.
 *
 * Hooks can modify the pointer they're given to remove or replace themselves.
 */
static void
traverse(hwloc_topology_t topology,
	 hwloc_obj_t *father,
	 void (*node_before)(hwloc_topology_t topology, hwloc_obj_t *obj, void *),
	 void (*leaf)(hwloc_topology_t topology, hwloc_obj_t *obj, void *),
	 void (*node_after)(hwloc_topology_t topology, hwloc_obj_t *obj, void *),
	 void *data)
{
  hwloc_obj_t *pobj, obj;

  if (!(*father)->first_child) {
    if (leaf)
      leaf(topology, father, data);
    return;
  }
  if (node_before)
    node_before(topology, father, data);
  if (!(*father))
    return;
  for (pobj = &(*father)->first_child, obj = *pobj;
       obj;
       /* Check whether the current obj was dropped.  */
       (*pobj == obj ? pobj = &(*pobj)->next_sibling : NULL),
       /* Get pointer to next object.  */
	obj = *pobj)
    traverse(topology, pobj, node_before, leaf, node_after, data);
  if (node_after)
    node_after(topology, father, data);
}

static void
get_proc_cpuset(hwloc_topology_t topology, hwloc_obj_t *obj, void *data)
{
  hwloc_cpuset_t cpuset = data;
  if ((*obj)->type != HWLOC_OBJ_PROC)
    return;
  hwloc_cpuset_orset(cpuset, (*obj)->cpuset);
}

static void
apply_cpuset(hwloc_topology_t topology, hwloc_obj_t *obj, void *data)
{
  hwloc_cpuset_t cpuset = data;
  hwloc_cpuset_andset((*obj)->cpuset, cpuset);
}

static void
do_free_object(hwloc_topology_t topology, hwloc_obj_t *obj, void *data)
{
  free_object(*obj);
}

/* Remove all ignored objects.  */
static void
remove_ignored(hwloc_topology_t topology, hwloc_obj_t *pfather, void *data)
{
  hwloc_obj_t father = *pfather;
  if (topology->ignored_types[father->type] == HWLOC_IGNORE_TYPE_ALWAYS) {
    hwloc_obj_t child = father->first_child;
    /* Replace object with its list of children */
    *pfather = child;
    while (child->next_sibling)
      child = child->next_sibling;
    child->next_sibling = father->next_sibling;
    /* Remove ignored object */
    free_object(father);
  }
}

/* Remove all children whose cpuset is empty, except NUMA nodes
 * since we want to keep memory information.  */
static void
remove_empty(hwloc_topology_t topology, hwloc_obj_t *obj, void *data)
{
  if ((*obj)->type != HWLOC_OBJ_NODE && hwloc_cpuset_iszero((*obj)->cpuset)) {
    /* Remove empty children */
    traverse(topology, obj, NULL, NULL, do_free_object, NULL);
    *obj = (*obj)->next_sibling;
  }
}

/*
 * Merge with the only child if either the father or the child has a type to be
 * ignored while keeping structure
 */
static void
merge_useless_child(hwloc_topology_t topology, hwloc_obj_t *pfather, void *data)
{
  hwloc_obj_t father = *pfather, child = father->first_child;
  if (child->next_sibling)
    /* There are several children, it's useful to keep them.  */
    return;

  /* TODO: have a preference order?  */
  if (topology->ignored_types[father->type] == HWLOC_IGNORE_TYPE_KEEP_STRUCTURE) {
    /* Father can be ignored in favor of the child.  */
    *pfather = child;
    child->next_sibling = father->next_sibling;
    free_object(father);
  } else if (topology->ignored_types[child->type] == HWLOC_IGNORE_TYPE_KEEP_STRUCTURE) {
    /* Child can be ignored in favor of the father.  */
    father->first_child = child->first_child;
    free_object(child);
  }
}

/*
 * Initialize handy pointers in the whole topology
 */
static void
hwloc_connect(hwloc_obj_t father)
{
  unsigned n;
  hwloc_obj_t child, prev_child = NULL;

  for (n = 0, child = father->first_child;
       child;
       n++,   prev_child = child, child = child->next_sibling) {
    child->father = father;
    child->sibling_rank = n;
    child->prev_sibling = prev_child;
  }
  father->last_child = prev_child;

  father->arity = n;
  if (!n) {
    father->children = NULL;
    return;
  }

  father->children = malloc(n * sizeof(*father->children));
  assert(father->children);
  for (n = 0, child = father->first_child;
       child;
       n++,   child = child->next_sibling) {
    father->children[n] = child;
    hwloc_connect(child);
  }
}

/*
 * Check whether there is an object below ROOT that has the same type as OBJ
 */
static int
find_same_type(hwloc_obj_t root, hwloc_obj_t obj)
{
  hwloc_obj_t child;

  if (hwloc_type_cmp(root, obj) == HWLOC_TYPE_EQUAL)
    return 1;

  for (child = root->first_child; child; child = child->next_sibling)
    if (find_same_type(child, obj))
      return 1;

  return 0;
}

/*
 * Empty binding hooks always returning success
 */

static int dontset_cpubind(hwloc_topology_t topology, hwloc_cpuset_t set, int strict)
{
  return 0;
}
static int dontset_thisproc_cpubind(hwloc_topology_t topology, hwloc_cpuset_t set, int strict)
{
  return 0;
}
static int dontset_thisthread_cpubind(hwloc_topology_t topology, hwloc_cpuset_t set, int strict)
{
  return 0;
}
static int dontset_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t set, int strict)
{
  return 0;
}
#ifdef hwloc_thread_t
static int dontset_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t tid, hwloc_cpuset_t set, int strict)
{
  return 0;
}
#endif


/* Main discovery loop */
static void
hwloc_discover(struct hwloc_topology *topology)
{
  unsigned l, i=0, taken_i, new_i, j;
  hwloc_obj_t *objs, *taken_objs, *new_objs, top_obj;
  unsigned n_objs, n_taken_objs, n_new_objs;

  assert(topology!=NULL);

  if (topology->backend_type == HWLOC_BACKEND_SYNTHETIC) {
    hwloc_look_synthetic(topology);
#ifdef HAVE_XML
  } else if (topology->backend_type == HWLOC_BACKEND_XML) {
    hwloc_look_xml(topology);
#endif
  } else {

  /* Raw detection, from coarser levels to finer levels for more efficiency.  */

  /* hwloc_look_* functions should use hwloc_obj_add to add objects initialized
   * through hwloc_alloc_setup_object. For node levels, memory_Kb and huge_page_free
   * must be initialized. For cache levels, memory_kB and attr->cache.depth must be
   * initialized, for misc levels, attr->misc.depth must be initialized
   */

  /* There must be at least a PROC object for each logical processor, at worse
   * produced by hwloc_setup_proc_level()
   */

  /* If the OS can provide NUMA distances, it should call
   * hwloc_setup_misc_level_from_distances() to automatically group NUMA nodes
   * into misc objects.
   */

  /* Each OS type should also fill the bind functions pointers, at least the
   * set_cpubind one
   */

#    ifdef LINUX_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_linux(topology);
#    endif /* LINUX_SYS */

#    ifdef  AIX_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_aix(topology);
#    endif /* AIX_SYS */

#    ifdef  OSF_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_osf(topology);
#    endif /* OSF_SYS */

#    ifdef  SOLARIS_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_solaris(topology);
#    endif /* SOLARIS_SYS */

#    ifdef  WIN_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_windows(topology);
#    endif /* WIN_SYS */

#    ifdef  DARWIN_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_darwin(topology);
#    endif /* DARWIN_SYS */

#    ifdef  HPUX_SYS
#      define HAVE_OS_SUPPORT
    hwloc_look_hpux(topology);
#    endif /* HPUX_SYS */

#    ifndef HAVE_OS_SUPPORT
    hwloc_setup_proc_level(topology, hwloc_fallback_nbprocessors (), NULL);
#    endif /* Unsupported OS */
  }

  print_objects(topology, 0, topology->levels[0][0]);

  /* First tweak a bit to clean the topology.  */

  hwloc_debug("\nComputing the system cpuset by ORing all Proc objects\n");
  hwloc_cpuset_zero(topology->levels[0][0]->cpuset);
  traverse(topology, &topology->levels[0][0], NULL, get_proc_cpuset, NULL, topology->levels[0][0]->cpuset);

  hwloc_debug("\nApplying the system cpuset to all nodes\n");
  traverse(topology, &topology->levels[0][0], apply_cpuset, apply_cpuset, NULL, topology->levels[0][0]->cpuset);

  hwloc_debug("\nRemoving ignored objects\n");
  traverse(topology, &topology->levels[0][0], remove_ignored, remove_ignored, NULL, NULL);

  hwloc_debug("\nRemoving empty objects except numa nodes\n");
  traverse(topology, &topology->levels[0][0], remove_empty, remove_empty, NULL, NULL);

  print_objects(topology, 0, topology->levels[0][0]);

  hwloc_debug("\nRemoving objects whose type has HWLOC_IGNORE_TYPE_KEEP_STRUCTURE and have only one child or are the only child\n");
  traverse(topology, &topology->levels[0][0], NULL, NULL, merge_useless_child, NULL);

  hwloc_debug("\nOk, finished tweaking, now connect\n");

  /* Now connect handy pointers.  */

  hwloc_connect(topology->levels[0][0]);

  print_objects(topology, 0, topology->levels[0][0]);

  /* Explore the resulting topology level by level.  */

  /* initialize all depth to unknown */
  for (l=1; l < HWLOC_OBJ_TYPE_MAX; l++)
    topology->type_depth[l] = HWLOC_TYPE_DEPTH_UNKNOWN;
  topology->type_depth[HWLOC_OBJ_SYSTEM] = 0;

  /* Start with children of the whole system.  */
  l = 0;
  n_objs = topology->levels[0][0]->arity;
  objs = malloc(n_objs * sizeof(objs[0]));
  assert(objs);
  memcpy(objs, topology->levels[0][0]->children, n_objs * sizeof(objs[0]));

  /* Keep building levels while there are objects left in OBJS.  */
  while (n_objs) {

    /* First find which type of object is the topmost.  */
    top_obj = objs[0];
    for (i = 1; i < n_objs; i++) {
      if (hwloc_type_cmp(top_obj, objs[i]) != HWLOC_TYPE_EQUAL) {
	if (find_same_type(objs[i], top_obj)) {
	  /* OBJS[i] is strictly above an object of the same type as TOP_OBJ, so it
	   * is above TOP_OBJ.  */
	  top_obj = objs[i];
	}
      }
    }

    /* Now peek all objects of the same type, build a level with that and
     * replace them with their children.  */

    /* First count them.  */
    n_taken_objs = 0;
    n_new_objs = 0;
    for (i = 0; i < n_objs; i++)
      if (hwloc_type_cmp(top_obj, objs[i]) == HWLOC_TYPE_EQUAL) {
	n_taken_objs++;
	n_new_objs += objs[i]->arity;
      }

    /* New level.  */
    taken_objs = malloc((n_taken_objs + 1) * sizeof(taken_objs[0]));
    /* New list of pending objects.  */
    new_objs = malloc((n_objs - n_taken_objs + n_new_objs) * sizeof(new_objs[0]));

    taken_i = 0;
    new_i = 0;
    for (i = 0; i < n_objs; i++)
      if (hwloc_type_cmp(top_obj, objs[i]) == HWLOC_TYPE_EQUAL) {
	/* Take it, add children.  */
	taken_objs[taken_i++] = objs[i];
	for (j = 0; j < objs[i]->arity; j++)
	  new_objs[new_i++] = objs[i]->children[j];
      } else
	/* Leave it.  */
	new_objs[new_i++] = objs[i];


    /* Make sure we didn't mess up.  */
    assert(taken_i == n_taken_objs);
    assert(new_i == n_objs - n_taken_objs + n_new_objs);

    /* Ok, put numbers in the level.  */
    for (i = 0; i < n_taken_objs; i++) {
      taken_objs[i]->depth = topology->nb_levels;
      taken_objs[i]->logical_index = i;
      if (i) {
	taken_objs[i]->prev_cousin = taken_objs[i-1];
	taken_objs[i-1]->next_cousin = taken_objs[i];
      }
    }

    /* One more level!  */
    if (top_obj->type == HWLOC_OBJ_CACHE)
      hwloc_debug("--- Cache level depth %d", top_obj->attr->cache.depth);
    else
      hwloc_debug("--- %s level", hwloc_obj_type_string(top_obj->type));
    hwloc_debug(" has number %d\n\n", topology->nb_levels);

    if (topology->type_depth[top_obj->type] == HWLOC_TYPE_DEPTH_UNKNOWN)
      topology->type_depth[top_obj->type] = topology->nb_levels;
    else
      topology->type_depth[top_obj->type] = HWLOC_TYPE_DEPTH_MULTIPLE; /* mark as unknown */

    taken_objs[n_taken_objs] = NULL;

    topology->level_nbobjects[topology->nb_levels] = n_taken_objs;
    topology->levels[topology->nb_levels] = taken_objs;

    topology->nb_levels++;

    free(objs);
    objs = new_objs;
    n_objs = new_i;
  }

  /* It's empty now.  */
  free(objs);

  if (topology->flags & HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM)
    topology->is_thissystem = 1;

  if (topology->is_thissystem) {
#    ifdef LINUX_SYS
    hwloc_set_linux_hooks(topology);
#    endif /* LINUX_SYS */

#    ifdef  AIX_SYS
    hwloc_set_aix_hooks(topology);
#    endif /* AIX_SYS */

#    ifdef  OSF_SYS
    hwloc_set_osf_hooks(topology);
#    endif /* OSF_SYS */

#    ifdef  SOLARIS_SYS
    hwloc_set_solaris_hooks(topology);
#    endif /* SOLARIS_SYS */

#    ifdef  WIN_SYS
    hwloc_set_windows_hooks(topology);
#    endif /* WIN_SYS */

#    ifdef  DARWIN_SYS
    hwloc_set_darwin_hooks(topology);
#    endif /* DARWIN_SYS */

#    ifdef  HPUX_SYS
    hwloc_set_hpux_hooks(topology);
#    endif /* HPUX_SYS */
  } else {
    topology->set_cpubind = dontset_cpubind;
    topology->set_proc_cpubind = dontset_proc_cpubind;
#ifdef hwloc_thread_t
    topology->set_thread_cpubind = dontset_thread_cpubind;
#endif
    topology->set_thisproc_cpubind = dontset_thisproc_cpubind;
    topology->set_thisthread_cpubind = dontset_thisthread_cpubind;
  }
}

static void
hwloc_topology_setup_defaults(struct hwloc_topology *topology)
{
  struct hwloc_obj *system_obj;
  int i;


  /* No objects by default but System on top by default */
  memset(topology->level_nbobjects, 0, sizeof(topology->level_nbobjects));
  for (i=0; i < HWLOC_OBJ_TYPE_MAX; i++)
    topology->type_depth[i] = HWLOC_TYPE_DEPTH_UNKNOWN;
  topology->nb_levels = 1; /* there's at least SYSTEM */
  topology->levels[0] = malloc (sizeof (struct hwloc_obj));
  topology->level_nbobjects[0] = 1;
  topology->type_depth[HWLOC_OBJ_SYSTEM] = 0;

  /* Create the actual System object */
  system_obj = hwloc_alloc_setup_object(HWLOC_OBJ_SYSTEM, 0);
  system_obj->cpuset = hwloc_cpuset_alloc();
  system_obj->depth = 0;
  system_obj->logical_index = 0;
  system_obj->sibling_rank = 0;
  system_obj->attr->system.memory_kB = 0;
  system_obj->attr->system.huge_page_free = 0;
#ifdef HAVE__SC_LARGE_PAGESIZE
  system_obj->attr->system.huge_page_size_kB = sysconf(_SC_LARGE_PAGESIZE);
#else /* HAVE__SC_LARGE_PAGESIZE */
  system_obj->attr->system.huge_page_size_kB = 0;
#endif /* HAVE__SC_LARGE_PAGESIZE */
  system_obj->attr->system.dmi_board_vendor = NULL;
  system_obj->attr->system.dmi_board_name = NULL;
  hwloc_cpuset_fill(system_obj->cpuset);
  topology->levels[0][0] = system_obj;
}

int
hwloc_topology_init (struct hwloc_topology **topologyp)
{
  struct hwloc_topology *topology;
  int i;

  topology = malloc (sizeof (struct hwloc_topology));
  if(!topology)
    return -1;

  /* Setup topology context */
  topology->is_loaded = 0;
  topology->flags = 0;
  topology->is_thissystem = 1;
  topology->backend_type = HWLOC_BACKEND_NONE; /* backend not specified by default */
  topology->set_cpubind = NULL;
  topology->set_proc_cpubind = NULL;
#ifdef hwloc_thread_t
  topology->set_thread_cpubind = NULL;
#endif
  topology->set_thisproc_cpubind = NULL;
  topology->set_thisthread_cpubind = NULL;
  /* Only ignore useless cruft by default */
  for(i=0; i< HWLOC_OBJ_TYPE_MAX; i++)
    topology->ignored_types[i] = HWLOC_IGNORE_TYPE_NEVER;
  topology->ignored_types[HWLOC_OBJ_MISC] = HWLOC_IGNORE_TYPE_KEEP_STRUCTURE;

  /* Make the topology look like something coherent but empty */
  hwloc_topology_setup_defaults(topology);

  *topologyp = topology;
  return 0;
}

static void
hwloc_backend_exit(struct hwloc_topology *topology)
{
  switch (topology->backend_type) {
#ifdef LINUX_SYS
  case HWLOC_BACKEND_SYSFS:
    hwloc_backend_sysfs_exit(topology);
    break;
#endif
#ifdef HAVE_XML
  case HWLOC_BACKEND_XML:
    hwloc_backend_xml_exit(topology);
    break;
#endif
  case HWLOC_BACKEND_SYNTHETIC:
    hwloc_backend_synthetic_exit(topology);
    break;
  default:
    break;
  }

  assert(topology->backend_type == HWLOC_BACKEND_NONE);
}

int
hwloc_topology_set_fsroot(struct hwloc_topology *topology, const char *fsroot_path)
{
  /* cleanup existing backend */
  hwloc_backend_exit(topology);

#ifdef LINUX_SYS
  if (hwloc_backend_sysfs_init(topology, fsroot_path) < 0)
    return -1;
#endif /* LINUX_SYS */

  return 0;
}

int
hwloc_topology_set_synthetic(struct hwloc_topology *topology, const char *description)
{
  /* cleanup existing backend */
  hwloc_backend_exit(topology);

  return hwloc_backend_synthetic_init(topology, description);
}

int
hwloc_topology_set_xml(struct hwloc_topology *topology, const char *xmlpath)
{
#ifdef HAVE_XML
  /* cleanup existing backend */
  hwloc_backend_exit(topology);

  return hwloc_backend_xml_init(topology, xmlpath);
#else /* HAVE_XML */
  return -1;
#endif /* !HAVE_XML */
}

int
hwloc_topology_set_flags (struct hwloc_topology *topology, unsigned long flags)
{
  topology->flags = flags;
  return 0;
}

int
hwloc_topology_ignore_type(struct hwloc_topology *topology, hwloc_obj_type_t type)
{
  if (type >= HWLOC_OBJ_TYPE_MAX)
    return -1;

  if (type == HWLOC_OBJ_SYSTEM)
    /* we don't want 2 heads */
    return -1;

  if (type == HWLOC_OBJ_PROC)
    /* we need the proc level */
    return -1;

  topology->ignored_types[type] = HWLOC_IGNORE_TYPE_ALWAYS;
  return 0;
}

int
hwloc_topology_ignore_type_keep_structure(struct hwloc_topology *topology, hwloc_obj_type_t type)
{
  if (type >= HWLOC_OBJ_TYPE_MAX)
    return -1;

  if (type == HWLOC_OBJ_SYSTEM)
    /* we don't want 2 heads */
    return -1;

  if (type == HWLOC_OBJ_PROC)
    /* we need the proc level */
    return -1;

  topology->ignored_types[type] = HWLOC_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
hwloc_topology_ignore_all_keep_structure(struct hwloc_topology *topology)
{
  unsigned type;
  for(type=0; type<HWLOC_OBJ_TYPE_MAX; type++)
    if (type != HWLOC_OBJ_SYSTEM && type != HWLOC_OBJ_PROC)
      topology->ignored_types[type] = HWLOC_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

static void
hwloc_topology_clear (struct hwloc_topology *topology)
{
  unsigned l,i;
  for (l=0; l<topology->nb_levels; l++) {
    for (i=0; i<topology->level_nbobjects[l]; i++)
      free_object(topology->levels[l][i]);
    free(topology->levels[l]);
  }
}

void
hwloc_topology_destroy (struct hwloc_topology *topology)
{
  hwloc_topology_clear(topology);
  hwloc_backend_exit(topology);
  free(topology);
}

int
hwloc_topology_load (struct hwloc_topology *topology)
{
  char *local_env;

  if (topology->is_loaded) {
    hwloc_topology_clear(topology);
    hwloc_topology_setup_defaults(topology);
    topology->is_loaded = 0;
  }

  /* enforce backend anyway if a FORCE variable was given */
#ifdef LINUX_SYS
  {
    char *fsroot_path_env = getenv("HWLOC_FORCE_FSROOT");
    if (fsroot_path_env) {
      hwloc_backend_exit(topology);
      hwloc_backend_sysfs_init(topology, fsroot_path_env);
    }
  }
#endif
#ifdef HAVE_XML
  {
    char *xmlpath_env = getenv("HWLOC_FORCE_XMLFILE");
    if (xmlpath_env) {
      hwloc_backend_exit(topology);
      hwloc_backend_xml_init(topology, xmlpath_env);
    }
  }
#endif

  /* only apply non-FORCE variables if we have not changed the backend yet */
#ifdef LINUX_SYS
  if (topology->backend_type == HWLOC_BACKEND_NONE) {
    char *fsroot_path_env = getenv("HWLOC_FSROOT");
    if (fsroot_path_env)
      hwloc_backend_sysfs_init(topology, fsroot_path_env);
  }
#endif
#ifdef HAVE_XML
  if (topology->backend_type == HWLOC_BACKEND_NONE) {
    char *xmlpath_env = getenv("HWLOC_FORCE_XMLFILE");
    if (xmlpath_env)
      hwloc_backend_xml_init(topology, xmlpath_env);
  }
#endif
  if (topology->backend_type == HWLOC_BACKEND_NONE) {
    local_env = getenv("HWLOC_THISSYSTEM");
    if (local_env)
      topology->is_thissystem = atoi(local_env);
  }

  /* if we haven't chosen the backend, set the OS-specific one if needed */
  if (topology->backend_type == HWLOC_BACKEND_NONE) {
#ifdef LINUX_SYS
    if (hwloc_backend_sysfs_init(topology, "/") < 0)
      return -1;
#endif
  }

  /* actual topology discovery */
  hwloc_discover(topology);

  /* enforce THISSYSTEM if given in a FORCE variable */
  local_env = getenv("HWLOC_FORCE_THISSYSTEM");
  if (local_env)
    topology->is_thissystem = atoi(local_env);

#ifndef HWLOC_DEBUG
  if (getenv("HWLOC_DEBUG_CHECK"))
#endif
    hwloc_topology_check(topology);

  topology->is_loaded = 1;
  return 0;
}

int
hwloc_topology_is_thissystem(struct hwloc_topology *topology)
{
  return topology->is_thissystem;
}

unsigned
hwloc_topology_get_depth(struct hwloc_topology *topology) 
{
  return topology->nb_levels;
}


/* check children between a father object */
static void
hwloc__check_children(struct hwloc_topology *topology, struct hwloc_obj *father)
{
  hwloc_cpuset_t remaining_father_set;
  int j;

  if (!father->arity) {
    /* check whether that father has no children for real */
    assert(!father->children);
    assert(!father->first_child);
    assert(!father->last_child);
    return;
  }
  /* check whether that father has children for real */
  assert(father->children);
  assert(father->first_child);
  assert(father->last_child);

  /* first child specific checks */
  assert(father->first_child->sibling_rank == 0);
  assert(father->first_child == father->children[0]);
  assert(father->first_child->prev_sibling == NULL);

  /* last child specific checks */
  assert(father->last_child->sibling_rank == father->arity-1);
  assert(father->last_child == father->children[father->arity-1]);
  assert(father->last_child->next_sibling == NULL);

  remaining_father_set = hwloc_cpuset_dup(father->cpuset);
  for(j=0; j<father->arity; j++) {
    /* check that child cpuset is included in the father */
    assert(hwloc_cpuset_isincluded(father->children[j]->cpuset, remaining_father_set));
    /* check that children are correctly ordered (see below), empty ones may be anywhere */
    if (!hwloc_cpuset_iszero(father->children[j]->cpuset))
      assert(hwloc_cpuset_first(father->children[j]->cpuset) == hwloc_cpuset_first(remaining_father_set));
    /* clear previously used father cpuset bits so that we actually checked above
     * that children cpusets do not intersect and are ordered properly
     */
    hwloc_cpuset_clearset(remaining_father_set, father->children[j]->cpuset);
  }
  assert(hwloc_cpuset_iszero(remaining_father_set));
  free(remaining_father_set);

  /* checks for all children */
  for(j=1; j<father->arity; j++) {
    assert(father->children[j]->sibling_rank == j);
    assert(father->children[j-1]->next_sibling == father->children[j]);
    assert(father->children[j]->prev_sibling == father->children[j-1]);
  }
}

/* check a whole topology structure */
void
hwloc_topology_check(struct hwloc_topology *topology)
{
  struct hwloc_obj *obj;
  int i,j;
  hwloc_obj_type_t type;
  unsigned depth;

  /* check type orders */
  for (type = HWLOC_OBJ_SYSTEM; type < HWLOC_OBJ_TYPE_MAX; type++)
    assert(hwloc_get_order_type(hwloc_get_type_order(type)) == type);
  for (i = hwloc_get_order_type(HWLOC_OBJ_SYSTEM); i <= hwloc_get_order_type(HWLOC_OBJ_CORE); i++)
    assert(hwloc_get_type_order(hwloc_get_order_type(i)) == i);

  /* check that first level is SYSTEM */
  assert(hwloc_get_depth_type(topology, 0) == HWLOC_OBJ_SYSTEM);
  /* check that last level is PROC */
  assert(hwloc_get_depth_type(topology, hwloc_topology_get_depth(topology)-1) == HWLOC_OBJ_PROC);
  /* check that other levels are neither PROC nor SYSTEM */
  for(i=1; i<hwloc_topology_get_depth(topology)-1; i++) {
    assert(hwloc_get_depth_type(topology, i) != HWLOC_OBJ_SYSTEM);
    assert(hwloc_get_depth_type(topology, i) != HWLOC_OBJ_PROC);
  }

  /* top-level specific checks */
  assert(hwloc_get_nbobjs_by_depth(topology, 0) == 1);
  obj = hwloc_get_system_obj(topology);
  assert(obj);
  /* top-level object must be SYSTEM */
  assert(obj->type == HWLOC_OBJ_SYSTEM);

  depth = hwloc_topology_get_depth(topology);

  /* check each level */
  for(i=0; i<depth; i++) {
    unsigned width = hwloc_get_nbobjs_by_depth(topology, i);
    struct hwloc_obj *prev = NULL;

    /* check each object of the level */
    for(j=0; j<width; j++) {
      obj = hwloc_get_obj_by_depth(topology, i, j);
      /* check that the object is corrected placed horizontally and vertically */
      assert(obj);
      assert(obj->depth == i);
      assert(obj->logical_index == j);
      /* check that all objects in the level have the same type */
      if (prev) {
	assert(hwloc_type_cmp(obj, prev) == HWLOC_TYPE_EQUAL);
	assert(prev->next_cousin == obj);
	assert(obj->prev_cousin == prev);
      }
      /* check children */
      hwloc__check_children(topology, obj);
      prev = obj;
    }

    /* check first object of the level */
    obj = hwloc_get_obj_by_depth(topology, i, 0);
    assert(obj);
    assert(!obj->prev_cousin);

    /* check type */
    assert(hwloc_get_depth_type(topology, i) == obj->type);
    assert(hwloc_get_type_depth(topology, obj->type) == i
	   || hwloc_get_type_depth(topology, obj->type) == HWLOC_TYPE_DEPTH_MULTIPLE);

    /* check last object of the level */
    obj = hwloc_get_obj_by_depth(topology, i, width-1);
    assert(obj);
    assert(!obj->next_cousin);

    /* check last+1 object of the level */
    obj = hwloc_get_obj_by_depth(topology, i, width);
    assert(!obj);
  }

  /* check bottom objects */
  assert(hwloc_get_nbobjs_by_depth(topology, depth-1) > 0);
  for(j=0; j<hwloc_get_nbobjs_by_depth(topology, depth-1); j++) {
    obj = hwloc_get_obj_by_depth(topology, depth-1, j);
    assert(obj);
    assert(obj->arity == 0);
    assert(obj->children == NULL);
    /* bottom-level object must always be PROC */
    assert(obj->type == HWLOC_OBJ_PROC);
  }
}
