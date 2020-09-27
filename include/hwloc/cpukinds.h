/*
 * Copyright © 2020 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Kinds of CPU cores.
 */

#ifndef HWLOC_CPUKINDS_H
#define HWLOC_CPUKINDS_H

#include "hwloc.h"

#ifdef __cplusplus
extern "C" {
#elif 0
}
#endif

/** \defgroup hwlocality_cpukinds Kinds of CPU cores
 *
 * Platforms with heterogeneous CPUs may have some cores with
 * different features or frequencies.
 * This API exposes identical PUs in sets called CPU kinds.
 * Each PU of the topology may only be in a single kind.
 *
 * The number of kinds may be obtained with hwloc_cpukinds_get_nr().
 * If the platform is homogeneous, there may be a single kind
 * with all PUs.
 * If the platform or operating system does not expose any
 * information about CPU cores, there may be no kind at all.
 *
 * The index of the kind that describes a given CPU set
 * (if any, and not partially)
 * may be obtained with hwloc_cpukinds_get_by_cpuset().
 *
 * From the index of a kind, it is possible to retrieve information
 * with hwloc_cpukinds_get_info():
 * an abstracted efficiency value,
 * and an array of info attributes
 * (for instance the "CoreType" and "FrequencyMaxMHz").
 *
 * A higher efficiency value means intrinsic greater performance
 * (and possibly less performance/power efficiency).
 * Kinds with lower efficiency are ranked first:
 * Passing 0 as \p kind_index to hwloc_cpukinds_get_info() will
 * return information about the less efficient CPU kind.
 *
 * Efficiency values are gathered from the Windows API when available.
 * Otherwise hwloc tries to compute efficiencies by comparing CPU kinds.
 * If it fails, all kinds will have an unknown efficiency (\c -1),
 * and they are not indexed/ordered in any specific way.
 *
 * @{
 */

/** \brief Get the number of different kinds of CPU cores in the topology.
 *
 * \p flags must be \c 0 for now.
 *
 * \return The number of CPU kinds (positive integer) on success.
 * \return \c 0 if no information about kinds was found.
 * \return \c -1 with \p errno set to \c EINVAL if \p flags is invalid.
 */
HWLOC_DECLSPEC int
hwloc_cpukinds_get_nr(hwloc_topology_t topology,
                      unsigned long flags);

/** \brief Get the index of the CPU kind that contains CPUs listed in \p cpuset.
 *
 * \p flags must be \c 0 for now.
 *
 * \return The index of the CPU kind (positive integer or 0) on success.
 * \return \c -1 with \p errno set to \c EXDEV if \p cpuset is
 * only partially included in the some kind.
 * \return \c -1 with \p errno set to \c ENOENT if \p cpuset is
 * not included in any kind, even partially.
 * \return \c -1 with \p errno set to \c EINVAL if parameters are invalid.
 */
HWLOC_DECLSPEC int
hwloc_cpukinds_get_by_cpuset(hwloc_topology_t topology,
                             hwloc_const_bitmap_t cpuset,
                             unsigned long flags);

/** \brief Get the CPU set and infos about a CPU kind in the topology.
 *
 * \p kind_index identifies one kind of CPU between 0 and the number
 * of kinds returned by hwloc_cpukinds_get_nr() minus 1.
 *
 * If not \c NULL, the bitmap \p cpuset will be filled with
 * the set of PUs of this kind.
 *
 * The integer pointed by \p efficiency, if not \c NULL will, be filled
 * with the ranking of this kind of CPU in term of efficiency.
 * It ranges from \c 0 to the number of kinds
 * (as reported by hwloc_cpukinds_get_nr()) minus 1.
 *
 * Kinds with lower efficiency are reported first.
 *
 * If there is a single kind in the topology, its efficiency \c 0.
 * If the efficiency of some kinds of cores is unknown,
 * the efficiency of all kinds is set to \c -1,
 * and kinds are reported in no specific order.
 *
 * The array of info attributes (for instance the "CoreType",
 * "FrequencyMaxMHz" or "FrequencyBaseMHz")
 * and its length are returned in \p infos or \p nr_infos.
 * The array belongs to the topology, it should not be freed or modified.
 *
 * If \p nr_infos or \p infos is \c NULL, no info is returned.
 *
 * \p flags must be \c 0 for now.
 *
 * \return \c 0 on success.
 * \return \c -1 with \p errno set to \c ENOENT if \p kind_index does not match any CPU kind.
 * \return \c -1 with \p errno set to \c EINVAL if parameters are invalid.
 */
HWLOC_DECLSPEC int
hwloc_cpukinds_get_info(hwloc_topology_t topology,
                        unsigned kind_index,
                        hwloc_bitmap_t cpuset,
                        int *efficiency,
                        unsigned *nr_infos, struct hwloc_info_s **infos,
                        unsigned long flags);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_CPUKINDS_H */
