/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2011 INRIA.  All rights reserved.
 * Copyright © 2009-2011 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_MASK_H
#define HWLOC_MASK_H

#include <private/private.h>
#include <private/misc.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>

typedef enum hwloc_mask_append_mode_e {
  HWLOC_MASK_APPEND_ADD,
  HWLOC_MASK_APPEND_CLR,
  HWLOC_MASK_APPEND_AND,
  HWLOC_MASK_APPEND_XOR
} hwloc_mask_append_mode_t;

static __hwloc_inline int
hwloc_mask_append_cpuset(hwloc_bitmap_t set, hwloc_const_bitmap_t newset,
		       hwloc_mask_append_mode_t mode, int verbose)
{
  char *s1, *s2;
  hwloc_bitmap_asprintf(&s1, newset);
  hwloc_bitmap_asprintf(&s2, set);
  switch (mode) {
  case HWLOC_MASK_APPEND_ADD:
    if (verbose)
      fprintf(stderr, "adding %s to %s\n",
          s1, s2);
    hwloc_bitmap_or(set, set, newset);
    break;
  case HWLOC_MASK_APPEND_CLR:
    if (verbose)
      fprintf(stderr, "clearing %s from %s\n",
          s1, s2);
    hwloc_bitmap_andnot(set, set, newset);
    break;
  case HWLOC_MASK_APPEND_AND:
    if (verbose)
      fprintf(stderr, "and'ing %s from %s\n",
          s1, s2);
    hwloc_bitmap_and(set, set, newset);
    break;
  case HWLOC_MASK_APPEND_XOR:
    if (verbose)
      fprintf(stderr, "xor'ing %s from %s\n",
          s1, s2);
    hwloc_bitmap_xor(set, set, newset);
    break;
  default:
    assert(0);
  }
  free(s1);
  free(s2);
  return 0;
}

static __hwloc_inline hwloc_obj_t __hwloc_attribute_pure
hwloc_mask_get_obj_inside_cpuset_by_depth(hwloc_topology_t topology, hwloc_const_bitmap_t rootset,
					 unsigned depth, unsigned i, int logical)
{
  if (logical) {
    return hwloc_get_obj_inside_cpuset_by_depth(topology, rootset, depth, i);
  } else {
    hwloc_obj_t obj = NULL;
    while ((obj = hwloc_get_next_obj_inside_cpuset_by_depth(topology, rootset, depth, obj)) != NULL) {
      if (obj->os_index == i)
        return obj;
    }
    return NULL;
  }
}

/* extended version of hwloc_obj_type_of_string()
 *
 * matches L2, L3Cache and Group4, and return the corresponding depth attribute if depthattrp isn't NULL.
 * only looks at the beginning of the string to allow truncated type names.
 */
static __hwloc_inline int
hwloc_obj_type_sscanf(const char *string, hwloc_obj_type_t *typep, unsigned *depthattrp)
{
  hwloc_obj_type_t type = (hwloc_obj_type_t) -1;
  unsigned depthattr = (unsigned) -1;

  /* types without depthattr */
  if (!strncasecmp(string, "system", 2)) {
    type = HWLOC_OBJ_SYSTEM;
  } else if (!hwloc_strncasecmp(string, "machine", 2)) {
    type = HWLOC_OBJ_MACHINE;
  } else if (!hwloc_strncasecmp(string, "node", 1)) {
    type = HWLOC_OBJ_NODE;
  } else if (!hwloc_strncasecmp(string, "socket", 2)) {
    type = HWLOC_OBJ_SOCKET;
  } else if (!hwloc_strncasecmp(string, "core", 2)) {
    type = HWLOC_OBJ_CORE;
  } else if (!hwloc_strncasecmp(string, "pu", 1) || !hwloc_strncasecmp(string, "proc", 1) /* backward compat with 0.9 */) {
    type = HWLOC_OBJ_PU;
  } else if (!hwloc_strncasecmp(string, "misc", 2)) {
    type = HWLOC_OBJ_MISC;

  /* types with depthattr */
  } else if (!hwloc_strncasecmp(string, "cache", 2)) {
    type = HWLOC_OBJ_CACHE;
  } else if ((string[0] == 'l' || string[0] == 'L') && string[1] >= '0' && string[1] <= '9') {
    type = HWLOC_OBJ_CACHE;
    depthattr = atoi(string+1);
  } else if (!hwloc_strncasecmp(string, "group", 1)) {
    int length;
    type = HWLOC_OBJ_GROUP;
    length = strcspn(string, "0123456789");
    if (string[length] != '\0')
      depthattr = atoi(string+length);
  } else
    return -1;

  *typep = type;
  if (depthattrp)
    *depthattrp = depthattr;

  return 0;
}

