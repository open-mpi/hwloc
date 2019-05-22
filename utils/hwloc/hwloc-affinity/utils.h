/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#ifndef HWLOC_AFFINITY_UTILS
#define HWLOC_AFFINITY_UTILS

#include <hwloc.h>

/**
 * Parse a string hwloc_obj:logical_index and return the matching topology object.
 * @param topology: The topolology in which to search for obj.
 * @param str: The object as string. The allowed string format is object:logical_index
 * where object is parsable by hwloc_type_string function and logical index is the logical
 * index of such object type in topology.
 * @return Matching object on success.
 * @return NULL on failure with errno set to the error reason.
 **/
hwloc_obj_t hwloc_obj_from_string(hwloc_topology_t topology, const char* str);

/**
 * Load a topology configured for the library. Topology require to use merge filter.
 * @param file: A string where to find topology file to load. If NULL the current system
 *        topology will be loaded.
 * @return A topology object on success.
 * @return NULL on failure with errno set to the error.
 **/
hwloc_topology_t hwloc_affinity_topology_load(const char * file);

/**
 * Restrict topology to an object, its parents and children.
 * @param topology: The topology to restrict.
 * @param obj: The object to which the topology has to be restricted.
 * @return 0 on success.
 * @return -1 on failure with errno set to the error reason.
 **/
int hwloc_topology_restrict_obj(hwloc_topology_t topology, hwloc_obj_t obj);

#endif
