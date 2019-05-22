/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#ifndef CPUAFFINITY_TLEAF_H
#define CPUAFFINITY_TLEAF_H

/**
 * A tleaf is a tree where all levels at a same depth have the 
 * same arity. This iterator is iterating on leaves of such a tree.
 * Iteration is made level wise. For each level we consider the 
 * level node arity. Iteration works by incrementing a counter
 * of the last level until it reaches level arity. When it does,
 * it is reset to zero and next level arity is incremented, and
 * so on, recursively on levels.
 * The iterator implements iteration politics into levels order.
 * Iteration counter is updated according to such an order.
 * The iteration index in each level encodes the tree
 * coordinates of a leaf and allow to return the appropriate one.
 *
 *          0
 *         / \
 *        /   \
 *      0,0   0,1
 *      / \
 *     /   \
 *  0,0,0 0,0,1
 *
 * Therefore, when building the tree, it is possible
 * to implement various policies by ordering levels in a 
 * different manners. For instance, ordering from topology
 * leaves to topology root would walk leaves in a round-robin
 * fashion whereas the opposite order would walk leaves in 
 * a scatter fashion.
 **/
struct hwloc_tleaf_iterator;

/**
 * Allocates a topology iterator.
 * @param topology: The topology to iterate on.
 * @param n_levels: The number of levels composing the policy.
 * @param levels: The topology levels. Order matters, it defines the policy.
 * @return NULL on failure, an iterator on success;
 **/
struct hwloc_tleaf_iterator *hwloc_tleaf_iterator_alloc(hwloc_topology_t topology,
							const int n_levels,
							const hwloc_obj_type_t * levels);

/**
 * Free a topology iterator.
 * @param t: The iterator to free.
 **/
void hwloc_tleaf_iterator_free(struct hwloc_tleaf_iterator * t);

void hwloc_tleaf_iterator_shuffle(struct hwloc_tleaf_iterator * t);

size_t hwloc_tleaf_iterator_size(const struct hwloc_tleaf_iterator * t);

/**
 * Return the current object pointed by the iterator.
 * @param t: The iterator.
 * @return The object currently pointed.
 **/
hwloc_obj_t hwloc_tleaf_iterator_current(const struct hwloc_tleaf_iterator * t);

/**
 * Get next element in iterator.
 * @param t: The iterator
 * @return Next element. If iterator is NULL or has less
 * than 2 levels, arity does not exists and then it cannot
 * iterate and will return NULL.
 **/
hwloc_obj_t hwloc_tleaf_iterator_next(struct hwloc_tleaf_iterator *t);

/**
 * Convert a tree iterator to a flat array of objects.
 * @param it: A tree iterator to retrieve topology objects.
 * 
 **/
struct cpuaffinity_enum * hwloc_tleaf_iterator_enumeration(struct hwloc_tleaf_iterator *it);

#endif
