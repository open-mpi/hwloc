/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2013 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux 1
 * Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/**
 * This file contains the inline code of functions declared in hwloc.h
 */

#ifndef HWLOC_INLINES_H
#define HWLOC_INLINES_H

#ifndef HWLOC_H
#error Please include the main hwloc.h instead
#endif

#include <stdlib.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif

static __hwloc_inline int
hwloc_get_nbobjs_by_type (hwloc_topology_t topology, hwloc_obj_type_t type)
{
  int depth = hwloc_get_type_depth(topology, type);
  if (depth == HWLOC_TYPE_DEPTH_UNKNOWN)
    return 0;
  if (depth == HWLOC_TYPE_DEPTH_MULTIPLE)
    return -1; /* FIXME: agregate nbobjs from different levels? */
  return hwloc_get_nbobjs_by_depth(topology, depth);
}

static __hwloc_inline hwloc_obj_t
hwloc_get_obj_by_type (hwloc_topology_t topology, hwloc_obj_type_t type, unsigned idx)
{
  int depth = hwloc_get_type_depth(topology, type);
  if (depth == HWLOC_TYPE_DEPTH_UNKNOWN)
    return NULL;
  if (depth == HWLOC_TYPE_DEPTH_MULTIPLE)
    return NULL;
  return hwloc_get_obj_by_depth(topology, depth, idx);
}

static __hwloc_inline const char *
hwloc_obj_get_info_by_name(hwloc_obj_t obj, const char *name)
{
  unsigned i;
  for(i=0; i<obj->infos_count; i++)
    if (!strcmp(obj->infos[i].name, name))
      return obj->infos[i].value;
  return NULL;
}


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_INLINES_H */
