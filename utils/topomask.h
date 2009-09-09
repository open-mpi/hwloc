/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#ifndef TOPOMASK_H
#define TOPOMASK_H

#include <hwloc.h>
#include <private/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef enum topomask_append_mode_e {
  TOPOMASK_APPEND_ADD,
  TOPOMASK_APPEND_CLR,
  TOPOMASK_APPEND_AND,
  TOPOMASK_APPEND_XOR,
} topomask_append_mode_t;

static __inline__ void
topomask_append_cpuset(topo_cpuset_t *set, topo_cpuset_t *newset,
		       topomask_append_mode_t mode, int verbose)
{
  switch (mode) {
  case TOPOMASK_APPEND_ADD:
    if (verbose)
      fprintf(stderr, "adding %" TOPO_PRIxCPUSET " to %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(newset), TOPO_CPUSET_PRINTF_VALUE(set));
    topo_cpuset_orset(set, newset);
    break;
  case TOPOMASK_APPEND_CLR:
    if (verbose)
      fprintf(stderr, "clearing %" TOPO_PRIxCPUSET " from %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(newset), TOPO_CPUSET_PRINTF_VALUE(set));
    topo_cpuset_clearset(set, newset);
    break;
  case TOPOMASK_APPEND_AND:
    if (verbose)
      fprintf(stderr, "and'ing %" TOPO_PRIxCPUSET " from %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(newset), TOPO_CPUSET_PRINTF_VALUE(set));
    topo_cpuset_andset(set, newset);
    break;
  case TOPOMASK_APPEND_XOR:
    if (verbose)
      fprintf(stderr, "xor'ing %" TOPO_PRIxCPUSET " from %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(newset), TOPO_CPUSET_PRINTF_VALUE(set));
    topo_cpuset_xorset(set, newset);
    break;
  default:
    assert(1);
  }
}

static __inline__ int
topomask_append_object(topo_topology_t topology, struct topo_topology_info *topoinfo,
		       topo_cpuset_t *rootset, const char *string,
		       topo_cpuset_t *set, topomask_append_mode_t mode, int verbose)
{
  topo_obj_t obj;
  unsigned depth, width;
  char *sep, *sep2, *sep3;
  unsigned first, wrap, amount;
  unsigned i,j;

  if (!strncasecmp(string, "system", 2))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_SYSTEM);
  else if (!strncasecmp(string, "machine", 1))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_MACHINE);
  else if (!strncasecmp(string, "node", 1))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_NODE);
  else if (!strncasecmp(string, "socket", 2))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_SOCKET);
  else if (!strncasecmp(string, "core", 1))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_CORE);
  else if (!strncasecmp(string, "proc", 1))
    depth = topo_get_type_or_above_depth(topology, TOPO_OBJ_PROC);
  else
    depth = atoi(string);

  if (depth >= topoinfo->depth) {
    if (verbose)
      fprintf(stderr, "ignoring invalid depth %u\n", depth);
    return -1;
  }
  width = topo_get_depth_nbobjs(topology, depth);

  sep = strchr(string, ':');
  if (!sep) {
    if (verbose)
      fprintf(stderr, "missing colon separator in argument %s\n", string);
    return -1;
  }

  first = atoi(sep+1);
  amount = 1;
  wrap = 0;

  sep3 = strchr(sep+1, '.');

  sep2 = strchr(sep+1, '-');
  if (sep2 && (sep2 < sep3 || !sep3)) {
    if (*(sep2+1) == '\0')
      amount = width-first;
    else
      amount = atoi(sep2+1)-first+1;
  } else {
    sep2 = strchr(sep+1, ':');
    if (sep2 && (sep2 < sep3 || !sep3)) {
      amount = atoi(sep2+1);
      wrap = 1;
    }
  }

  for(i=first, j=0; j<amount; i++, j++) {
    if (wrap && i==width)
      i = 0;

    obj = topo_get_obj_below_cpuset_by_depth(topology, rootset, depth, i);
    if (verbose) {
      if (obj)
	printf("object #%u depth %u below cpuset %" TOPO_PRIxCPUSET " found\n",
	       i, depth, TOPO_CPUSET_PRINTF_VALUE(rootset));
      else
	printf("object #%u depth %u below cpuset %" TOPO_PRIxCPUSET " does not exist\n",
	       i, depth, TOPO_CPUSET_PRINTF_VALUE(rootset));
    }
    if (obj) {
      if (sep3)
	topomask_append_object(topology, topoinfo, &obj->cpuset, sep3+1, set, mode, verbose);
      else
        topomask_append_cpuset(set, &obj->cpuset, mode, verbose);
    }
  }

  return 0;
}

static __inline__ void
topomask_process_arg(topo_topology_t topology, struct topo_topology_info *topoinfo,
		     const char *arg, topo_cpuset_t *set,
		     int verbose)
{
  char *colon;
  topomask_append_mode_t mode = TOPOMASK_APPEND_ADD;

  if (*arg == '~') {
    mode = TOPOMASK_APPEND_CLR;
    arg++;
  } else if (*arg == 'x') {
    mode = TOPOMASK_APPEND_AND;
    arg++;
  } else if (*arg == '^') {
    mode = TOPOMASK_APPEND_XOR;
    arg++;
  }

  colon = strchr(arg, ':');
  if (colon) {
    topomask_append_object(topology, topoinfo, &topo_get_system_obj(topology)->cpuset, arg, set, mode, verbose);
  } else {
    topo_cpuset_t newset;
    topo_cpuset_zero(&newset);
    topo_cpuset_from_string(arg, &newset);
    topomask_append_cpuset(set, &newset, mode, verbose);
  }
}

#endif /* TOPOMASK_H */