static __hwloc_inline int
hwloc_mask_append_object(hwloc_topology_t topology, unsigned topodepth,
		       hwloc_const_bitmap_t rootset, const char *string, int logical,
		       hwloc_bitmap_t set, int verbose)
{
  hwloc_obj_t obj;
  hwloc_obj_type_t type;
  unsigned depth, width, depthattr;
  char *sep, *sep2, *sep3;
  char typestring[20+1]; /* large enough to store all type names, even with a depth attribute */
  unsigned first, wrap, amount, step;
  unsigned i,j;
  int err;

  sep = strchr(string, ':');
  if (!sep) {
    fprintf(stderr, "missing colon separator in argument %s\n", string);
    return -1;
  }
  if ((unsigned) (sep-string) >= sizeof(typestring)) {
    fprintf(stderr, "invalid type name %s\n", string);
    return -1;
  }
  strncpy(typestring, string, sep-string);
  typestring[sep-string] = '\0';

  /* try to match a type name */
  err = hwloc_obj_type_sscanf(typestring, &type, &depthattr);
  if (!err) {
    if (depthattr == (unsigned) -1) {
      hwloc_obj_type_t realtype;
      /* matched a type without depth attribute, try to get the depth from the type if it exists and is unique */
      depth = hwloc_get_type_or_above_depth(topology, type);
      if (depth == (unsigned) HWLOC_TYPE_DEPTH_MULTIPLE) {
	fprintf(stderr, "type %s has multiple possible depths\n", hwloc_obj_type_string(type));
	return -1;
      } else if (depth == (unsigned) HWLOC_TYPE_DEPTH_UNKNOWN) {
	fprintf(stderr, "type %s isn't available\n", hwloc_obj_type_string(type));
	return -1;
      }
      realtype = hwloc_get_depth_type(topology, depth);
      if (type != realtype && verbose)
	fprintf(stderr, "using type %s (depth %d) instead of %s\n",
		hwloc_obj_type_string(realtype), depth, hwloc_obj_type_string(type));
    } else {
      /* matched a type with a depth attribute, look at the first object of each level to find the depth */
      assert(type == HWLOC_OBJ_CACHE || type == HWLOC_OBJ_GROUP);
      for(i=0; ; i++) {
	hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, i, 0);
	if (!obj) {
	  fprintf(stderr, "type %s with custom depth %u does not exists\n", hwloc_obj_type_string(type), depthattr);
	  return -1;
	}
	if (obj->type == type
	    && ((type == HWLOC_OBJ_CACHE && depthattr == obj->attr->cache.depth)
		|| (type == HWLOC_OBJ_GROUP && depthattr == obj->attr->group.depth))) {
	  depth = i;
	  break;
	}
      }
    }
  } else {
    /* try to match a numeric depth */
    char *end;
    depth = strtol(string, &end, 0);
    if (end == string) {
      fprintf(stderr, "invalid type name %s\n", string);
      return -1;
    }
  }

  if (depth >= topodepth) {
    fprintf(stderr, "ignoring invalid depth %u\n", depth);
    return -1;
  }
  width = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, rootset, depth);

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

    obj = hwloc_mask_get_obj_inside_cpuset_by_depth(topology, rootset, depth, i, logical);
    if (verbose || !obj) {
      char *s;
      hwloc_bitmap_asprintf(&s, rootset);
      if (obj)
	printf("using object #%u depth %u below cpuset %s\n",
	       i, depth, s);
      else
	fprintf(stderr, "object #%u depth %u (type %s) below cpuset %s does not exist\n",
		i, depth, typestring, s);
      free(s);
    }
    if (obj) {
      if (sep3)
	hwloc_mask_append_object(topology, topodepth, obj->cpuset, sep3+1, logical, set, verbose);
      else
	/* add to the temporary cpuset
	 * and let the caller add/clear/and/xor for the actual final cpuset depending on cmdline options
	 */
        hwloc_mask_append_cpuset(set, obj->cpuset, HWLOC_MASK_APPEND_ADD, verbose);
    }
  }

  return 0;
}

