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

#include <topology.h>
#include <topology/private.h>
#include <topology/debug.h>

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
   used as a fall-back method, allowing `topo_set_fsys_root ()' to
   have the desired effect.  */
unsigned
topo_fallback_nbprocessors(void) {
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
#warning No known way to discover number of available processors on this system
#warning topo_fallback_nbprocessors will default to 1
  return 1;
#endif
}

/*
 * Place objects in groups if they are in complete graphs with minimal distances.
 * Return how many groups were created, or 0 if some incomplete distance graphs were found.
 */
static unsigned
topo_setup_group_from_min_distance_clique(struct topo_obj **objs,
					  unsigned nbobjs,
					  unsigned *distances,
					  unsigned *groupids)
{
  unsigned groupid = 0;
  unsigned i,j,k;

  memset(groupids, 0, nbobjs*sizeof(*groupids));

  /* try to find complete graphs */
  for(i=0; i<nbobjs; i++) {
    topo_cpuset_t closest_objs_set = TOPO_CPUSET_ZERO;
    unsigned min_distance = -1;
    unsigned size = 1; /* current object i */

    /* if already grouped, skip */
    if (groupids[i])
      continue;

    /* find closest nodes */
    for(j=i+1; j<nbobjs; j++) {
      if (distances[i*nbobjs+j] < min_distance) {
	/* reset the closest set and use new min_distance */
	topo_cpuset_cpu(&closest_objs_set, j);
	min_distance = distances[i*nbobjs+j];
	size = 2; /* current objects i and j */
      } else if (distances[i*nbobjs+j] == min_distance) {
	/* add object to current closest set */
	topo_cpuset_set(&closest_objs_set, j);
	size++;
      }
    }
    /* check that we actually have a complete graph between these closest objects */
    for (j=i+1; j<nbobjs; j++)
      for (k=j+1; k<nbobjs; k++)
	if (topo_cpuset_isset(&closest_objs_set, j) &&
	    topo_cpuset_isset(&closest_objs_set, k) &&
	    distances[j*nbobjs+k] != min_distance) {
	  /* the minimal-distance graph is not complete. abort */
	  topo_debug("found incomplete minimal-distance graph, aborting\n");
	  return 0;
	}

    /* fill a new group */
    groupid++;
    groupids[i] = groupid;
    for(j=i+1; j<nbobjs; j++)
      if (topo_cpuset_isset(&closest_objs_set, j))
	groupids[j] = groupid;
    topo_debug("found complete graph with %u objects with minimal distance %u\n",
	       size, min_distance);
  }

  /* return the last id, since it's also the number of used group ids */
  return groupid;
}

/*
 * Look at object physical distances to group them.
 */
void
topo_setup_misc_level_from_distances(struct topo_topology *topology,
				     struct topo_obj **objs,
				     unsigned nbobjs,
				     unsigned *distances)
{
  unsigned *groupids;
  int nbgroups;
  unsigned i,j;

  if (getenv("TOPO_IGNORE_DISTANCES"))
    return;

  topo_debug("trying to group %s objects into misc objects according to physical distances\n",
	     topo_obj_type_string(objs[0]->type));

  /* check that the matrix is ok */
  for(i=0; i<nbobjs; i++) {
    for(j=i+1; j<nbobjs; j++) {
      /* should be symmetric */
      if (distances[i*nbobjs+j] != distances[j*nbobjs+i]) {
	topo_debug("distance matrix asymmetric ([%u,%u]=%u != [%u,%u]=%u), aborting\n",
		   i, j, distances[i*nbobjs+j], j, i, distances[j*nbobjs+i]);
	return;
      }
      /* diagonal is smaller than everything else */
      if (distances[i*nbobjs+j] <= distances[i*nbobjs+i]) {
	topo_debug("distance to self not strictly minimal ([%u,%u]=%u <= [%u,%u]=%u), aborting\n",
		   i, j, distances[i*nbobjs+j], i, i, distances[i*nbobjs+i]);
	return;
      }
    }
  }

  groupids = malloc(nbobjs*sizeof(*groupids));
  if (!groupids)
    goto out;

  nbgroups = topo_setup_group_from_min_distance_clique(objs, nbobjs, distances, groupids);
  if (!nbgroups) {
    /* FIXME: try relaxed strategies such as minimal-distance with transitivity instead of complete graph */
    goto out_with_groupids;
  }

  if (nbgroups == 1) {
    topo_debug("ignoring misc object with all objects\n");
    goto out_with_groupids;
  }

  for(i=0; i<nbgroups; i++) {
    /* create the misc object */
    topo_obj_t misc_obj;
    misc_obj = topo_alloc_setup_object(TOPO_OBJ_MISC, -1);
    for (j=0; j<nbobjs; j++)
      if (groupids[j] == i+1)
	topo_cpuset_orset(&misc_obj->cpuset, &objs[j]->cpuset);
    topo_debug("adding misc object with cpuset %"TOPO_PRIxCPUSET"\n",
	       TOPO_CPUSET_PRINTF_VALUE(&misc_obj->cpuset));
    topo_add_object(topology, misc_obj);
  }

  /* FIXME: factorize distances and invoke recursively? */

 out_with_groupids:
  free(groupids);
 out:
  return;
}

/*
 * Use the given number of processors and the optional online cpuset if given
 * to set a Proc level.
 */
void
topo_setup_proc_level(struct topo_topology *topology,
		      unsigned nb_processors,
		      topo_cpuset_t *online_cpuset)
{
  struct topo_obj *obj;
  unsigned oscpu,cpu;

  topo_debug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<nb_processors; oscpu++)
    {
      if (online_cpuset && !topo_cpuset_isset(online_cpuset, oscpu))
       continue;

      obj = topo_alloc_setup_object(TOPO_OBJ_PROC, oscpu);

      topo_cpuset_cpu(&obj->cpuset, oscpu);

      topo_debug("cpu %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
      topo_add_object(topology, obj);

      cpu++;
    }
}

/* Just for debugging.  */
static void
print_objects(struct topo_topology *topology, int indent, topo_obj_t obj)
{
  char line[TOPO_CPUSET_STRING_LENGTH+1];
  topo_debug("%*s", 2*indent, "");
  topo_obj_snprintf(line, sizeof(line), topology, obj, "#", 1);
  topo_debug("%s", line);
  topo_cpuset_snprintf(line, sizeof(line), &obj->cpuset);
  topo_debug(" %s", line);
  if (obj->arity)
    topo_debug(" arity %d", obj->arity);
  topo_debug("\n");
  for (obj = obj->first_child; obj; obj = obj->next_sibling)
    print_objects(topology, indent + 1, obj);
}

/* Free an object and all its content.  */
static void
free_object(topo_obj_t obj)
{
  switch (obj->type) {
  case TOPO_OBJ_SYSTEM:
    free(obj->attr->system.dmi_board_vendor);
    free(obj->attr->system.dmi_board_name);
    break;
  case TOPO_OBJ_MACHINE:
    free(obj->attr->machine.dmi_board_vendor);
    free(obj->attr->machine.dmi_board_name);
    break;
  default:
    break;
  }
  free(obj->attr);
  free(obj->children);
  free(obj);
}

/*
 * How to compare objects based on types.
 *
 * Note that HIGHER/LOWER is only a (consistent) heuristic, used to sort
 * objects with same cpuset consistently.
 * Only EQUAL / not EQUAL can be relied upon.
 */

enum topo_type_cmp_e {
  TOPO_TYPE_HIGHER,
  TOPO_TYPE_DEEPER,
  TOPO_TYPE_EQUAL,
};

static const int obj_type_order[] = {
  [TOPO_OBJ_SYSTEM] = 0,
  [TOPO_OBJ_MACHINE] = 1,
  [TOPO_OBJ_MISC] = 2,
  [TOPO_OBJ_NODE] = 3,
  [TOPO_OBJ_SOCKET] = 4,
  [TOPO_OBJ_CACHE] = 5,
  [TOPO_OBJ_CORE] = 6,
  [TOPO_OBJ_PROC] = 7,
};

static const topo_obj_type_t obj_order_type[] = {
  [0] = TOPO_OBJ_SYSTEM,
  [1] = TOPO_OBJ_MACHINE,
  [2] = TOPO_OBJ_MISC,
  [3] = TOPO_OBJ_NODE,
  [4] = TOPO_OBJ_SOCKET,
  [5] = TOPO_OBJ_CACHE,
  [6] = TOPO_OBJ_CORE,
  [7] = TOPO_OBJ_PROC,
};

int topo_get_type_order(topo_obj_type_t type)
{
  return obj_type_order[type];
}

topo_obj_type_t topo_get_order_type(int order)
{
  return obj_order_type[order];
}

static enum topo_type_cmp_e
topo_type_cmp(topo_obj_t obj1, topo_obj_t obj2)
{
  if (topo_get_type_order(obj1->type) > topo_get_type_order(obj2->type))
    return TOPO_TYPE_DEEPER;
  if (topo_get_type_order(obj1->type) < topo_get_type_order(obj2->type))
    return TOPO_TYPE_HIGHER;

  /* Caches have the same types but can have different depths.  */
  if (obj1->type == TOPO_OBJ_CACHE) {
    if (obj1->attr->cache.depth < obj2->attr->cache.depth)
      return TOPO_TYPE_DEEPER;
    else if (obj1->attr->cache.depth > obj2->attr->cache.depth)
      return TOPO_TYPE_HIGHER;
  }

  /* Misc objects have the same types but can have different depths.  */
  if (obj1->type == TOPO_OBJ_MISC) {
    if (obj1->attr->misc.depth < obj2->attr->misc.depth)
      return TOPO_TYPE_DEEPER;
    else if (obj1->attr->misc.depth > obj2->attr->misc.depth)
      return TOPO_TYPE_HIGHER;
  }

  return TOPO_TYPE_EQUAL;
}

/*
 * How to compare objects based on cpusets.
 */

enum topo_obj_cmp_e {
  TOPO_OBJ_EQUAL,	/**< \brief Equal */
  TOPO_OBJ_INCLUDED,	/**< \brief Strictly included into */
  TOPO_OBJ_CONTAINS,	/**< \brief Strictly contains */
  TOPO_OBJ_INTERSECTS,	/**< \brief Intersects, but no inclusion! */
  TOPO_OBJ_DIFFERENT,	/**< \brief No intersection */
};

static int
topo_obj_cmp(topo_obj_t obj1, topo_obj_t obj2)
{
  if (topo_cpuset_iszero(&obj1->cpuset) || topo_cpuset_iszero(&obj2->cpuset))
    return TOPO_OBJ_DIFFERENT;

  if (topo_cpuset_isequal(&obj1->cpuset, &obj2->cpuset)) {

    /* Same cpuset, subsort by type to have a consistent ordering.  */

    switch (topo_type_cmp(obj1, obj2)) {
      case TOPO_TYPE_DEEPER:
	return TOPO_OBJ_INCLUDED;
      case TOPO_TYPE_HIGHER:
	return TOPO_OBJ_CONTAINS;
      case TOPO_TYPE_EQUAL:
	/* Same level cpuset and type!  Let's hope it's coherent.  */
	return TOPO_OBJ_EQUAL;
    }

    /* For dumb compilers */
    abort();

  } else {

    /* Different cpusets, sort by inclusion.  */

    if (topo_cpuset_isincluded(&obj1->cpuset, &obj2->cpuset))
      return TOPO_OBJ_INCLUDED;

    if (topo_cpuset_isincluded(&obj2->cpuset, &obj1->cpuset))
      return TOPO_OBJ_CONTAINS;

    if (topo_cpuset_intersects(&obj1->cpuset, &obj2->cpuset))
      return TOPO_OBJ_INTERSECTS;

    return TOPO_OBJ_DIFFERENT;
  }
}

/*
 * How to insert objects into the topology.
 *
 * Note: during detection, only the first_child and next_sibling pointers are
 * kept up to date.  Others are computed only once topology detection is
 * complete.
 */

#define check_sizes(obj1, obj2, field) \
  assert(!(obj1)->field || !(obj1)->field || obj1->field == obj2->field)

/* Try to insert OBJ in CUR, recurse if needed */
static void
add_object(struct topo_topology *topology, topo_obj_t cur, topo_obj_t obj)
{
  topo_obj_t child, container, *cur_children, *obj_children, next_child;
  int put;

  /* Make sure we haven't gone too deep.  */
  assert(topo_cpuset_isincluded(&obj->cpuset, &cur->cpuset));

  /* Check whether OBJ is included in some child.  */
  container = NULL;
  for (child = cur->first_child; child; child = child->next_sibling) {
    switch (topo_obj_cmp(obj, child)) {
      case TOPO_OBJ_EQUAL:
	assert(topo_cpuset_isequal(&obj->cpuset, &child->cpuset));
	assert(obj->os_index == child->os_index);
	switch(obj->type) {
	  case TOPO_OBJ_NODE:
	    // Do not check these, it may change between calls
	    //check_sizes(obj, child, attr->node.memory_kB);
	    //check_sizes(obj, child, attr->node.huge_page_free);
	    break;
	  case TOPO_OBJ_CACHE:
	    check_sizes(obj, child, attr->cache.memory_kB);
	    break;
	  default:
	    break;
	}
	/* Already present, no need to insert.  */
	return;
      case TOPO_OBJ_INCLUDED:
	if (container) {
	  /* TODO: how to report?  */
	  fprintf(stderr, "object included in several different objects!\n");
	  /* We can't handle that.  */
	  return;
	}
	/* This child contains OBJ.  */
	container = child;
	break;
      case TOPO_OBJ_INTERSECTS:
	/* TODO: how to report?  */
	fprintf(stderr, "object intersection without inclusion!\n");
	/* We can't handle that.  */
	return;
      case TOPO_OBJ_CONTAINS:
	/* OBJ will be above CHILD.  */
	break;
      case TOPO_OBJ_DIFFERENT:
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
  for (child = cur->first_child, child ? next_child = child->next_sibling : 0;
       child;
       child = next_child, child ? next_child = child->next_sibling : 0) {

    switch (topo_obj_cmp(obj, child)) {

      case TOPO_OBJ_DIFFERENT:
	/* Leave CHILD in CUR.  */
	if (!put && topo_cpuset_compar_first(&obj->cpuset, &child->cpuset) < 0) {
	  /* Sort children by cpuset: put OBJ before CHILD in CUR's children.  */
	  *cur_children = obj;
	  cur_children = &obj->next_sibling;
	  put = 1;
	}
	/* Now put CHILD in CUR's children.  */
	*cur_children = child;
	cur_children = &child->next_sibling;
	break;

      case TOPO_OBJ_CONTAINS:
	/* OBJ contains CHILD, put the latter in the former.  */
	*obj_children = child;
	obj_children = &child->next_sibling;
	break;

      case TOPO_OBJ_EQUAL:
      case TOPO_OBJ_INCLUDED:
      case TOPO_OBJ_INTERSECTS:
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
topo_add_object(struct topo_topology *topology, topo_obj_t obj)
{
  if (topology->ignored_types[obj->type] == TOPO_IGNORE_TYPE_ALWAYS)
    return;

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
traverse(topo_topology_t topology,
	 topo_obj_t *father,
	 void (*node_before)(topo_topology_t topology, topo_obj_t *obj, void *),
	 void (*leaf)(topo_topology_t topology, topo_obj_t *obj, void *),
	 void (*node_after)(topo_topology_t topology, topo_obj_t *obj, void *),
	 void *data)
{
  topo_obj_t *pobj, obj;

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
       (*pobj == obj ? pobj = &(*pobj)->next_sibling : 0),
       /* Get pointer to next object.  */
	obj = *pobj)
    traverse(topology, pobj, node_before, leaf, node_after, data);
  if (node_after)
    node_after(topology, father, data);
}

static void
get_proc_cpuset(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  topo_cpuset_t *cpuset = data;
  if ((*obj)->type != TOPO_OBJ_PROC)
    return;
  topo_cpuset_orset(cpuset, &(*obj)->cpuset);
}

static void
apply_cpuset(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  topo_cpuset_t *cpuset = data;
  topo_cpuset_andset(&(*obj)->cpuset, cpuset);
}

static void
do_free_object(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  free_object(*obj);
}

/* Remove all children whose cpuset is empty, except NUMA nodes
 * since we want to keep memory information.  */
static void
remove_empty(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  if ((*obj)->type != TOPO_OBJ_NODE && topo_cpuset_iszero(&(*obj)->cpuset)) {
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
merge_useless_child(topo_topology_t topology, topo_obj_t *pfather, void *data)
{
  topo_obj_t father = *pfather, child = father->first_child;
  if (child->next_sibling)
    /* There are several children, it's useful to keep them.  */
    return;

  /* TODO: have a preference order?  */
  if (topology->ignored_types[father->type] == TOPO_IGNORE_TYPE_KEEP_STRUCTURE) {
    /* Father can be ignored in favor of the child.  */
    *pfather = child;
    child->next_sibling = father->next_sibling;
    free(father);
  } else if (topology->ignored_types[child->type] == TOPO_IGNORE_TYPE_KEEP_STRUCTURE) {
    /* Child can be ignored in favor of the father.  */
    father->first_child = child->first_child;
    free(child);
  }
}

/*
 * Initialize handy pointers in the whole topology
 */
static void
topo_connect(topo_obj_t father)
{
  unsigned n;
  topo_obj_t child, prev_child = NULL;

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
    topo_connect(child);
  }
}

/*
 * Check whether there is an object below ROOT that has the same type as OBJ
 */
static int
find_same_type(topo_obj_t root, topo_obj_t obj)
{
  topo_obj_t child;

  if (topo_type_cmp(root, obj) == TOPO_TYPE_EQUAL)
    return 1;

  for (child = root->first_child; child; child = child->next_sibling)
    if (find_same_type(child, obj))
      return 1;

  return 0;
}

/* Main discovery loop */
static void
topo_discover(struct topo_topology *topology)
{
  unsigned l, i=0, taken_i, new_i, j;
  topo_obj_t *objs, *taken_objs, *new_objs, top_obj;
  unsigned n_objs, n_taken_objs, n_new_objs;

  assert(topology!=NULL);

  if (topology->backend_type == TOPO_BACKEND_SYNTHETIC) {
    topo_look_synthetic(topology);
#ifdef HAVE_XML
  } else if (topology->backend_type == TOPO_BACKEND_XML) {
    topo_look_xml(topology);
#endif
  } else {

  /* Raw detection, from coarser levels to finer levels for more efficiency.  */

  /* topo_look_* functions should use topo_obj_add to add objects initialized
   * through topo_setup_object. For node levels, memory_Kb and huge_page_free
   * must be initialized. For cache levels, memory_kB and attr->cache.depth must be
   * initialized, for misc levels, attr->misc.depth must be initialized
   */

  /* There must be at least a PROC object for each logical processor, at worse
   * produced by topo_setup_proc_level()
   */

  /* Each OS type should also fill the bind functions pointers, at least the
   * set_cpubind one
   */

#    ifdef LINUX_SYS
#      define HAVE_OS_SUPPORT
    topo_look_linux(topology);
#    endif /* LINUX_SYS */

#    ifdef  AIX_SYS
#      define HAVE_OS_SUPPORT
    topo_look_aix(topology);
#    endif /* AIX_SYS */

#    ifdef  OSF_SYS
#      define HAVE_OS_SUPPORT
    topo_look_osf(topology);
#    endif /* OSF_SYS */

#    ifdef  SOLARIS_SYS
#      define HAVE_OS_SUPPORT
    topo_look_solaris(topology);
#    endif /* SOLARIS_SYS */

#    ifdef  WIN_SYS
#      define HAVE_OS_SUPPORT
    topo_look_windows(topology);
#    endif /* WIN_SYS */

#    ifdef  DARWIN_SYS
#      define HAVE_OS_SUPPORT
    topo_look_darwin(topology);
#    endif /* DARWIN_SYS */

#    ifndef HAVE_OS_SUPPORT
    topo_setup_proc_level(topology, topo_fallback_nbprocessors (), NULL);
#    endif /* Unsupported OS */
  }

  print_objects(topology, 0, topology->levels[0][0]);

  /* First tweak a bit to clean the topology.  */

  topo_debug("\nComputing the system cpuset by ORing all Proc objects\n");
  topo_cpuset_zero(&topology->levels[0][0]->cpuset);
  traverse(topology, &topology->levels[0][0], NULL, get_proc_cpuset, NULL, &topology->levels[0][0]->cpuset);

  topo_debug("\nApplying the system cpuset to all nodes\n");
  traverse(topology, &topology->levels[0][0], apply_cpuset, apply_cpuset, NULL, &topology->levels[0][0]->cpuset);

  topo_debug("\nRemoving empty objects except numa nodes\n");
  traverse(topology, &topology->levels[0][0], remove_empty, remove_empty, NULL, NULL);

  print_objects(topology, 0, topology->levels[0][0]);

  topo_debug("\nRemoving objects whose type has TOPO_IGNORE_TYPE_KEEP_STRUCTURE and have only one child or are the only child\n");
  traverse(topology, &topology->levels[0][0], NULL, NULL, merge_useless_child, NULL);

  topo_debug("\nOk, finished tweaking, now connect\n");

  /* Now connect handy pointers.  */

  topo_connect(topology->levels[0][0]);

  print_objects(topology, 0, topology->levels[0][0]);

  /* Explore the resulting topology level by level.  */

  /* initialize all depth to unknown */
  for (l=1; l < TOPO_OBJ_TYPE_MAX; l++)
    topology->type_depth[l] = TOPO_TYPE_DEPTH_UNKNOWN;
  topology->type_depth[TOPO_OBJ_SYSTEM] = 0;

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
      if (topo_type_cmp(top_obj, objs[i]) != TOPO_TYPE_EQUAL) {
	if (find_same_type(top_obj, objs[i])) {
	  /* OBJTOP is strictly above an object of the same type as OBJ, so it
	   * is above OBJ.  */
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
      if (topo_type_cmp(top_obj, objs[i]) == TOPO_TYPE_EQUAL) {
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
      if (topo_type_cmp(top_obj, objs[i]) == TOPO_TYPE_EQUAL) {
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
    if (top_obj->type == TOPO_OBJ_CACHE)
      topo_debug("--- cache level depth %d", top_obj->attr->cache.depth);
    else
      topo_debug("--- %s level", topo_obj_type_string(top_obj->type));
    topo_debug(" has number %d\n\n", topology->nb_levels);

    if (topology->type_depth[top_obj->type] == TOPO_TYPE_DEPTH_UNKNOWN)
      topology->type_depth[top_obj->type] = topology->nb_levels;
    else
      topology->type_depth[top_obj->type] = TOPO_TYPE_DEPTH_MULTIPLE; /* mark as unknown */

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
}

static void
topo_topology_setup_defaults(struct topo_topology *topology)
{
  struct topo_obj *system_obj;
  int i;


  /* No objects by default but System on top by default */
  memset(topology->level_nbobjects, 0, sizeof(topology->level_nbobjects));
  for (i=0; i < TOPO_OBJ_TYPE_MAX; i++)
    topology->type_depth[i] = TOPO_TYPE_DEPTH_UNKNOWN;
  topology->nb_levels = 1; /* there's at least SYSTEM */
  topology->levels[0] = malloc (sizeof (struct topo_obj));
  topology->level_nbobjects[0] = 1;
  topology->type_depth[TOPO_OBJ_SYSTEM] = 0;

  /* Create the actual System object */
  system_obj = topo_alloc_setup_object(TOPO_OBJ_SYSTEM, 0);
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
  topo_cpuset_fill(&system_obj->cpuset);
  topology->levels[0][0] = system_obj;
}

int
topo_topology_init (struct topo_topology **topologyp)
{
  struct topo_topology *topology;
  int i;

  topology = malloc (sizeof (struct topo_topology));
  if(!topology)
    return -1;

  /* Setup topology context */
  topology->is_loaded = 0;
  topology->flags = 0;
  topology->is_fake = 0;
  topology->backend_type = TOPO_BACKEND_NONE; /* backend not specified by default */
  topology->set_cpubind = NULL;
  topology->set_proc_cpubind = NULL;
  topology->set_thread_cpubind = NULL;
  topology->set_thisproc_cpubind = NULL;
  topology->set_thisthread_cpubind = NULL;
  /* Only ignore useless cruft by default */
  for(i=0; i< TOPO_OBJ_TYPE_MAX; i++)
    topology->ignored_types[i] = TOPO_IGNORE_TYPE_NEVER;
  topology->ignored_types[TOPO_OBJ_MISC] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;

  /* Make the topology look like something coherent but empty */
  topo_topology_setup_defaults(topology);

  *topologyp = topology;
  return 0;
}

static void
topo_backend_exit(struct topo_topology *topology)
{
  switch (topology->backend_type) {
#ifdef LINUX_SYS
  case TOPO_BACKEND_SYSFS:
    topo_backend_sysfs_exit(topology);
    break;
#endif
#ifdef HAVE_XML
  case TOPO_BACKEND_XML:
    topo_backend_xml_exit(topology);
    break;
#endif
  case TOPO_BACKEND_SYNTHETIC:
    topo_backend_synthetic_exit(topology);
    break;
  default:
    break;
  }

  assert(topology->backend_type == TOPO_BACKEND_NONE);
}

int
topo_topology_set_fsys_root(struct topo_topology *topology, const char *fsys_root_path)
{
  /* cleanup existing backend */
  topo_backend_exit(topology);

#ifdef LINUX_SYS
  if (topo_backend_sysfs_init(topology, fsys_root_path) < 0)
    return -1;
#endif /* LINUX_SYS */

  return 0;
}

int
topo_topology_set_synthetic(struct topo_topology *topology, const char *description)
{
  /* cleanup existing backend */
  topo_backend_exit(topology);

  return topo_backend_synthetic_init(topology, description);
}

int
topo_topology_set_xml(struct topo_topology *topology, const char *xmlpath)
{
#ifdef HAVE_XML
  /* cleanup existing backend */
  topo_backend_exit(topology);

  return topo_backend_xml_init(topology, xmlpath);
#else /* HAVE_XML */
  return -1;
#endif /* !HAVE_XML */
}

int
topo_topology_set_flags (struct topo_topology *topology, unsigned long flags)
{
  topology->flags = flags;
  return 0;
}

int
topo_topology_ignore_type(struct topo_topology *topology, topo_obj_type_t type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  if (type == TOPO_OBJ_SYSTEM)
    /* we don't want 2 heads */
    return -1;

  if (type == TOPO_OBJ_PROC)
    /* we need the proc level */
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_ALWAYS;
  return 0;
}

int
topo_topology_ignore_type_keep_structure(struct topo_topology *topology, topo_obj_type_t type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  if (type == TOPO_OBJ_SYSTEM)
    /* we don't want 2 heads */
    return -1;

  if (type == TOPO_OBJ_PROC)
    /* we need the proc level */
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_ignore_all_keep_structure(struct topo_topology *topology)
{
  unsigned type;
  for(type=0; type<TOPO_OBJ_TYPE_MAX; type++)
    if (type != TOPO_OBJ_SYSTEM && type != TOPO_OBJ_PROC)
      topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

static void
topo_topology_clear (struct topo_topology *topology)
{
  unsigned l,i;
  for (l=0; l<topology->nb_levels; l++) {
    for (i=0; i<topology->level_nbobjects[l]; i++)
      free_object(topology->levels[l][i]);
    free(topology->levels[l]);
  }
}

void
topo_topology_destroy (struct topo_topology *topology)
{
  topo_topology_clear(topology);
  topo_backend_exit(topology);
  free(topology);
}

int
topo_topology_load (struct topo_topology *topology)
{
  if (topology->is_loaded) {
    topo_topology_clear(topology);
    topo_topology_setup_defaults(topology);
    topology->is_loaded = 0;
  }

#ifdef LINUX_SYS
  char *fsys_root_path_env = getenv("TOPO_FSYS_ROOT_PATH");
  if (fsys_root_path_env) {
    topo_backend_exit(topology);
    topo_backend_sysfs_init(topology, fsys_root_path_env);
  }
#endif
#ifdef HAVE_XML
  char *xmlpath_env = getenv("TOPO_XML_PATH");
  if (xmlpath_env) {
    topo_backend_exit(topology);
    topo_backend_xml_init(topology, xmlpath_env);
  }
#endif

  if (topology->backend_type == TOPO_BACKEND_NONE) {
    /* if we haven't chosen the backend, set the OS-specific one if needed */
#ifdef LINUX_SYS
    if (topo_backend_sysfs_init(topology, "/") < 0)
      return -1;
#endif
  }

  topo_discover(topology);

#ifndef TOPO_DEBUG
  if (getenv("TOPO_DEBUG_CHECK"))
#endif
    topo_topology_check(topology);

  topology->is_loaded = 1;
  return 0;
}

int
topo_topology_get_info(struct topo_topology *topology, struct topo_topology_info *info)
{
  info->depth = topology->nb_levels;
  info->is_fake = topology->is_fake;
  return 0;
}


/* check children between a father object */
static void
topo__check_children(struct topo_topology *topology, struct topo_obj *father)
{
  int j;

  if (!father->arity) {
    assert(!father->children);
    assert(!father->first_child);
    assert(!father->last_child);
    return;
  }

  /* first child specific checks */
  assert(father->first_child->sibling_rank == 0);
  assert(father->first_child == father->children[0]);
  assert(father->first_child->prev_sibling == NULL);

  /* last child specific checks */
  assert(father->last_child->sibling_rank == father->arity-1);
  assert(father->last_child == father->children[father->arity-1]);
  assert(father->last_child->next_sibling == NULL);

  /* checks for all children */
  for(j=1; j<father->arity; j++) {
    assert(father->children[j]->sibling_rank == j);
    assert(father->children[j-1]->next_sibling == father->children[j]);
    assert(father->children[j]->prev_sibling == father->children[j-1]);
    assert(father->children[j-1]->next_cousin == father->children[j]);
    assert(father->children[j]->prev_cousin == father->children[j-1]);
    /* don't check that types are equal, in case of asymmetric topologies? */
  }
}

/* check a whole topology structure */
void
topo_topology_check(struct topo_topology *topology)
{
  struct topo_topology_info info;
  struct topo_obj *obj;
  int i,j;
  topo_obj_type_t type;

  for (type = TOPO_OBJ_SYSTEM; type < TOPO_OBJ_TYPE_MAX; type++)
    assert(topo_get_order_type(topo_get_type_order(type)) == type);
  for (i = topo_get_order_type(TOPO_OBJ_SYSTEM); i <= topo_get_order_type(TOPO_OBJ_CORE); i++)
    assert(topo_get_type_order(topo_get_order_type(i)) == i);

  assert(topo_topology_get_info(topology, &info) >= 0);

  /* top-level specific checks */
  assert(topo_get_depth_nbobjs(topology, 0) == 1);
  obj = topo_get_system_obj(topology);
  assert(obj);
  assert(obj->type == TOPO_OBJ_SYSTEM);

  /* check each level */
  for(i=0; i<info.depth; i++) {
    unsigned width = topo_get_depth_nbobjs(topology, i);
    struct topo_obj *prev = NULL;

    /* check that each object is equal to the previous one */
    for(j=0; j<width; j++) {
      obj = topo_get_obj_by_depth(topology, i, j);
      assert(obj);
      assert(obj->depth == i);
      assert(obj->logical_index == j);
      if (prev) {
	assert(topo_type_cmp(obj, prev) == TOPO_TYPE_EQUAL);
	assert(prev->next_cousin == obj);
	assert(obj->prev_cousin == prev);
      }
      topo__check_children(topology, obj);
      prev = obj;
    }

    /* check first object of the level */
    obj = topo_get_obj_by_depth(topology, i, 0);
    assert(obj);
    assert(!obj->prev_cousin);

    /* check type */
    assert(topo_get_depth_type(topology, i) == obj->type);
    assert(topo_get_type_depth(topology, obj->type) == i
	   || topo_get_type_depth(topology, obj->type) == TOPO_TYPE_DEPTH_MULTIPLE);

    /* check last object */
    obj = topo_get_obj_by_depth(topology, i, width-1);
    assert(obj);
    assert(!obj->next_cousin);

    /* check last+1 object */
    obj = topo_get_obj_by_depth(topology, i, width);
    assert(!obj);
  }

  /* check bottom objects */
  assert(topo_get_depth_nbobjs(topology, info.depth-1) > 0);
  for(j=0; j<topo_get_depth_nbobjs(topology, info.depth-1); j++) {
    obj = topo_get_obj_by_depth(topology, info.depth-1, j);
    assert(obj);
    assert(obj->type == TOPO_OBJ_PROC);
    assert(obj->arity == 0);
    assert(obj->children == NULL);
  }
}
