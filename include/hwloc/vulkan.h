/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2009-2025 Christopher Taylor.  All rights reserved.
 * Copyright © 2025-2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Macros to help interaction between hwloc and the Vulkan API.
 *
 * Applications that use both hwloc and Vulkan may want to
 * include this file so as to get topology information for Vulkan devices.
 */

#ifndef HWLOC_VULKAN_H
#define HWLOC_VULKAN_H

#include "hwloc.h"
#include "hwloc/autogen/config.h"
#include "hwloc/helper.h"
#ifdef HWLOC_LINUX_SYS
#include "hwloc/linux.h"
#endif

#include <vulkan/vulkan.h>


#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup hwlocality_vulkan Interoperability with the Vulkan API.
 *
 * This interface offers ways to retrieve topology information about
 * devices managed by the Vulkan API.
 *
 * @{
 */

/** \brief Return the domain, bus and device IDs of the Vulkan physical device \p device.
 *
 * Device \p device must match the local machine.
 *
 * \return 0 on success.
 * \return -1 on error, for instance if device information could not be found.
 */
static __hwloc_inline int
hwloc_vulkan_get_device_pci_busid(VkPhysicalDevice device,
                                  unsigned *domain, unsigned *bus, unsigned *dev, unsigned *func)
{
  VkPhysicalDeviceProperties props;
  VkPhysicalDevicePCIBusInfoPropertiesEXT pciprops;
  VkExtensionProperties *extprops = NULL;
  uint32_t extcount = 0;
  VkResult res;
  int i, have_ext = 0;
  VkStructureType st = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  void *pnext = &props;
  VkPhysicalDeviceProperties2 props2;

  vkEnumerateDeviceExtensionProperties(device, NULL, &extcount, NULL);
  if (extcount > 0) {
    extprops = malloc(extcount * sizeof(*extprops));
    if (extprops)
      vkEnumerateDeviceExtensionProperties(device, NULL, &extcount, extprops);
  }

  if (extprops) {
    for (i = 0; i < (int) extcount; i++) {
      if (!strcmp(extprops[i].extensionName, VK_EXT_PCI_BUS_INFO_EXTENSION_NAME)) {
        have_ext = 1;
        break;
      }
    }
    free(extprops);
  }

  if (have_ext) {
    pciprops.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT;
    pciprops.pNext = NULL;
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &pciprops;
    vkGetPhysicalDeviceProperties2(device, &props2);
    *domain = pciprops.pciDomain;
    *bus = pciprops.pciBus;
    *dev = pciprops.pciDevice;
    *func = pciprops.pciFunction;
    return 0;
  }

  vkGetPhysicalDeviceProperties(device, &props);

  if (props.vendorID == 0x10de || props.vendorID == 0x1002) {
    unsigned int businfo = props.deviceID;
    *domain = 0;
    *bus = (businfo >> 8) & 0xff;
    *dev = (businfo >> 3) & 0x1f;
    *func = businfo & 0x7;
    return 0;
  }

  return -1;
}

/** \brief Get the CPU set of logical processors that are physically
 * close to the Vulkan physical device \p device.
 *
 * Store in \p set the CPU-set describing the locality of
 * the Vulkan device \p device.
 *
 * I/O devices detection and the Vulkan component are not needed in the topology.
 *
 * The function only returns the locality of the device.
 * If more information about the device is needed, OS objects should
 * be used instead, see hwloc_vulkan_get_device_osdev().
 *
 * This function is currently only implemented in a meaningful way for
 * Linux; other systems will simply get the cpuset of the entire topology.
 *
 * \return 0 on success.
 * \return -1 on error, for instance if device information could not be found.
 */
static __hwloc_inline int
hwloc_vulkan_get_device_cpuset(hwloc_topology_t topology __hwloc_attribute_unused,
                               VkPhysicalDevice device, hwloc_cpuset_t set)
{
  unsigned domain, bus, dev, func;

  if (!hwloc_topology_is_thissystem(topology)) {
    errno = EINVAL;
    return -1;
  }

  if (hwloc_vulkan_get_device_pci_busid(device, &domain, &bus, &dev, &func) < 0) {
    hwloc_bitmap_copy(set, hwloc_topology_get_complete_cpuset(topology));
    return 0;
  }

  return hwloc_get_pci_busid_cpuset(topology, set, domain, bus, dev, func);
}

/** \brief Get the hwloc OS device object corresponding to Vulkan physical device
 * \p device.
 *
 * \return The hwloc OS device object that describes the given Vulkan device \p device.
 * \return \c NULL if none could be found.
 *
 * Topology \p topology and device \p device must match the local machine.
 * I/O devices detection and the Vulkan component must be enabled in the topology.
 * If not, the locality of the object may still be found using
 * hwloc_vulkan_get_device_cpuset().
 *
 * \note The corresponding hwloc PCI device may be found by looking
 * at the result parent pointer (unless PCI devices are filtered out).
 */
static __hwloc_inline hwloc_obj_t
hwloc_vulkan_get_device_osdev(hwloc_topology_t topology, VkPhysicalDevice device)
{
  VkPhysicalDeviceProperties props;
  unsigned domain, bus, dev, func;
  hwloc_obj_t osdev;

  if (!hwloc_topology_is_thissystem(topology)) {
    errno = EINVAL;
    return NULL;
  }

  if (hwloc_vulkan_get_device_pci_busid(device, &domain, &bus, &dev, &func) < 0) {
    errno = EINVAL;
    return NULL;
  }

  osdev = NULL;
  while ((osdev = hwloc_get_next_osdev(topology, osdev)) != NULL) {
    hwloc_obj_t pcidev;

    if (strncmp(osdev->name, "vulkan", 6))
      continue;

    pcidev = osdev;
    while (pcidev && pcidev->type != HWLOC_OBJ_PCI_DEVICE)
      pcidev = pcidev->parent;
    if (!pcidev)
      continue;

    if (pcidev
      && pcidev->type == HWLOC_OBJ_PCI_DEVICE
      && pcidev->attr->pcidev.domain == domain
      && pcidev->attr->pcidev.bus == bus
      && pcidev->attr->pcidev.dev == dev
      && pcidev->attr->pcidev.func == func)
      return osdev;
  }

  return NULL;
}

/** \brief Get the hwloc OS device object corresponding to the
 * Vulkan device for the given indexes.
 *
 * \return The hwloc OS device object describing the Vulkan device
 * whose driver index is \p driver_index,
 * and whose device index within this driver is \p device_index.
 * \return \c NULL if there is none.
 *
 * The topology \p topology does not necessarily have to match the current
 * machine. For instance the topology may be an XML import of a remote host.
 * I/O devices detection and the Vulkan component must be enabled in the topology.
 *
 * \note The corresponding PCI device object can be obtained by looking
 * at the OS device parent object (unless PCI devices are filtered out).
 */
static __hwloc_inline hwloc_obj_t
hwloc_vulkan_get_device_osdev_by_index(hwloc_topology_t topology,
                                       unsigned driver_index, unsigned device_index)
{
  unsigned x = (unsigned) -1, y = (unsigned) -1;
  hwloc_obj_t osdev = NULL;
  while ((osdev = hwloc_get_next_osdev(topology, osdev)) != NULL) {
    if ((osdev->attr->osdev.types & HWLOC_OBJ_OSDEV_COPROC)
      && osdev->name
      && sscanf(osdev->name, "vulkan%ud%u", &x, &y) == 2
      && driver_index == x && device_index == y)
      return osdev;
  }
  return NULL;
}

/** @} */


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_VULKAN_H */
