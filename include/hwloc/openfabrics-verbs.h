/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and OpenFabrics
 * verbs.
 *
 * Applications that use both hwloc and OpenFabrics verbs may want to
 * include this file so as to get topology information for OpenFabrics
 * hardware.
 *
 */

#ifndef HWLOC_OPENFABRICS_VERBS_H
#define HWLOC_OPENFABRICS_VERBS_H

#include <hwloc.h>
#include <hwloc/config.h>
#include <hwloc/linux.h>

#include <infiniband/verbs.h>

/** \defgroup hwloc_openfabrics OpenFabrics-Specific Functions
 * @{
 */

/** \brief Get the CPU set of logical processors that are physically
 * close to device \p ibdev.
 *
 * For the given OpenFabrics device \p ibdev, read the corresponding
 * kernel-provided cpumap file and return the corresponding CPU set.
 * This function is currently only implemented in a meaningful way for
 * Linux; other systems will simply get a full cpuset.
 */
static __inline hwloc_cpuset_t
hwloc_ibv_get_device_cpuset(struct ibv_device *ibdev)
{
#if defined(HWLOC_LINUX_SYS)
  /* If we're on Linux, use the verbs-provided sysfs mechanism to
     get the local cpus */
#define HWLOC_OPENFABRICS_VERBS_SYSFS_PATH_MAX 128
  char path[HWLOC_OPENFABRICS_VERBS_SYSFS_PATH_MAX];
  FILE *sysfile = NULL;
  hwloc_cpuset_t set;

  sprintf(path, "/sys/class/infiniband/%s/device/local_cpus",
	  ibv_get_device_name(ibdev));
  sysfile = fopen(path, "r");
  if (!sysfile)
    return NULL;

  set = hwloc_linux_parse_cpumap_file(sysfile);

  fclose(sysfile);
  return set;
#else
  /* Non-Linux systems simply get a full cpuset */
  return hwloc_cpuset_dup(hwloc_get_system_obj(topology)->cpuset);
#endif
}

/** @} */

#endif /* HWLOC_OPENFABRICS_VERBS_H */
