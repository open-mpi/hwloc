/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/*
 * The original plpa.h file is
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2008 Cisco, Inc.  All rights reserved.
*/

/** \file
 * \brief PLPA API wrappers.
 *
 * These wrappers are provided as a quick and dirty way to switch from
 * PLPA to hwloc. Users are encouraged to switch to the native hwloc API
 * as soon as possible. For this reason, the documentation below will
 * remain minimal.
 *
 * The major differences between the original PLPA API and this
 * reimplementation are:
 * + The hwloc_ prefix was added to all function names.
 * + A hwloc_topology_t parameter was added to all functions.
 *   Since a topology context is always used, PLPA's caching is now
 *   meaningless. So set_cache_behavior() was removed.
 * + PLPA's cpusets are replaced by hwloc_cpuset_t. See hwloc/cpuset.h
 *   for details about manipulating those.
 * + All countspec parameters were removed. Only online CPUs are
 *   manipulated here. Offlines CPUs are now handled separately since
 *   they have no topology information. The list of online processors
 *   may be obtained with hwloc_topology_get_online_cpuset().
 * + The deprecated get_processor_info() function was not reimplemented.
 */

#ifndef HWLOC_PLPA_H
#define HWLOC_PLPA_H

#include <hwloc.h>
#include <private/config.h>

#ifdef HWLOC_LINUX_SYS
#include <hwloc/linux.h>
#endif

/** \defgroup hwlocality_plpa PLPA-like interface
 * @{
 */


/** \brief Initialize a hwloc-plpa context.
 *
 * \note This function is not optional anymore,
 * it is now required since the topology context must be setup.
 */
static __inline int
hwloc_plpa_init(hwloc_topology_t *topology)
{
  int err;
  err = hwloc_topology_init(topology);
  if (err < 0)
    return err;
  err = hwloc_topology_load(*topology);
  return err;
}

/** \brief Destroy a hwloc-plpa context.
 */
static __inline int
hwloc_plpa_finalize(hwloc_topology_t topology)
{
  hwloc_topology_destroy(topology);
  return 0;
}

/** \brief Binding support probe results.
 *
 * \note PROBE_UNSET and PROBE_UNKNOWN are not defined
 * since they do not make sense with hwloc.
 */
typedef enum {
  HWLOC_PLPA_PROBE_OK,			/**< \brief Binding is supported */
  HWLOC_PLPA_PROBE_NOT_SUPPORTED	/**< \brief Binding unavailable/unimplemented */
} hwloc_plpa_api_type_t;

/** \brief Probe binding support.
 */
static __inline int
hwloc_plpa_api_probe(hwloc_topology_t topology, hwloc_plpa_api_type_t *api_type)
{
  unsigned long flags;
  hwloc_topology_get_support(topology, &flags);
  /* FIXME: should be SET_THREAD_CPUBIND given with a pid */
  if (flags & HWLOC_SUPPORT_SET_PROC_CPUBIND)
    *api_type = HWLOC_PLPA_PROBE_OK;
  else
    *api_type = HWLOC_PLPA_PROBE_NOT_SUPPORTED;
  return 0;
}

/** \brief Probe topology discovery support.
 */
static __inline int
hwloc_plpa_have_topology_information(hwloc_topology_t topology, int *supported)
{
  unsigned long flags;
  hwloc_topology_get_support(topology, &flags);
  *supported = ((flags & HWLOC_SUPPORT_DISCOVERY) != 0);
  return 0;
}

/** \brief Bind process given by \p pid to CPU set \p cpuset.
 *
 * \note This function now manipulates hwloc cpusets.
 * \note In the case of Linux (and Linux only), a thread can be designated by
 * its tid instead of a pid.
 */
static __inline int
hwloc_plpa_sched_setaffinity(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_cpuset_t cpuset)
{
#ifdef HWLOC_LINUX_SYS
  return hwloc_linux_set_tid_cpubind(topology, pid, cpuset);
#else /* HWLOC_LINUX_SYS */
  return hwloc_set_proc_cpubind(topology, pid, cpuset, 0);
#endif /* HWLOC_LINUX_SYS */
}

