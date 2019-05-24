/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
****************************************************************************/

#ifndef HWLOC_TBIND_H
#define HWLOC_TBIND_H

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
 * @param logical: A boolean. If 0 then os index of hwloc_obj 
 *        are printed, else the logical index is printed.
 * @param num: The number of index to print. 
 *        If 0, all objects index are printed.
 **/
void cpuaffinity_enum_print(const struct cpuaffinity_enum *e,
			    const char *sep,
			    const int logical,
			    const int cpuset,
			    const int taskset,
			    const int reverse,
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
hwloc_obj_t cpuaffinity_enum_get(struct cpuaffinity_enum * e,
				 const size_t i);

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
 * @params n_depths: The number of level in topology to use when building 
 *         the tleaf.
 * @params depths: The topology levels to use in the tleaf. 
 *         The order of this levels encodes the iteration policy.
 * @param  shuffle: If not 0, will shuffle iteration index on each level of 
 *         the tleaf so 
 *         that the overall policy is fulfilled but leaves order is 
 *         randomized. 
 * @return an enumeration of the deepest objects in levels.
 **/
struct cpuaffinity_enum * cpuaffinity_tleaf(hwloc_topology_t topology,
					    const size_t n_depths,
					    const int *depths,
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
struct cpuaffinity_enum *
cpuaffinity_round_robin(hwloc_topology_t topology,
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

/**
 * Bind a thread on next topology object.
 * @param objs: The list of hwloc objects to use.
 * @param tid: The system id of the thread to bind.
 * @return The object on which thread is bound.
 **/
hwloc_obj_t cpuaffinity_bind_thread(struct cpuaffinity_enum * objs,
				    const pid_t tid);

/**
 * Enforce a thread binding by checking if it matches a topology object 
 * cpuset.
 * @param topology: The topology object to walk when looking for thread 
 *        location.
 * @param target: The topology object under which the thread is supposed to 
 *        be bound.
 * @param tid: The system thread id of the thread to check.
 * @return 0 if target and thread location does not match, else 1.
 **/
int cpuaffinity_check(const hwloc_topology_t topology,
		      const hwloc_obj_t target,
		      const pid_t tid);

/**
 * Bind the next threads spawned by a process with a particular cpuaffinity.
 * @param objs: The cpu affinity containing a list of object where to 
 * consecutively bind threads.
 * @param pid: The pid of the process to bind.
 * @param stopped: A boolean telling the process has been sent a SIGSTOP 
 *        signal prior to call to cpuaffinity_attach().
 * @return -1 on failure, 0 on success.
 **/
int cpuaffinity_attach(struct cpuaffinity_enum * objs,
		       const pid_t pid,
		       const int stopped);

/**
 * Fork a new process and bind the subsequent spawned threads with
 * a specific cpu affinity.
 * @param objs: The cpu affinity defining the process threads binding
 *        policy.
 * @param argv: The argv argument provided by main function to provide
 *        to execvp.
 * @return The pid of child process on success, -1 on error to execvp call.
 **/
pid_t cpuaffinity_exec(struct cpuaffinity_enum * objs, char** argv);

/**
 * Parse a string hwloc_obj:logical_index and return the matching 
 * topology object.
 * @param topology: The topolology in which to search for obj.
 * @param str: The object as string. The allowed string format is 
 *        object:logical_index where object is parsable by 
 *        hwloc_type_string function and logical index is the logical
 *        index of such object type in topology.
 * @return Matching object on success.
 * @return NULL on failure with errno set to the error reason.
 **/
hwloc_obj_t hwloc_obj_from_string(hwloc_topology_t topology,
				  const char* str);

/**
 * Load a topology configured for the library. Topology require to use 
 * merge filter.
 * @param file: A string where to find topology file to load. 
 *        If NULL the current system topology will be loaded.
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
 * @param n_depths: The number of levels composing the policy.
 * @param depths: The topology levels. Order matters, it defines the policy.
 * @return NULL on failure, an iterator on success;
 **/
struct hwloc_tleaf_iterator *
hwloc_tleaf_iterator_alloc(hwloc_topology_t topology,
			   const int n_depths,
			   const int * depths);

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
hwloc_obj_t
hwloc_tleaf_iterator_current(const struct hwloc_tleaf_iterator * t);

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
struct cpuaffinity_enum *
hwloc_tleaf_iterator_enumeration(struct hwloc_tleaf_iterator *it);

#endif
