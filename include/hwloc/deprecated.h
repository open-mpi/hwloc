/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2023 Inria.  All rights reserved.
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

/* backward compat with v2.0 before WHOLE_SYSTEM renaming */
#define HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED

/** \brief Get the depth from an object type.
 *
 * Superseded by hwloc_type_sscanf()+hwloc_get_type_depth_with_attr() in v3.0.
 */
HWLOC_DECLSPEC int hwloc_type_sscanf_as_depth(const char *string, hwloc_obj_type_t *typep,
                                              hwloc_topology_t topology, int *depthp) __hwloc_attribute_deprecated;

/** \brief Add a distances structure.
 *
 * Superseded by hwloc_distances_add_create()+hwloc_distances_add_values()+hwloc_distances_add_commit()
 * in v2.5.
 */
HWLOC_DECLSPEC int hwloc_distances_add(hwloc_topology_t topology,
				       unsigned nbobjs, hwloc_obj_t *objs, hwloc_uint64_t *values,
				       unsigned long kind, unsigned long flags) __hwloc_attribute_deprecated;

/** \brief Set the default memory binding policy of the current
 * process or thread to prefer the NUMA node(s) specified by physical \p nodeset
 */
static __hwloc_inline int
hwloc_set_membind_nodeset(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_set_membind_nodeset(hwloc_topology_t topology, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  return hwloc_set_membind(topology, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Query the default memory binding policy and physical locality of the
 * current process or thread.
 */
static __hwloc_inline int
hwloc_get_membind_nodeset(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_get_membind_nodeset(hwloc_topology_t topology, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  return hwloc_get_membind(topology, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Set the default memory binding policy of the specified
 * process to prefer the NUMA node(s) specified by physical \p nodeset
 */
static __hwloc_inline int
hwloc_set_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_set_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  return hwloc_set_proc_membind(topology, pid, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Query the default memory binding policy and physical locality of the
 * specified process.
 */
static __hwloc_inline int
hwloc_get_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_get_proc_membind_nodeset(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  return hwloc_get_proc_membind(topology, pid, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Bind the already-allocated memory identified by (addr, len)
 * to the NUMA node(s) in physical \p nodeset.
 */
static __hwloc_inline int
hwloc_set_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_set_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  return hwloc_set_area_membind(topology, addr, len, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Query the physical NUMA node(s) and binding policy of the memory
 * identified by (\p addr, \p len ).
 */
static __hwloc_inline int
hwloc_get_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags) __hwloc_attribute_deprecated;
static __hwloc_inline int
hwloc_get_area_membind_nodeset(hwloc_topology_t topology, const void *addr, size_t len, hwloc_nodeset_t nodeset, hwloc_membind_policy_t * policy, int flags)
{
  return hwloc_get_area_membind(topology, addr, len, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Allocate some memory on the given physical nodeset \p nodeset
 */
static __hwloc_inline void *
hwloc_alloc_membind_nodeset(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags) __hwloc_attribute_malloc __hwloc_attribute_deprecated;
static __hwloc_inline void *
hwloc_alloc_membind_nodeset(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  return hwloc_alloc_membind(topology, len, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}

/** \brief Allocate some memory on the given nodeset \p nodeset.
 */
static __hwloc_inline void *
hwloc_alloc_membind_policy_nodeset(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags) __hwloc_attribute_malloc __hwloc_attribute_deprecated;
static __hwloc_inline void *
hwloc_alloc_membind_policy_nodeset(hwloc_topology_t topology, size_t len, hwloc_const_nodeset_t nodeset, hwloc_membind_policy_t policy, int flags)
{
  return hwloc_alloc_membind_policy(topology, len, nodeset, policy, flags | HWLOC_MEMBIND_BYNODESET);
}


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_DEPRECATED_H */
