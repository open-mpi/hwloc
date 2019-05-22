/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#ifndef CPUAFFINITY_BIND_H
#define CPUAFFINITY_BIND_H

#include <sys/types.h>
#include "affinity.h"

/**
 * Bind a thread on next topology object.
 * @param objs: The list of hwloc objects to use.
 * @param tid: The system id of the thread to bind.
 * @return The object on which thread is bound.
 **/
hwloc_obj_t cpuaffinity_bind_thread(struct cpuaffinity_enum * objs, const pid_t tid);

/**
 * Enforce a thread binding by checking if it matches a topology object cpuset.
 * @param topology: The topology object to walk when looking for thread location.
 * @param target: The topology object under which the thread is supposed to be bound.
 * @param tid: The system thread id of the thread to check.
 * @return 0 if target and thread location does not match, else 1.
 **/
int cpuaffinity_check(const hwloc_topology_t topology, const hwloc_obj_t target, const pid_t tid);

/**
 * Bind the next threads spawned by a process with a particular cpuaffinity.
 * @param objs: The cpu affinity containing a list of object where to 
 * consecutively bind threads.
 **/
int cpuaffinity_attach(struct cpuaffinity_enum * objs, const pid_t pid);

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

#endif //CPUAFFINITY_BIND_H
