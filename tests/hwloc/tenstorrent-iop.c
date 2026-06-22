/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Tenstorrent USA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * See COPYING in top-level directory.
 *
 * Tests hwloc/tenstorrent.h (and optionally hwloc/tenstorrent_metal.h) helpers.
 * Built only when configure passed --enable-tt-metal-free-iop.
 *
 *   make -C tests/hwloc check-TESTS TESTS=tenstorrent-iop
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hwloc.h"
#include "hwloc/tenstorrent.h"

#ifdef HWLOC_TT_METAL_IOP_TEST
#include "hwloc/tenstorrent_metal.h"
#endif

int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t o;
  hwloc_cpuset_t set;
  int err;

  err = hwloc_topology_init(&topology);
  if (err)
    return 1;
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PCI_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  err = hwloc_topology_load(topology);
  if (err) {
    hwloc_topology_destroy(topology);
    return 1;
  }

  o = hwloc_tenstorrent_get_osdev_by_name(topology, "tenstorrent0");
  if (o) {
    if (!hwloc_tenstorrent_obj_is_osdev(o)) {
      fprintf(stderr, "tenstorrent-iop: is_osdev failed for tenstorrent0\n");
      hwloc_topology_destroy(topology);
      return 1;
    }
    set = hwloc_bitmap_alloc();
    if (!set) {
      hwloc_topology_destroy(topology);
      return 1;
    }
    err = hwloc_tenstorrent_get_osdev_cpuset(topology, o, set);
    if (err) {
      fprintf(stderr, "tenstorrent-iop: get_osdev_cpuset returned %d\n", err);
      hwloc_bitmap_free(set);
      hwloc_topology_destroy(topology);
      return 1;
    }
    hwloc_bitmap_free(set);
  }

#ifdef HWLOC_TT_METAL_IOP_TEST
  /* Compile-only exercise of the metal header (PCI product lookup may return NULL). */
  (void) hwloc_tenstorrent_get_osdev_by_pci_product(topology, 0x1e52u, 0x401eu);
#endif

  hwloc_topology_destroy(topology);
  return 0;
}
