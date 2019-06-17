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
 * @param num: The number of index to print. 
 *        If 0, all objects index are printed.
 **/
void cpuaffinity_enum_print(const struct cpuaffinity_enum *e,
			    const char *sep,
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
 * Bind a thread on next topology object.
 * @param objs: The list of hwloc objects to use.
 * @param tid: The system id of the thread to bind.
 * @return The object on which thread is bound.
 **/
hwloc_obj_t cpuaffinity_bind_thread(struct cpuaffinity_enum * objs,
				    const pid_t tid);

/**
 * Bind the next threads spawned by a process with a particular cpuaffinity.
 * @param pid: The pid of the process to bind.
 * @param objs: The cpu affinity containing a list of object where to 
 * consecutively bind threads.
 * @param repeat: If more thread are created than objects in 
 *        cpuaffinity_enum, and repeat is set, then next will,
 *        bound in a round robin fashion of objs instead of not
 *        not beeing bound.
 * @param stopped: A boolean telling the process has been sent a SIGSTOP 
 *        signal prior to call to cpuaffinity_attach().
 * @return -1 on failure, pid exit status on success, 0 if pid has terminated
 *         because of a signal.
 **/
int cpuaffinity_attach(const pid_t pid,
		       struct cpuaffinity_enum * objs,
		       const int repeat,
		       const int stopped);

hwloc_cpuset_t hwloc_process_location(hwloc_topology_t topology,
				      const char* str);

int restrict_topology(hwloc_topology_t topology,
		      const char *restrict_str);

#endif
