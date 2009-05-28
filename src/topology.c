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
#include <topology/helper.h>
#include <topology/debug.h>
#include <topology/allocator.h>

#ifdef WIN_SYS
#include <windows.h>
#endif

/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `topo_set_fsys_root ()' to
   have the desired effect.  */
static unsigned
topo_fallback_nbprocessors(void) {
	/* TODO: change into HAVE_SC_foobar, for OSes that don't provide the macro version */
#if defined(_SC_NPROCESSORS_ONLN)
  return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
  return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
  return sysconf(_SC_NPROC_CONF);
	/* TODO: change into HAVE_HOST_INFO */
#elif defined(DARWIN_SYS)
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


/* This is not used any more...  */
#if 0
void
topo_setup_cache_level(int cachelevel, int procid_max,
		       unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes,
		       struct topo_topology *topology)
{
  struct topo_obj **level;
  int j;

  topo_debug("%d L%d caches\n", numcaches[cachelevel], cachelevel+1);
  level = calloc((numcaches[cachelevel]+1), sizeof(*level));
  assert(level);

  for (j = 0; j < numcaches[cachelevel]; j++)
    {
      level[j] = topo_setup_object(TOPO_OBJ_CACHE, j);
      level[j]->memory_kB = cachesizes[cachelevel*TOPO_NBMAXCPUS+j];
      level[j]->cache_depth = cachelevel+1;

      topo_object_cpuset_from_array(level[j], j, &cacheids[cachelevel*TOPO_NBMAXCPUS], procid_max);

      topo_debug("L%d cache %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 cachelevel+1, j, TOPO_CPUSET_PRINTF_VALUE(level[j]->cpuset));
    }
  topo_debug("\n");

  topo_add_level(topology, level, numcaches[cachelevel]);
}
#endif /* LINUX_SYS */

/* Use the value stored in topology->nb_processors,
 * and the optional online cpuset if given.
 */
void
topo_setup_proc_level(struct topo_topology *topology,
		      topo_cpuset_t *online_cpuset)
{
  struct topo_obj **cpu_level;
  unsigned oscpu,cpu;

  cpu_level = calloc(topology->nb_processors+1, sizeof(*cpu_level));
  assert(cpu_level);

  topo_debug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; oscpu++)
    {
      if (online_cpuset && !topo_cpuset_isset(online_cpuset, oscpu))
       continue;

      cpu_level[cpu] = topo_alloc_setup_object(TOPO_OBJ_PROC, oscpu);

      topo_cpuset_cpu(&cpu_level[cpu]->cpuset, oscpu);

      topo_debug("cpu %d (os %d) has cpuset %"TOPO_PRIxCPUSET"\n",
		 cpu, oscpu, TOPO_CPUSET_PRINTF_VALUE(cpu_level[cpu]->cpuset));
      cpu++;
    }

  topo_add_level(topology, cpu_level, topology->nb_processors);
}

/* Just for debugging.  */
static void
print_objects(struct topo_topology *topology, int indent, topo_obj_t obj)
{
  char line[TOPO_CPUSET_STRING_LENGTH];
  topo_debug("%*s", 2*indent, "");
  topo_object_snprintf(line, sizeof(line), topology, obj, "#", 1);
  topo_debug("%s", line);
  topo_cpuset_snprintf(line, sizeof(line), &obj->cpuset);
  topo_debug(" %s", line);
  if (obj->arity)
    topo_debug(" arity %d", obj->arity);
  topo_debug("\n");
  for (obj = obj->first_child; obj; obj = obj->next_sibling)
    print_objects(topology, indent + 1, obj);
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
  [TOPO_OBJ_MACHINE] = 0,
  [TOPO_OBJ_FAKE] = 1,
  [TOPO_OBJ_NODE] = 2,
  [TOPO_OBJ_SOCKET] = 3,
  [TOPO_OBJ_CACHE] = 4,
  [TOPO_OBJ_CORE] = 5,
  [TOPO_OBJ_PROC] = 6,
};

static enum topo_type_cmp_e
topo_type_cmp(topo_obj_t obj1, topo_obj_t obj2)
{
  if (obj_type_order[obj1->type] > obj_type_order[obj2->type])
    return TOPO_TYPE_DEEPER;
  if (obj_type_order[obj1->type] < obj_type_order[obj2->type])
    return TOPO_TYPE_HIGHER;

  /* Caches have the same types but can have different depths.  */
  if (obj1->type == TOPO_OBJ_CACHE) {
    if (obj1->cache_depth < obj2->cache_depth)
      return TOPO_TYPE_DEEPER;
    else if (obj1->cache_depth > obj2->cache_depth)
      return TOPO_TYPE_HIGHER;
  }

  /* Fake objects have the same types but can have different depths.  */
  if (obj1->type == TOPO_OBJ_FAKE) {
    if (obj1->fake_depth < obj2->fake_depth)
      return TOPO_TYPE_DEEPER;
    else if (obj1->fake_depth > obj2->fake_depth)
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
	assert(obj->physical_index == child->physical_index);
	switch(obj->type) {
	  case TOPO_OBJ_NODE:
	    // Do not check these, it may change between calls
	    //check_sizes(obj, child, memory_kB);
	    //check_sizes(obj, child, huge_page_free);
	    break;
	  case TOPO_OBJ_CACHE:
	    check_sizes(obj, child, memory_kB);
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
	/* TODO: could have a topo_cpuset helper to perform the comparison more efficiently.  */
	if (!put && topo_cpuset_first(&obj->cpuset) < topo_cpuset_first(&child->cpuset)) {
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
free_object(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  free(*obj);
}

/* Remove all children whose cpuset is empty, except NUMA nodes
 * since we want to keep memory information.  */
static void
remove_empty(topo_topology_t topology, topo_obj_t *obj, void *data)
{
  if ((*obj)->type != TOPO_OBJ_NODE && topo_cpuset_iszero(&(*obj)->cpuset)) {
    /* Remove empty children */
    traverse(topology, obj, NULL, NULL, free_object, NULL);
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
    child->index = n;
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

  /* Initialize the number of processor to some reasonable default, e.g.,
     obtained using sysconf(3).  */
  topology->nb_processors = topo_fallback_nbprocessors ();

  /* Raw detection, from coarser levels to finer levels for more efficiency.  */
  /* topo_look_* functions should use topo_obj_add to add objects initialized
   * through topo_setup_object. For node levels, memory_Kb and huge_page_free
   * must be initialized. For cache levels, memory_kB and cache_depth must be
   * initialized, for fake levels, fake_depth must be initialized
   */
  /* There must be at least a PROC object for each logical processor, at worse
   * produced by topo_setup_proc_level()  */

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

#    ifdef HAVE_LIBLGRP
  topo_look_lgrp(topology);
#    endif /* HAVE_LIBLGRP */
#    ifdef HAVE_LIBKSTAT
  topo_look_kstat(topology);
#    endif /* HAVE_LIBKSTAT */
#    ifdef  SOLARIS_SYS
#      define HAVE_OS_SUPPORT
  topo_setup_proc_level(topology, NULL);
#    endif /* SOLARIS_SYS */

#    ifdef  WIN_SYS
#      define HAVE_OS_SUPPORT
  topo_look_windows(topology);
#    endif /* WIN_SYS */

#    ifndef HAVE_OS_SUPPORT
  topo_setup_proc_level(topology, NULL);
#    endif /* Unsupported OS */

  topo_debug("%d online processors found\n", topology->nb_processors);
  assert(topology->nb_processors);

  print_objects(topology, 0, topology->levels[0][0]);

  /* First tweak a bit to clean the topology.  */

  topo_debug("\nComputing the system cpuset by ORing all Proc objects\n");
  topo_cpuset_zero(&topology->levels[0][0]->cpuset);
  traverse(topology, &topology->levels[0][0], NULL, get_proc_cpuset, NULL, &topology->levels[0][0]->cpuset);

  topo_debug("\nApplying the machine cpuset to all nodes\n");
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
  topology->type_depth[0] = TOPO_OBJ_MACHINE;

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
      taken_objs[i]->level = topology->nb_levels;
      taken_objs[i]->number = i;
    }

    /* One more level!  */
    if (top_obj->type == TOPO_OBJ_CACHE)
      topo_debug("--- cache level depth %d", top_obj->cache_depth);
    else
      topo_debug("--- %s level", topo_object_type_string(top_obj->type));
    topo_debug(" has number %d\n\n", topology->nb_levels);

    if (topology->type_depth[top_obj->type] == TOPO_TYPE_DEPTH_UNKNOWN)
      topology->type_depth[top_obj->type] = topology->nb_levels;
    else
      topology->type_depth[top_obj->type] = TOPO_TYPE_DEPTH_MULTIPLE; /* mark as unknown */

    taken_objs[n_taken_objs] = NULL;

    topology->level_nbobjects[topology->nb_levels] = n_taken_objs;
    if (top_obj->type == TOPO_OBJ_NODE)
      topology->nb_nodes = n_taken_objs;
    topology->levels[topology->nb_levels] = taken_objs;

    topology->nb_levels++;

    free(objs);
    objs = new_objs;
    n_objs = new_i;
  }

  /* It's empty now.  */
  free(objs);

  /* Setup the depth of all still unknown levels (the ones that got merged or
   * never created).  */
  int type, prevdepth = TOPO_TYPE_DEPTH_UNKNOWN;
  for (type = TOPO_OBJ_MACHINE; type < TOPO_OBJ_FAKE; type++)
    {
      if (topology->type_depth[type] == TOPO_TYPE_DEPTH_UNKNOWN) {
	topology->type_depth[type] = prevdepth;
      } else {
	prevdepth = topology->type_depth[type];
      }
    }
}

int
topo_topology_init (struct topo_topology **topologyp)
{
  struct topo_topology *topology;
  int i;

  topology = topo_default_allocator.allocate (sizeof (struct topo_topology));
  if(!topology)
    return -1;

  topology->nb_processors = 0;
  topology->nb_nodes = 0;
  topology->nb_levels = 1; /* there's at least MACHINE */
  topology->flags = 0;
  topology->is_fake = 0;
  topology->backend_type = TOPO_BACKEND_NONE; /* backend not specified by default */
  topology->huge_page_size_kB = 0;
  topology->dmi_board_vendor = NULL;
  topology->dmi_board_name = NULL;
  for(i=0; i< TOPO_OBJ_TYPE_MAX; i++)
    topology->ignored_types[i] = TOPO_IGNORE_TYPE_NEVER;
  /* Avoid useless cruft */
  topology->ignored_types[TOPO_OBJ_FAKE] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  memset(topology->level_nbobjects, 0, sizeof(topology->level_nbobjects));
  topology->level_nbobjects[0] = 1;
  for (i=0; i < TOPO_OBJ_TYPE_MAX; i++)
    topology->type_depth[i] = -1;
  topology->levels[0] = calloc (2, sizeof (struct topo_obj));
  topo_setup_machine_level (topology->levels[0]);

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
topo_topology_set_flags (struct topo_topology *topology, unsigned long flags)
{
  topology->flags = flags;
  return 0;
}

int
topo_topology_ignore_type(struct topo_topology *topology, enum topo_obj_type_e type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  if (type == TOPO_OBJ_MACHINE)
    /* we don't want 2 heads */
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_ALWAYS;
  return 0;
}

int
topo_topology_ignore_type_keep_structure(struct topo_topology *topology, topo_obj_type_t type)
{
  if (type >= TOPO_OBJ_TYPE_MAX)
    return -1;

  topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_ignore_all_keep_structure(struct topo_topology *topology)
{
  unsigned type;
  for(type=0; type<TOPO_OBJ_TYPE_MAX; type++)
    topology->ignored_types[type] = TOPO_IGNORE_TYPE_KEEP_STRUCTURE;
  return 0;
}

int
topo_topology_load (struct topo_topology *topology)
{
  if (topology->backend_type == TOPO_BACKEND_NONE) {
    /* if we haven't chosen the backend, set the OS-specific one if needed */
#ifdef LINUX_SYS
    if (topo_backend_sysfs_init(topology, "/") < 0)
      return -1;
#endif
  }

  if (topology->backend_type == TOPO_BACKEND_SYNTHETIC)
    topo_synthetic_load(topology);
  else
    topo_discover(topology);

  return 0;
}

int
topo_topology_get_info(struct topo_topology *topology, struct topo_topology_info *info)
{
  info->nb_processors = topology->nb_processors;
  info->nb_nodes = topology->nb_nodes;
  info->depth = topology->nb_levels;
  info->dmi_board_vendor = topology->dmi_board_vendor;
  info->dmi_board_name = topology->dmi_board_name;
  info->huge_page_size_kB = topology->huge_page_size_kB;
  info->is_fake = topology->is_fake;
  return 0;
}

void
topo_topology_destroy (struct topo_topology *topology)
{
  unsigned l,i;

  for (l=0; l<topology->nb_levels; l++) {
    for (i=0; i<topology->level_nbobjects[l]; i++) {
      free(topology->levels[l][i]->children);
      free(topology->levels[l][i]);
    }
    free(topology->levels[l]);
  }

  topo_backend_exit(topology);

  free(topology->dmi_board_vendor);
  free(topology->dmi_board_name);

  topo_default_allocator.free(topology);
}
