/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2012-2026 Inria.  All rights reserved.
 * Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.
 * Written by Advanced Micro Devices,
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and the AMD SMI Management Library.
 *
 * Applications that use both hwloc and the AMD SMI Management Library may want to
 * include this file so as to get topology information for AMD GPU devices.
 */

#ifndef HWLOC_AMDSMI_H
#define HWLOC_AMDSMI_H

#include "hwloc.h"
#include "hwloc/autogen/config.h"
#include "hwloc/helper.h"

#include <amd_smi/amdsmi.h>


#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup hwlocality_amdsmi Interoperability with the AMD SMI Management Library
 *
 * This interface offers ways to retrieve topology information about
 * devices managed by the AMD SMI Management Library.
 *
 * @{
 */

/** \brief Get the CPU set of logical processors that are physically
 * close to AMD GPU device whose index is \p dv_ind.
 *
 * Store in \p set the CPU-set describing the locality of the AMD GPU device
 * whose processor handle is \p proc.
 *
 * I/O devices detection and the RSMI component are not needed in the
 * topology.
 *
 * The function only returns the locality of the device.
 * If more information about the device is needed, OS objects should
 * be used instead, see hwloc_amdsmi_get_device_osdev()
 * and hwloc_amdsmi_get_device_osdev_by_index().
 *
 * This function is currently only implemented in a meaningful way for
 * Linux; other systems will simply get the cpuset of the entire topology.
 *
 * \return 0 on success.
 * \return -1 on error, for instance if device information could not be found.
 */
static __hwloc_inline int
hwloc_amdsmi_get_device_cpuset(hwloc_topology_t topology,
                               amdsmi_processor_handle proc, hwloc_cpuset_t set)
{
  amdsmi_status_t ret;
  uint64_t bdfid = 0;
  unsigned domain, device, bus;

  if (!hwloc_topology_is_thissystem(topology)) {
    errno = EINVAL;
    return -1;
  }

  ret = amdsmi_get_gpu_bdf_id(proc, &bdfid);
  if (AMDSMI_STATUS_SUCCESS != ret) {
    errno = EINVAL;
    return -1;
  }
  domain = (bdfid>>32) & 0xffffffff;
  bus = ((bdfid & 0xffff)>>8) & 0xff;
  device = ((bdfid & 0xff)>>3) & 0x1f;

  return hwloc_get_pci_busid_cpuset(topology, set,
                                    domain, bus, device, 0);
}

/** \brief Get the hwloc OS device object corresponding to the
 * AMD GPU device whose HIP index is \p hip_id.
 *
 * \return The hwloc OS device object describing the AMD GPU device whose
 * HIP index is \p hip_id.
 * \return \c NULL if none could be found.
 *
 * The topology \p topology does not necessarily have to match the current
 * machine. For instance the topology may be an XML import of a remote host.
 * I/O devices detection and the RSMI component must be enabled in the
 * topology.
 *
 * \note The corresponding PCI device object can be obtained by looking
 * at the OS device parent object (unless PCI devices are filtered out).
 */
static __hwloc_inline hwloc_obj_t
hwloc_amdsmi_get_device_osdev_by_index(hwloc_topology_t topology, uint32_t hip_id)
{
  hwloc_obj_t osdev = NULL;
  while ((osdev = hwloc_get_next_osdev(topology, osdev)) != NULL) {
    if ((osdev->attr->osdev.types & (HWLOC_OBJ_OSDEV_GPU|HWLOC_OBJ_OSDEV_COPROC)) /* assume future RSMI devices will be at least GPU or COPROC */
      && osdev->name
      && !strncmp("rsmi", osdev->name, 4)
      && atoi(osdev->name + 4) == (int) hip_id)
      return osdev;
  }
  return NULL;
}

/** \brief Get the hwloc OS device object corresponding to AMD GPU device,
 * whose processor handle is \p proc.
 *
 * \return The hwloc OS device object that describes the given
 * AMD GPU, whose processor handle is \p proc.
 * \return \c NULL if none could be found.
 *
 * Topology \p topology and processor handle \p proc must match the local machine.
 * I/O devices detection and the RSMI component must be enabled in the
 * topology. If not, the locality of the object may still be found using
 * hwloc_amdsmi_get_device_cpuset().
 *
 * \note The corresponding hwloc PCI device may be found by looking
 * at the result parent pointer (unless PCI devices are filtered out).
 */
static __hwloc_inline hwloc_obj_t
hwloc_amdsmi_get_device_osdev(hwloc_topology_t topology, amdsmi_processor_handle proc)
{
  amdsmi_status_t ret;
  amdsmi_enumeration_info_t enum_info;

  ret = amdsmi_get_gpu_enumeration_info(proc, &enum_info);
  if (AMDSMI_STATUS_SUCCESS != ret)
    return NULL;

  return hwloc_amdsmi_get_device_osdev_by_index(topology, enum_info.hip_id);
}

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_AMDSMI_H */
