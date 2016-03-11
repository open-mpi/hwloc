/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2016 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux
 * Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/**
 * This file contains the inline code of functions declared in hwloc.h
 */

#ifndef HWLOC_DEPRECATED_H
#define HWLOC_DEPRECATED_H

#ifndef HWLOC_H
#error Please include the main hwloc.h instead
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* backward compat with v1.10 before Socket->Package renaming */
#define HWLOC_OBJ_SOCKET HWLOC_OBJ_PACKAGE
/* backward compat with v1.10 before Node->NUMANode clarification */
#define HWLOC_OBJ_NODE HWLOC_OBJ_NUMANODE

/** \brief Insert a misc object by parent.
 *
 * Identical to hwloc_topology_insert_misc_object().
 */
static __hwloc_inline hwloc_obj_t
hwloc_topology_insert_misc_object_by_parent(hwloc_topology_t topology, hwloc_obj_t parent, const char *name) __hwloc_attribute_deprecated;
static __hwloc_inline hwloc_obj_t
hwloc_topology_insert_misc_object_by_parent(hwloc_topology_t topology, hwloc_obj_t parent, const char *name)
{
  return hwloc_topology_insert_misc_object(topology, parent, name);
}

/** \brief Stringify the cpuset containing a set of objects.
 *
 * If \p size is 0, \p string may safely be \c NULL.
 *
 * \return the number of character that were actually written if not truncating,
 * or that would have been written (not including the ending \\0).
 */
static __hwloc_inline int
hwloc_obj_cpuset_snprintf(char *str, size_t size, size_t nobj, struct hwloc_obj * const *objs) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_obj_cpuset_snprintf(char *str, size_t size, size_t nobj, struct hwloc_obj * const *objs)
{
  hwloc_bitmap_t set = hwloc_bitmap_alloc();
  int res;
  unsigned i;

  hwloc_bitmap_zero(set);
  for(i=0; i<nobj; i++)
    if (objs[i]->cpuset)
      hwloc_bitmap_or(set, set, objs[i]->cpuset);

  res = hwloc_bitmap_snprintf(str, size, set);
  hwloc_bitmap_free(set);
  return res;
}

/** \brief Convert a type string into a type and some attributes.
 *
 * Deprecated by hwloc_type_sscanf()
 */
static __hwloc_inline int
hwloc_obj_type_sscanf(const char *string, hwloc_obj_type_t *typep, int *depthattrp, void *typeattrp, size_t typeattrsize) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_obj_type_sscanf(const char *string, hwloc_obj_type_t *typep, int *depthattrp, void *typeattrp, size_t typeattrsize)
{
  union hwloc_obj_attr_u attr;
  int err = hwloc_type_sscanf(string, typep, &attr, sizeof(attr));
  if (err < 0)
    return err;
  if (hwloc_obj_type_is_cache(*typep)) {
    if (depthattrp)
      *depthattrp = attr.cache.depth;
    if (typeattrp && typeattrsize >= sizeof(hwloc_obj_cache_type_t))
      memcpy(typeattrp, &attr.cache.type, sizeof(hwloc_obj_cache_type_t));
  } else if (*typep == HWLOC_OBJ_GROUP) {
    if (depthattrp)
      *depthattrp = attr.group.depth;
  }
  return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_INLINES_H */
