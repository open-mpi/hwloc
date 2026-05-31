/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 *
 * Exercises the Tenstorrent discovery component when present (Linux + KMD).
 * On other platforms, or when no /dev/tenstorrent devices exist, this test is a no-op.
 *
 * Run from the build tree as:
 *   make check-tenstorrent
 * (top-level Makefile forwards to tests/hwloc), or from tests/hwloc:
 *   make check-TESTS TESTS=tenstorrent
 * Do not use "make check TESTS=tenstorrent" here: that exports TESTS to SUBDIRS
 * (xml/, ports/, …) and breaks the Automake test harness.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hwloc.h"

static int backend_is_tenstorrent(hwloc_topology_t topology)
{
  struct hwloc_infos_s *infos = hwloc_topology_get_infos(topology);
  unsigned i;
  for (i = 0; i < infos->count; i++)
    if (!strcmp(infos->array[i].name, "Backend")
        && !strcmp(infos->array[i].value, "Tenstorrent"))
      return 1;
  return 0;
}

static int count_tenstorrent_osdevs(hwloc_topology_t topology)
{
  hwloc_obj_t o = NULL;
  unsigned n = 0;
  while ((o = hwloc_get_next_osdev(topology, o)) != NULL) {
    if (o->subtype && !strcmp(o->subtype, "Tenstorrent")
        && o->name && !strncmp(o->name, "tenstorrent", 11))
      n++;
  }
  return (int) n;
}

int main(void)
{
  hwloc_topology_t topology;
  int n_tt, err;

  hwloc_topology_init(&topology);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PCI_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  err = hwloc_topology_load(topology);
  if (err < 0) {
    hwloc_topology_destroy(topology);
    return 1;
  }

  n_tt = count_tenstorrent_osdevs(topology);
  if (n_tt > 0 && !backend_is_tenstorrent(topology)) {
    fprintf(stderr, "Found Tenstorrent OS devices but topology Backend is not Tenstorrent\n");
    hwloc_topology_destroy(topology);
    return 1;
  }

  hwloc_topology_destroy(topology);
  return 0;
}
