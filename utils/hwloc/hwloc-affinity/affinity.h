/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#ifndef CPUAFFINITY_H
#define CPUAFFINITY_H

#include "hwloc.h"

/**
 * Structure holding an ordered enumeration of
 * all objects (hwloc_obj) of the same type.
 * This structure is further used for binding threads
 * in the order of their creation. Enumerations can be
 * initialized by manually appending topology objects
 * to it or with helper functions.
 **/
struct cpuaffinity_enum;

/**
 * Allocate an enumeration of topology objects.
 * @param topology: The topology where objects originate.
 *        enumeration will fit at most the number of leaves.
 **/
struct cpuaffinity_enum *cpuaffinity_enum_alloc(hwloc_topology_t topology);

/**
 * Free enumeration of processing units.
 * @param obj: The object to free.
 **/
void cpuaffinity_enum_free(struct cpuaffinity_enum * obj);

/**
 * Add a processing unit at the end of enumeration.
 * @param e: The enumeration to which append hwloc_obj.
 * @param obj: The hwloc_obj to append.
 * @return -1 on error with errno set to;
 *          - EINVAL if obj already exists in enumeration, or on argument 
 *            is NULL.
 *          - EDOM if the enumeration is full.
 * @return 0 on success.
 **/
int cpuaffinity_enum_append(struct cpuaffinity_enum * e, hwloc_obj_t obj);

/**
 * Get enumeration length.
 **/
size_t cpuaffinity_enum_size(struct cpuaffinity_enum *obj);

/**
 * Print indexes of hwloc_obj to stdout.
 * @param e: The enumeration to print. Printing can be shorten by setting 
 *        field "n" to a smaller number desired number of elements.
 * @param sep: The character separator to print between hwloc_obj indexes.
 * @param logical: A boolean. If 0 then os index of hwloc_obj are printed, else
 *        the logical index is printed.
 * @param num: The number of index to print. If 0, all objects index are printed.
 **/
void cpuaffinity_enum_print(const struct cpuaffinity_enum * e,
			    const char *sep,
			    const int logical,
			    unsigned num);

/**
 * Get next object in enumeration. If end of enumeration has been reach,
 * then first object is returned.
 * @param e: The object enumeration to iterate.
 * @return The next obj in enumeration.
 **/
hwloc_obj_t cpuaffinity_enum_next(struct cpuaffinity_enum * e);

/**
 * Get next an object in enumeration. If index is out of bound, a modulo 
 * on index is done.
 * @param e: The enumeration of objects.
 * @param i: The index of object.
 * @return An object in enumeration.
 **/
hwloc_obj_t cpuaffinity_enum_get(struct cpuaffinity_enum * e, const size_t i);

/*******************************************************************/
/*                     tleaf fashion enumeration                   */
/*******************************************************************/

/**
 * This function returns a topology tleaf iterator.
 * A tleaf is a tree where all levels at a same depth have the 
 * same arity. This iterator is iterating on leaves of such a tree.
 * And such a tree is mapped on a machine topology.
 * Iteration is made level wise. For each level we consider the 
 * level node arity. Iteration works by incrementing a counter
 * of the last level until it reaches level arity. When it does,
 * it is reset to zero and next level arity is incremented, and
 * so on, recursively on levels.
 * The iterator implements iteration politics into levels order.
 * Iteration index of levels is updated according to this order.
 * The iteration index in each level encodes the tree
 * coordinates of a leaf and allow to return the appropriate 
 * topology object on tleaf leaves.
 * @param topology: The topology on which to map a tleaf.
 *        If NULL, the current system topology is loaded.
 * @params n_levels: The number of level in topology to use when building the tleaf.
 * @params levels: The topology levels to use in the tleaf. The order of this levels
 *         encodes the iteration policy.
 * @param  shuffle: If not 0, will shuffle iteration index on each level of the tleaf so 
 *         that the overall policy is fulfilled but leaves order is randomized. 
 * @return an enumeration of the deepest objects in levels.
 **/
struct cpuaffinity_enum * cpuaffinity_tleaf(hwloc_topology_t topology,
					    const size_t n_levels,
					    const hwloc_obj_type_t *levels,
					    const int shuffle);

/**
 * Iterate objects of a topology level in a round robin fashion,
 * and return one PU per object.
 * If over subcription happens, i.e more iteration are queried
 * than the amount level objects, then next iteration will 
 * return next HWLOC_OBJ_PUs, and so on in a round-robin fashion
 * for each level object.
 * @param topology: The topology from which to create the enumeration.
 * @param level: The level to iterate in a round-robin fashion.
 * @return a HWLOC_OBJ_PU enumeration.
 **/
struct cpuaffinity_enum * cpuaffinity_round_robin(hwloc_topology_t topology,
						  const hwloc_obj_type_t level);

/**
 * Returned Processing Units (PU) ordered in a scatter fashion, i.e
 * each PU is successively as far as possible to the previous ones.
 * This policy is usefull to grant as much topology resources as possible 
 * whatever the tread number.
 * If over subcription happens, i.e more iteration are queried
 * than the amount level objects, then next iteration will 
 * return next HWLOC_OBJ_PUs, and so on in a round-robin fashion
 * for each level object.
 * @param topology: The topology from which to create the enumeration.
 * @param level: The level to iterate in a scatter fashion.
 * @return a HWLOC_OBJ_PU enumeration.
 **/
struct cpuaffinity_enum * cpuaffinity_scatter(hwloc_topology_t topology,
					      const hwloc_obj_type_t level);

#endif //CPUAFFINITY_H
