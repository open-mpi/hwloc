/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and OpenFabrics verbs.
 *
 * Applications that use both hwloc and OpenFabrics verbs may want to include
 * this file so as to get topology information for OpenFabrics hardware.
 */

#ifndef HWLOC_IBVERBS_H
#define HWLOC_IBVERBS_H

#include <hwloc.h>
#include <hwloc/linux.h>

#include <infiniband/verbs.h>

/** \brief Get the CPU set of logical processors that are physicall close to device \p ibdev.
 *
 * For the given OpenFabrics device \p ibdev, read the corresponding
 * kernel-provided cpumap file and return the corresponding CPU set.
 */

static inline hwloc_cpuset_t
hwloc_ibverbs_get_device_cpuset(struct ibv_device *ibdev)
{
#define HWLOC_IBVERBS_SYSFS_PATH_MAX 128
  char path[HWLOC_IBVERBS_SYSFS_PATH_MAX];
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
}

#endif /* HWLOC_IBVERBS_H */
