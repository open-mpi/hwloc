/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and Linux.
 *
 * Applications that use hwloc on Linux may want to include this file
 * if using some low-level Linux features.
 */

#ifndef HWLOC_LINUX_H
#define HWLOC_LINUX_H

#include <hwloc.h>
#include <stdio.h>

/** \defgroup hwlocality_linux_cpumap Helpers for manipulating linux kernel cpumap files
 * @{
 */

/** \brief Convert a linux kernel cpumap file \p file into hwloc CPU set.
 *
 * Might be used when reading CPU set from sysfs attributes such as topology
 * and caches for processors, or local_cpus for devices.
 */
extern hwloc_cpuset_t hwloc_linux_parse_cpumap_file(FILE *file);

/** @} */

#endif /* HWLOC_GLIBC_SCHED_H */
