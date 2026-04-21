/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2012-2025 Inria.  All rights reserved.
 * Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.
 * Written by Advanced Micro Devices,
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <assert.h>

#include "hwloc.h"
#include "private/autogen/config.h"

/* check the RSMI and AMD SMI helpers */

static int check_rsmi_backend(hwloc_topology_t topology)
{
  struct hwloc_infos_s *infos = hwloc_topology_get_infos(topology);
  unsigned i;
  for(i=0; i<infos->count; i++)
    if (!strcmp(infos->array[i].name, "Backend")
        && !strcmp(infos->array[i].value, "RSMI"))
      return 1;
  return 0;
}

#ifdef HWLOC_RSMI_USE_ROCM_SMI
#include <rocm_smi/rocm_smi.h>
#include "hwloc/rsmi.h"

static int check_rsmi(hwloc_topology_t topology)
{
  rsmi_status_t ret;
  unsigned count, i;
  int has_rsmi_backend;
  int err = 0;

  rsmi_init(0);

  ret = rsmi_num_monitor_devices(&count);
  if (RSMI_STATUS_SUCCESS != ret || !count) {
    rsmi_shut_down();
    if (RSMI_STATUS_SUCCESS != ret) {
      fprintf(stderr, "Failed to get device count\n");
      return -1;
    } else {
      fprintf(stderr, "No GPU available\n");
      return 0;
    }
  }
  printf("rsmi_num_monitor_devices found %u devices\n", count);

  has_rsmi_backend = check_rsmi_backend(topology);

  for (i=0; i<count; i++) {
    hwloc_bitmap_t set;
    hwloc_obj_t osdev, osdev2, ancestor;
    const char *value;

    osdev = hwloc_rsmi_get_device_osdev(topology, i);
    assert(osdev);
    osdev2 = hwloc_rsmi_get_device_osdev_by_index(topology, i);
    assert(osdev2 == osdev);

    ancestor = hwloc_get_non_io_ancestor_obj(topology, osdev);

    printf("found OSDev %s\n", osdev->name);
    err = strncmp(osdev->name, "rsmi", 4);
    assert(!err);
    assert(atoi(osdev->name+4) == (int) i);

    assert(has_rsmi_backend);

    assert(osdev->attr->osdev.types == (HWLOC_OBJ_OSDEV_COPROC|HWLOC_OBJ_OSDEV_GPU));

    value = hwloc_obj_get_info_by_name(osdev, "GPUModel");
    printf("found OSDev model %s\n", value);

    set = hwloc_bitmap_alloc();
    err = hwloc_rsmi_get_device_cpuset(topology, i, set);
    if (err < 0) {
      printf("failed to get cpuset for device %u\n", i);
    } else {
      char *cpuset_string = NULL;
      hwloc_bitmap_asprintf(&cpuset_string, set);
      printf("got cpuset %s for device %u\n", cpuset_string, i);
      free(cpuset_string);
      if (hwloc_bitmap_isequal(hwloc_topology_get_complete_cpuset(topology), hwloc_topology_get_topology_cpuset(topology)))
        /* only compare if the topology is complete, otherwise things can be significantly different */
        assert(hwloc_bitmap_isequal(set, ancestor->cpuset));
    }
    hwloc_bitmap_free(set);
  }

  rsmi_shut_down();
  return 0;
}
#endif /* HWLOC_RSMI_USE_ROCM_SMI */

#ifdef HWLOC_RSMI_USE_AMD_SMI
#include <amd_smi/amdsmi.h>
#include "hwloc/amdsmi.h"

