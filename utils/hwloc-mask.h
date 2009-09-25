/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_MASK_H
#define HWLOC_MASK_H

#include <hwloc.h>
#include <private/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef enum hwloc_mask_append_mode_e {
  HWLOC_MASK_APPEND_ADD,
  HWLOC_MASK_APPEND_CLR,
  HWLOC_MASK_APPEND_AND,
  HWLOC_MASK_APPEND_XOR,
} hwloc_mask_append_mode_t;

static __inline__ void
hwloc_mask_append_cpuset(hwloc_cpuset_t *set, hwloc_cpuset_t *newset,
		       hwloc_mask_append_mode_t mode, int verbose)
{
  switch (mode) {
  case HWLOC_MASK_APPEND_ADD:
    if (verbose)
      fprintf(stderr, "adding %" HWLOC_PRIxCPUSET " to %" HWLOC_PRIxCPUSET "\n",
	      HWLOC_CPUSET_PRINTF_VALUE(newset), HWLOC_CPUSET_PRINTF_VALUE(set));
    hwloc_cpuset_orset(set, newset);
    break;
  case HWLOC_MASK_APPEND_CLR:
    if (verbose)
      fprintf(stderr, "clearing %" HWLOC_PRIxCPUSET " from %" HWLOC_PRIxCPUSET "\n",
	      HWLOC_CPUSET_PRINTF_VALUE(newset), HWLOC_CPUSET_PRINTF_VALUE(set));
    hwloc_cpuset_clearset(set, newset);
    break;
  case HWLOC_MASK_APPEND_AND:
    if (verbose)
      fprintf(stderr, "and'ing %" HWLOC_PRIxCPUSET " from %" HWLOC_PRIxCPUSET "\n",
	      HWLOC_CPUSET_PRINTF_VALUE(newset), HWLOC_CPUSET_PRINTF_VALUE(set));
    hwloc_cpuset_andset(set, newset);
    break;
  case HWLOC_MASK_APPEND_XOR:
    if (verbose)
      fprintf(stderr, "xor'ing %" HWLOC_PRIxCPUSET " from %" HWLOC_PRIxCPUSET "\n",
	      HWLOC_CPUSET_PRINTF_VALUE(newset), HWLOC_CPUSET_PRINTF_VALUE(set));
    hwloc_cpuset_xorset(set, newset);
    break;
  default:
    assert(1);
  }
}

static __inline__ int
hwloc_mask_append_object(hwloc_topology_t topology, unsigned topodepth,
		       hwloc_cpuset_t *rootset, const char *string,
		       hwloc_cpuset_t *set, hwloc_mask_append_mode_t mode, int verbose)
{
  hwloc_obj_t obj;
  unsigned depth, width;
  char *sep, *sep2, *sep3;
  unsigned first, wrap, amount, step;
  unsigned i,j;

  if (!strncasecmp(string, "system", 2))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_SYSTEM);
  else if (!strncasecmp(string, "machine", 1))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_MACHINE);
  else if (!strncasecmp(string, "node", 1))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_NODE);
  else if (!strncasecmp(string, "socket", 2))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_SOCKET);
  else if (!strncasecmp(string, "core", 1))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_CORE);
  else if (!strncasecmp(string, "proc", 1))
    depth = hwloc_get_type_or_above_depth(topology, HWLOC_OBJ_PROC);
  else
    depth = atoi(string);

  if (depth >= topodepth) {
    if (verbose)
      fprintf(stderr, "ignoring invalid depth %u\n", depth);
    return -1;
  }
  width = hwloc_get_nbobjs_by_depth(topology, depth);

  sep = strchr(string, ':');
  if (!sep) {
    if (verbose)
      fprintf(stderr, "missing colon separator in argument %s\n", string);
    return -1;
  }

  first = atoi(sep+1);
  amount = 1;
  step = 1;
  wrap = 0;
  if (!isdigit(*(sep+1))) {
    if (!strncmp(sep+1, "all", 3)) {
      first = 0;
      amount = width;
    } else if (!strncmp(sep+1, "odd", 3)) {
      first = 1;
      step = 2;
      amount = (width+1)/2;
    } else if (!strncmp(sep+1, "even", 4)) {
      first = 0;
      step = 2;
      amount = (width+1)/2;
    }
  }

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

  for(i=first, j=0; j<amount; i+=step, j++) {
    if (wrap && i==width)
      i = 0;

    obj = hwloc_get_obj_inside_cpuset_by_depth(topology, rootset, depth, i);
    if (verbose) {
      if (obj)
	printf("object #%u depth %u below cpuset %" HWLOC_PRIxCPUSET " found\n",
	       i, depth, HWLOC_CPUSET_PRINTF_VALUE(rootset));
      else
	printf("object #%u depth %u below cpuset %" HWLOC_PRIxCPUSET " does not exist\n",
	       i, depth, HWLOC_CPUSET_PRINTF_VALUE(rootset));
    }
    if (obj) {
      if (sep3)
	hwloc_mask_append_object(topology, topodepth, &obj->cpuset, sep3+1, set, mode, verbose);
      else
        hwloc_mask_append_cpuset(set, &obj->cpuset, mode, verbose);
    }
  }

  return 0;
}

static __inline__ void
hwloc_mask_process_arg(hwloc_topology_t topology, unsigned topodepth,
		     const char *arg, hwloc_cpuset_t *set,
		     int verbose)
{
  char *colon;
  hwloc_mask_append_mode_t mode = HWLOC_MASK_APPEND_ADD;

  if (*arg == '~') {
    mode = HWLOC_MASK_APPEND_CLR;
    arg++;
  } else if (*arg == 'x') {
    mode = HWLOC_MASK_APPEND_AND;
    arg++;
  } else if (*arg == '^') {
    mode = HWLOC_MASK_APPEND_XOR;
    arg++;
  }

  colon = strchr(arg, ':');
  if (colon) {
    hwloc_mask_append_object(topology, topodepth, &hwloc_get_system_obj(topology)->cpuset, arg, set, mode, verbose);
  } else {
    hwloc_cpuset_t newset;
    hwloc_cpuset_zero(&newset);
    hwloc_cpuset_from_string(arg, &newset);
    hwloc_mask_append_cpuset(set, &newset, mode, verbose);
  }
}

#endif /* HWLOC_MASK_H */
