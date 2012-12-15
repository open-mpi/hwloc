/*
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and the NVIDIA Management Library.
 *
 * Applications that use both hwloc and the NVIDIA Management Library may want to
 * include this file so as to get topology information for NVML devices.
 */

#ifndef HWLOC_NVML_H
#define HWLOC_NVML_H

#include <hwloc.h>
#include <hwloc/autogen/config.h>
#include <hwloc/linux.h>
#include <hwloc/helper.h>

#include <nvml.h>


#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup hwlocality_nvml NVIDIA Management Library Specific Functions
 * @{
 */

/** \brief Get the CPU set of logical processors that are physically
 * close to NVML device \p device.
 *
 * For the given NVML device \p idx, read the corresponding
 * kernel-provided cpumap file and return the corresponding CPU set.
 * This function is currently only implemented in a meaningful way for
 * Linux; other systems will simply get a full cpuset.
 *
 * This function does not require I/O device detection to be enabled,
 * neither PCI and NVML components. However, it only offers the locality
 * of the device. If more information about the device is needed,
 * OS objects should be used instead (see hwloc_nvml_get_device_osdev()).
 *
 * Topology \p topology must match the current machine.
 */
static __hwloc_inline int
hwloc_nvml_get_device_cpuset(hwloc_topology_t topology __hwloc_attribute_unused,
			     nvmlDevice_t device, hwloc_cpuset_t set)
{
#ifdef HWLOC_LINUX_SYS
  /* If we're on Linux, use the sysfs mechanism to get the local cpus */
#define HWLOC_CUDA_DEVICE_SYSFS_PATH_MAX 128
  char path[HWLOC_CUDA_DEVICE_SYSFS_PATH_MAX];
  FILE *sysfile = NULL;
  nvmlReturn_t nvres;
  nvmlPciInfo_t pci;

  if (!hwloc_topology_is_thissystem(topology)) {
    errno = EINVAL;
    return -1;
  }

  nvres = nvmlDeviceGetPciInfo(device, &pci);
  if (NVML_SUCCESS != nvres) {
    errno = EINVAL;
    return -1;
  }

  sprintf(path, "/sys/bus/pci/devices/%04x:%02x:%02x.0/local_cpus", pci.domain, pci.bus, pci.device);
  sysfile = fopen(path, "r");
  if (!sysfile)
    return -1;

  hwloc_linux_parse_cpumap_file(sysfile, set);
  if (hwloc_bitmap_iszero(set))
    hwloc_bitmap_copy(set, hwloc_topology_get_complete_cpuset(topology));

  fclose(sysfile);
#else
  /* Non-Linux systems simply get a full cpuset */
  hwloc_bitmap_copy(set, hwloc_topology_get_complete_cpuset(topology));
#endif
  return 0;
}

/** \brief Get the hwloc object for the PCI device corresponding to NVML device \p device.
 *
 * For the given NVML device \p idx, return the hwloc OS device
 * corresponding to the NVML device. Returns NULL if there is none.
 *
 * PCI and NVML components must be enabled in the topology,
 * as well as IO device detection.
 * If these are not available, the locality of the object may
 * still be found using hwloc_nvml_get_device_cpuset().
 *
 * \note The corresponding hwloc PCI device may be found by looking
 * at the result parent pointer.
 */
static __hwloc_inline hwloc_obj_t
hwloc_nvml_get_device_osdev(hwloc_topology_t topology, nvmlDevice_t device)
{
  hwloc_obj_t osdev;
  nvmlReturn_t nvres;
  nvmlPciInfo_t pci;

  nvres = nvmlDeviceGetPciInfo(device, &pci);
  if (NVML_SUCCESS != nvres)
    return NULL;

  osdev = NULL;
  while ((osdev = hwloc_get_next_osdev(topology, osdev)) != NULL) {
    hwloc_obj_t pcidev = osdev->parent;
    if (pcidev
	&& pcidev->type == HWLOC_OBJ_PCI_DEVICE
	&& pcidev->attr->pcidev.domain == pci.domain
	&& pcidev->attr->pcidev.bus == pci.bus
	&& pcidev->attr->pcidev.dev == pci.device
	&& pcidev->attr->pcidev.func == 0)
      return osdev;
  }

  return NULL;
}

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_NVML_H */
