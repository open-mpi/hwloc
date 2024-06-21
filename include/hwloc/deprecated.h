/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2024 Inria.  All rights reserved.
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

/* backward compat with v2.x before _MEANS_ became _VALUE_ */
#define HWLOC_DISTANCES_KIND_MEANS_LATENCY HWLOC_DISTANCES_KIND_VALUE_LATENCY
#define HWLOC_DISTANCES_KIND_MEANS_BANDWIDTH HWLOC_DISTANCES_KIND_VALUE_BANDWIDTH

/* backward compat with v2.x before BLOCK renaming */
#define HWLOC_OBJ_OSDEV_BLOCK HWLOC_OBJ_OSDEV_STORAGE
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

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_DEPRECATED_H */