static int check_amdsmi(hwloc_topology_t topology)
{
  amdsmi_status_t ret;
  unsigned nr_sockets, i;
  amdsmi_socket_handle *sockets;
  int has_rsmi_backend;
  int err = 0;

  amdsmi_init(AMDSMI_INIT_AMD_GPUS);

  ret = amdsmi_get_socket_handles(&nr_sockets, NULL);
  if (ret != AMDSMI_STATUS_SUCCESS || !nr_sockets) {
    amdsmi_shut_down();
    if (AMDSMI_STATUS_SUCCESS != ret) {
      fprintf(stderr, "Failed to get socket count\n");
      return -1;
    } else {
      fprintf(stderr, "No GPU available\n");
      return 0;
    }
  }
  printf("amdsmi_get_socket_handles found %u sockets\n", nr_sockets);

  sockets = malloc(nr_sockets * sizeof(*sockets));
  ret = amdsmi_get_socket_handles(&nr_sockets, sockets);

  has_rsmi_backend = check_rsmi_backend(topology);

  for (i=0; i<nr_sockets; i++) {
    uint32_t nr_procs, j;
    amdsmi_processor_handle *procs;

    ret = amdsmi_get_processor_handles(sockets[i], &nr_procs, NULL);
    if (ret != AMDSMI_STATUS_SUCCESS || !nr_procs)
      continue;
    printf("amdsmi_get_processor_handles found %u processors in socket #%u\n", nr_procs, i);
    procs = malloc(nr_procs * sizeof(*procs));
    ret = amdsmi_get_processor_handles(sockets[i], &nr_procs, procs);

    for(j=0; j<nr_procs; j++) {
      hwloc_bitmap_t set;
      amdsmi_enumeration_info_t enum_info;
      hwloc_obj_t osdev, osdev2, ancestor;
      const char *value;

      osdev = hwloc_amdsmi_get_device_osdev(topology, procs[j]);
      assert(osdev);

      ret = amdsmi_get_gpu_enumeration_info(procs[j], &enum_info);
      
      osdev2 = hwloc_rsmi_get_device_osdev_by_index(topology, enum_info.hip_id);
      assert(osdev2 == osdev);

      ancestor = hwloc_get_non_io_ancestor_obj(topology, osdev);

      printf("found OSDev %s\n", osdev->name);
      err = strncmp(osdev->name, "rsmi", 4);
      assert(!err);
      assert(atoi(osdev->name+4) == (int) enum_info.hip_id);

      assert(has_rsmi_backend);

      assert(osdev->attr->osdev.types == (HWLOC_OBJ_OSDEV_COPROC|HWLOC_OBJ_OSDEV_GPU));

      value = hwloc_obj_get_info_by_name(osdev, "GPUModel");
      printf("found OSDev model %s\n", value);

      set = hwloc_bitmap_alloc();
      err = hwloc_amdsmi_get_device_cpuset(topology, procs[j], set);
      if (err < 0) {
        printf("failed to get cpuset for socket %u processor %u\n", i, j);
      } else {
        char *cpuset_string = NULL;
        hwloc_bitmap_asprintf(&cpuset_string, set);
        printf("got cpuset %s for socket %u processor %u\n", cpuset_string, i, j);
        free(cpuset_string);
        if (hwloc_bitmap_isequal(hwloc_topology_get_complete_cpuset(topology), hwloc_topology_get_topology_cpuset(topology)))
          /* only compare if the topology is complete, otherwise things can be significantly different */
          assert(hwloc_bitmap_isequal(set, ancestor->cpuset));
      }
      hwloc_bitmap_free(set);
    }
  }

  amdsmi_shut_down();
  return 0;
}
#endif /* HWLOC_RSMI_USE_AMD_SMI */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  hwloc_topology_load(topology);

#ifdef HWLOC_RSMI_USE_ROCM_SMI
  check_rsmi(topology);
#endif /* HWLOC_RSMI_USE_ROCM_SMI */
  printf("\n");
#ifdef HWLOC_RSMI_USE_AMD_SMI
  check_amdsmi(topology);
#endif /* HWLOC_RSMI_USE_AMD_SMI */

  hwloc_topology_destroy(topology);
  return 0;
}