/** \brief Retrieve the binding CPU set of process given by \p pid.
 *
 * \note This function now manipulates hwloc cpusets.
 * \note In the case of Linux (and Linux only), a thread can be designated by
 * its tid instead of a pid.
 */
static __inline int
hwloc_plpa_sched_getaffinity(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t cpuset)
{
  hwloc_cpuset_t tmp;
#ifdef HWLOC_LINUX_SYS
  tmp = hwloc_linux_get_tid_cpubind(topology, pid);
#else /* HWLOC_LINUX_SYS */
  tmp = hwloc_get_proc_cpubind(topology, pid, 0);
#endif /* HWLOC_LINUX_SYS */
  if (!tmp)
    return -1;
  hwloc_cpuset_copy(cpuset, tmp);
  hwloc_cpuset_free(tmp);
  return 0;
}

/** \brief Map a given core os_index withing a socket os_index to a physical process os_index.
 *
 * \note Since the given core may contain multiple logical processors,
 * we return the os_index of the first one.
 */
static __inline int
hwloc_plpa_map_to_processor_id(hwloc_topology_t topology,
			       int socket_id, int core_id, int *processor_id)
{
  hwloc_obj_t socket = NULL, core = NULL, proc;
  /* find the socket whose os_index is socket_id */
  while ((socket = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket)) != NULL
	 && socket->os_index != socket_id);
  if (!socket)
    return -1;
  /* within this socket, find the core whose os_index is core_id */
  while ((core = hwloc_get_next_obj_inside_cpuset_by_type(topology, socket->cpuset, HWLOC_OBJ_CORE, core)) != NULL
	 && core->os_index != core_id);
  if (!core)
    return -1;
  /* within this core, take the first processor */
  proc = hwloc_get_next_obj_inside_cpuset_by_type(topology, core->cpuset, HWLOC_OBJ_PROC, NULL);
  if (!proc)
    return -1;
  /* return the os_index of this processor */
  *processor_id = proc->os_index;
  return 0;
}

/** \brief Given a physical processor os_index, return the os_index of the corresponding socket and core.
 */
static __inline int
hwloc_plpa_map_to_socket_core(hwloc_topology_t topology,
			      int processor_id, int *socket_id, int *core_id)
{
  hwloc_obj_t proc, core, socket;
  /* find the processor object */
  proc = hwloc_get_proc_obj_by_os_index(topology, processor_id);
  if (!proc)
    return -1;
  /* find the above core object */
  core = hwloc_get_parent_obj_by_type(topology, HWLOC_OBJ_CORE, proc);
  if (!core)
    return -1;
  /* find the above socket object */
  socket = hwloc_get_parent_obj_by_type(topology, HWLOC_OBJ_SOCKET, core);
  if (!socket)
    return -1;
  /* return their ids */
  *socket_id = socket->os_index;
  *core_id = core->os_index;
  return 0;
}

/** \brief Return the number of processors and the maximal os_index.
 *
 * \note This function only operates on online processors.
 */
static __inline int
hwloc_plpa_get_processor_data(hwloc_topology_t topology,
			      int *num_processors, int *max_processor_id)
{
  hwloc_obj_t proc = NULL;
  int max = -1;
  int i = 0;
  /* walk the list of processors and update the count and maximal id */
  while ((proc = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PROC, proc)) != NULL) {
    i++;
    if (proc->os_index > max)
      max = proc->os_index;
  }
  *num_processors = i;
  *max_processor_id = max;
  return 0;
}

/** \brief Return the os_index of the given processor logical index.
 *
 * \note This function only operates on online processors.
 */
static __inline int
hwloc_plpa_get_processor_id(hwloc_topology_t topology,
			    int processor_num, int *processor_id)
{
  hwloc_obj_t proc;
  proc = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PROC, processor_num);
  if (!proc)
    return -1;
  *processor_id = proc->os_index;
  return 0;
}

/** \brief Check whether the processor given by os_index exists and is online.
 */
