/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Helpers to relate Tenstorrent OS devices (from hwloc's tenstorrent
 * discovery component) to PCI placement and host CPU locality.
 *
 * This header does not include or link tt-metal.  It is installed when
 * hwloc is configured with \c --enable-tt-metal-free-iop (or with
 * \c --enable-tt-metal-iop, which implies the free IOP headers).
 *
 * Applications should load a topology with I/O devices enabled (for example
 * \c HWLOC_TYPE_FILTER_KEEP_IMPORTANT on \c HWLOC_OBJ_OS_DEVICE and
 * \c HWLOC_TOPOLOGY_FLAG_INCLUDE_IO_DEVICES or \c lstopo \--whole-io).
 */

#ifndef HWLOC_TENSTORRENT_H
#define HWLOC_TENSTORRENT_H

#include "hwloc.h"
#include "hwloc/helper.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup hwlocality_tenstorrent Interoperability with Tenstorrent devices
 * @{
 */

/** \brief Return whether an object is a Tenstorrent OS device from hwloc discovery.
 *
 * \return 1 if \p obj is an OS device with subtype "Tenstorrent", else 0.
 */
static __hwloc_inline int
hwloc_tenstorrent_obj_is_osdev(hwloc_obj_t obj)
{
  return obj
         && obj->type == HWLOC_OBJ_OS_DEVICE
         && obj->subtype
         && !strcmp(obj->subtype, "Tenstorrent");
}

/** \brief Find the Tenstorrent OS device under a PCI device.
 *
 * \return The first Tenstorrent OS child of \p pcidev, or \c NULL.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_under_pcidev(hwloc_topology_t topology __hwloc_attribute_unused,
                                         hwloc_obj_t pcidev)
{
  hwloc_obj_t cur = NULL;
  if (!pcidev || pcidev->type != HWLOC_OBJ_PCI_DEVICE)
    return NULL;
  while ((cur = hwloc_get_next_child(topology, pcidev, cur)) != NULL)
    if (hwloc_tenstorrent_obj_is_osdev(cur))
      return cur;
  return NULL;
}

/** \brief Find the Tenstorrent OS device matching a KMD index.
 *
 * This compares the \c TenstorrentDeviceIndex info (from the Linux KMD
 * character device minor) to \p kmd_index.
 *
 * \return A matching OS device, or \c NULL.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_by_kmd_index(hwloc_topology_t topology, int kmd_index)
{
  hwloc_obj_t o = NULL;
  char expect[16];
  (void) snprintf(expect, sizeof(expect), "%d", kmd_index);
  while ((o = hwloc_get_next_osdev(topology, o)) != NULL) {
    const char *idx;
    if (!hwloc_tenstorrent_obj_is_osdev(o))
      continue;
    idx = hwloc_obj_get_info_by_name(o, "TenstorrentDeviceIndex");
    if (idx && !strcmp(idx, expect))
      return o;
  }
  return NULL;
}

/** \brief Find the Tenstorrent OS device by logical name (e.g. \c tenstorrent0).
 *
 * \return A matching OS device, or \c NULL.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_by_name(hwloc_topology_t topology, const char *name)
{
  hwloc_obj_t o = NULL;
  if (!name)
    return NULL;
  while ((o = hwloc_get_next_osdev(topology, o)) != NULL)
    if (hwloc_tenstorrent_obj_is_osdev(o) && o->name && !strcmp(o->name, name))
      return o;
  return NULL;
}

/** \brief Find the Tenstorrent OS device for a PCI bus location.
 *
 * \return The Tenstorrent OS device under the PCI function, or \c NULL.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_by_pci_busid(hwloc_topology_t topology,
                                         unsigned domain, unsigned bus,
                                         unsigned dev, unsigned func)
{
  hwloc_obj_t pci = hwloc_get_pcidev_by_busid(topology, domain, bus, dev, func);
  if (!pci)
    return NULL;
  return hwloc_tenstorrent_get_osdev_under_pcidev(topology, pci);
}

/** \brief Find the Tenstorrent OS device for a PCI bus id string.
 *
 * Accepts \c xxxx:yy:zz.t or \c yy:zz.t (see hwloc_get_pcidev_by_busidstring()).
 *
 * \return The Tenstorrent OS device, or \c NULL.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_by_pci_busidstring(hwloc_topology_t topology, const char *busid)
{
  hwloc_obj_t pci = hwloc_get_pcidev_by_busidstring(topology, busid);
  if (!pci)
    return NULL;
  return hwloc_tenstorrent_get_osdev_under_pcidev(topology, pci);
}

/** \brief Get the PCI device parent of a Tenstorrent OS device, if any.
 *
 * \return The PCI object, or \c NULL if filtered or attached under the root.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_pcidev_for_osdev(hwloc_obj_t osdev)
{
  hwloc_obj_t p;
  if (!hwloc_tenstorrent_obj_is_osdev(osdev))
    return NULL;
  p = osdev->parent;
  while (p && p->type != HWLOC_OBJ_PCI_DEVICE)
    p = p->parent;
  return (p && p->type == HWLOC_OBJ_PCI_DEVICE) ? p : NULL;
}

/** \brief Fill \p set with CPUs close to a Tenstorrent OS device.
 *
 * Uses the first ancestor with a cpuset when present; otherwise parses the
 * \c PCIBusID info and uses hwloc_get_pci_busid_cpuset().
 *
 * \return 0 on success, \c -1 on error with \c errno set.
 */
static __hwloc_inline int
hwloc_tenstorrent_get_osdev_cpuset(hwloc_topology_t topology, hwloc_obj_t osdev,
                                   hwloc_cpuset_t set)
{
  hwloc_obj_t obj;
  const char *busid;
  unsigned domain, bus, dev, func;

  if (!hwloc_topology_is_thissystem(topology)) {
    errno = EINVAL;
    return -1;
  }
  if (!hwloc_tenstorrent_obj_is_osdev(osdev)) {
    errno = EINVAL;
    return -1;
  }

  obj = osdev;
  while (obj && !obj->cpuset)
    obj = obj->parent;
  if (obj && obj->cpuset) {
    hwloc_bitmap_copy(set, obj->cpuset);
    return 0;
  }

  busid = hwloc_obj_get_info_by_name(osdev, "PCIBusID");
  if (!busid
      || sscanf(busid, "%x:%x:%x.%u", &domain, &bus, &dev, &func) != 4) {
    errno = ENODEV;
    return -1;
  }
  return hwloc_get_pci_busid_cpuset(topology, set, domain, bus, dev, func);
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_TENSTORRENT_H */