static __hwloc_inline int
hwloc_mask_process_arg(hwloc_topology_t topology, unsigned topodepth,
		     const char *arg, int logical, hwloc_bitmap_t set,
		     int taskset, int verbose)
{
  char *colon;
  hwloc_mask_append_mode_t mode = HWLOC_MASK_APPEND_ADD;
  int err;

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

  if (!strcmp(arg, "all") || !strcmp(arg, "root"))
    return hwloc_mask_append_cpuset(set, hwloc_topology_get_topology_cpuset(topology), mode, verbose);

  colon = strchr(arg, ':');
  if (colon) {
    hwloc_bitmap_t newset = hwloc_bitmap_alloc();
    err = hwloc_mask_append_object(topology, topodepth, hwloc_topology_get_complete_cpuset(topology), arg, logical, newset, verbose);
    if (!err)
      err = hwloc_mask_append_cpuset(set, newset, mode, verbose);
    hwloc_bitmap_free(newset);

  } else if (taskset) {
    /* try to parse as a list of integer starting with 0xf...f or 0x */
    char *tmp = (char*) arg;
    hwloc_bitmap_t newset;
    if (strncasecmp(tmp, "0xf...f", 7) == 0) {
      tmp += 7;
      if (0 == *tmp) {
        err = -1;
        goto out;
      }
    } else {
      if (strncasecmp(tmp, "0x", 2) != 0) {
        err = -1;
        goto out;
      }
      tmp += 2;
      if (0 == *tmp) {
        err = -1;
        goto out;
      }
    }
    if (strlen(tmp) != strspn(tmp, "0123456789abcdefABCDEF")) {
      err = -1;
      goto out;
    }
    newset = hwloc_bitmap_alloc();
    hwloc_bitmap_taskset_sscanf(newset, arg);
    err = hwloc_mask_append_cpuset(set, newset, mode, verbose);
    hwloc_bitmap_free(newset);

  } else {
    /* try to parse as a comma-separated list of integer with 0x as an optional prefix, and possibly starting with 0xf...f */
    char *tmp = (char*) arg;
    hwloc_bitmap_t newset;
    if (strncasecmp(tmp, "0xf...f,", 8) == 0) {
      tmp += 8;
      if (0 == *tmp) {
        err = -1;
        goto out;
      }
    }
    while (1) {
      char *next = strchr(tmp, ',');
      size_t len;
      if (strncasecmp(tmp, "0x", 2) == 0) {
        tmp += 2;
        if (',' == *tmp || 0 == *tmp) {
          err = -1;
          goto out;
        }
      }
      len = next ? (size_t) (next-tmp) : strlen(tmp);
      if (len != strspn(tmp, "0123456789abcdefABCDEF")) {
        err = -1;
        goto out;
      }
      if (!next)
        break;
      tmp = next+1;
    }
    newset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(newset, arg);
    err = hwloc_mask_append_cpuset(set, newset, mode, verbose);
    hwloc_bitmap_free(newset);
  }

 out:
  return err;
}

#endif /* HWLOC_MASK_H */
