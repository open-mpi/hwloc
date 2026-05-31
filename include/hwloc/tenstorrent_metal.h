/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Optional Tenstorrent helpers including tt-metal UMD \c pci_ids.h.
 *
 * Installed only when hwloc is configured with \c --enable-tt-metal-iop and
 * \c --with-tt-metal-path=DIR (tt-metal repository root).  This implies
 * \c --enable-tt-metal-free-iop (\c hwloc/tenstorrent.h is installed as well).
 *
 * Compile user code with the same \c -I.../tt_metal/third_party/umd/device/api
 * flags as reported in \c pkg-config \c hwloc \--cflags when this feature is
 * enabled (see \c HWLOC_TT_METAL_CPPFLAGS in the build log).
 *
 * The included UMD header defines vendor and product PCI IDs; pass those
 * constants to hwloc_tenstorrent_get_osdev_by_pci_product() alongside the
 * free helpers from \c hwloc/tenstorrent.h.
 */

#ifndef HWLOC_TENSTORRENT_METAL_H
#define HWLOC_TENSTORRENT_METAL_H

#include "hwloc/tenstorrent.h"

#include <umd/device/pcie/pci_ids.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup hwlocality_tenstorrent_metal Interoperability with tt-metal UMD PCI IDs
 * @{
 */

/** \brief Find the Tenstorrent OS device for a PCI vendor and device id.
 *
 * Typical use with tt-metal \c pci_ids.h macros for \p vendor_id and
 * \p device_id.
 */
static __hwloc_inline hwloc_obj_t
hwloc_tenstorrent_get_osdev_by_pci_product(hwloc_topology_t topology,
                                           uint16_t vendor_id,
                                           uint16_t device_id)
{
  hwloc_obj_t pci = NULL;
  while ((pci = hwloc_get_next_pcidev(topology, pci)) != NULL)
    if (pci->attr->pcidev.vendor_id == vendor_id
        && pci->attr->pcidev.device_id == device_id)
      return hwloc_tenstorrent_get_osdev_under_pcidev(topology, pci);
  return NULL;
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_TENSTORRENT_METAL_H */
