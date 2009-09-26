/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and infiniband verbs.
 *
 * Applications that use both hwloc and infiniband verbs may want to include
 * this file so as to get topology information for infiniband hardware.
 */

#ifndef HWLOC_IBVERBS_H
#define HWLOC_IBVERBS_H

#include <hwloc.h>
#include <hwloc/linux.h>

#include <infiniband/verbs.h>

/** \brief Get the CPU set \p set of logical processors that are physicall close to device \p ibdev.
 *
 * For the given infiniband device \p ibdev, read the corresponding kernel-provided cpumap file
 * and store it inside the given CPU set \p set.
 */

static inline int
hwloc_ibverbs_get_device_cpuset(struct ibv_device *ibdev, hwloc_cpuset_t *set)
{
#define HWLOC_IBVERBS_SYSFS_PATH_MAX 128
  char path[HWLOC_IBVERBS_SYSFS_PATH_MAX];
  FILE *sysfile = NULL;

  sprintf(path, "/sys/class/infiniband/%s/device/local_cpus",
	  ibv_get_device_name(ibdev));
  sysfile = fopen(path, "r");
  if (!sysfile)
    return -1;

  hwloc_linux_parse_cpumap_file(sysfile, set);

  fclose(sysfile);
  return 0;
}

#endif /* HWLOC_IBVERBS_H */