static __inline int
hwloc_plpa_get_processor_flags(hwloc_topology_t topology, int processor_id, int *exists, int *online)
{
  hwloc_obj_t proc;
  /* find the processor */
  proc = hwloc_get_proc_obj_by_os_index(topology, processor_id);
  if (proc) {
    /* if found, it exists and is online */
    *exists = 1;
    *online = 1;
  } else {
    /* if not found, check whether it is in the online cpuset */
    /* FIXME exists / online */
    hwloc_const_cpuset_t cpuset = hwloc_topology_get_online_cpuset(topology);
    *exists = (hwloc_cpuset_isset(cpuset, processor_id) != 0);
    *online = 0;
  }
  return 0;
}

/** \brief Return the number of sockets and the maximal os_index.
 */
static __inline int
hwloc_plpa_get_socket_info(hwloc_topology_t topology,
			   int *num_sockets, int *max_socket_id)
{
  hwloc_obj_t socket = NULL;
  int num = 0, max = -1;
  /* walk socket objects in the system */
  while ((socket = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket)) != NULL) {
    num++;
    if (socket->os_index > max)
      max = socket->os_index;
  }
  *num_sockets = num;
  *max_socket_id = max;
  return 0;
}

/** \brief Return the os_index of socket given by logical index.
 */
static __inline int
hwloc_plpa_get_socket_id(hwloc_topology_t topology,
			 int socket_num, int *socket_id)
{
  hwloc_obj_t socket = NULL;
  socket = hwloc_get_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket_num);
  if (!socket)
    return -1;
  *socket_id = socket->os_index;
  return 0;
}

/** \brief Return the number of cores within a socket, and the maximal os_index.
 */
static __inline int
hwloc_plpa_get_core_info(hwloc_topology_t topology,
			 int socket_id, int *num_cores, int *max_core_id)
{
  hwloc_obj_t socket = NULL, core = NULL;
  int num = 0, max = -1;
  /* find the socket */
  while ((socket = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket)) != NULL
	 && socket->os_index != socket_id);
  if (!socket)
    return -1;
  /* walk core objects in the system */
  while ((core = hwloc_get_next_obj_inside_cpuset_by_type(topology, socket->cpuset, HWLOC_OBJ_CORE, core)) != NULL) {
    num++;
    if (core->os_index > max)
      max = core->os_index;
  }
  *num_cores = num;
  *max_core_id = max;
  return 0;
}

/** \brief Return the os_index of core given by logical index within a socket.
 */
static __inline int
hwloc_plpa_get_core_id(hwloc_topology_t topology,
		       int socket_id, int core_num, int *core_id)
{
  hwloc_obj_t socket = NULL, core = NULL;
  /* find the socket */
  while ((socket = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket)) != NULL
	 && socket->os_index != socket_id);
  if (!socket)
    return -1;
  /* find the core within this socket */
  core = hwloc_get_obj_inside_cpuset_by_type(topology, socket->cpuset, HWLOC_OBJ_CORE, core_num);
  if (!core)
    return -1;
  *core_id = core->os_index;
  return 0;
}

/** \brief Check whether a given core within a given socket exists.
 *
 * \note This function does not check whether a core is online since
 * it does not make sense on recent machines where logical processors
 * may be independently disabled.
 */
static __inline int
hwloc_plpa_get_core_flags(hwloc_topology_t topology,
			  int socket_id, int core_id,
			  int *exists)
{
  hwloc_obj_t socket = NULL, core = NULL;
  /* find the socket */
  while ((socket = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_SOCKET, socket)) != NULL
	 && socket->os_index != socket_id);
  if (!socket) {
    *exists = 0;
    return 0;
  }
  /* find the core within this socket */
  while ((core = hwloc_get_next_obj_inside_cpuset_by_type(topology, socket->cpuset, HWLOC_OBJ_CORE, core)) != NULL
	 && core->os_index != core_id);
  if (!core) {
    *exists = 0;
    return 0;
  }
  *exists = 1;
  return 0;
}


/** @} */

#endif /* HWLOC_PLPA_H */
