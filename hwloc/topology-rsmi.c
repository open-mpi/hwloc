/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2012-2026 Inria.  All rights reserved.
 * Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.
 * Written by Advanced Micro Devices,
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "hwloc/plugins.h"

/* private headers allowed for convenience because this plugin is built within hwloc */
#include "private/misc.h"
#include "private/debug.h"

#include "amd_smi/amdsmi.h"

static int
hwloc__rsmi_add_xgmi_bandwidth(hwloc_topology_t topology,
                               unsigned nbobjs, hwloc_obj_t *objs, hwloc_uint64_t *bws)
{
  void *handle;
  int err;

  handle = hwloc_backend_distances_add_create(topology, "XGMIBandwidth",
                                              HWLOC_DISTANCES_KIND_FROM_OS|HWLOC_DISTANCES_KIND_VALUE_BANDWIDTH,
                                              0);
  if (!handle)
    goto out;

  err = hwloc_backend_distances_add_values(topology, handle, nbobjs, objs, bws, 0);
  if (err < 0)
    goto out;
  /* arrays are now attached to the handle */
  objs = NULL;
  bws = NULL;

  err = hwloc_backend_distances_add_commit(topology, handle, 0 /* don't group GPUs */);
  if (err < 0)
    goto out;

  return 0;

 out:
  free(objs);
  free(bws);
  return -1;
}

static int
hwloc__rsmi_add_xgmi_hops(hwloc_topology_t topology,
                          unsigned nbobjs, hwloc_obj_t *objs, hwloc_uint64_t *hops)
{
  void *handle;
  int err;

  handle = hwloc_backend_distances_add_create(topology, "XGMIHops",
                                              HWLOC_DISTANCES_KIND_FROM_OS|HWLOC_DISTANCES_KIND_VALUE_HOPS,
                                              0);
  if (!handle)
    goto out;

  err = hwloc_backend_distances_add_values(topology, handle, nbobjs, objs, hops, 0);
  if (err < 0)
    goto out;
  /* arrays are now attached to the handle */
  objs = NULL;
  hops = NULL;

  err = hwloc_backend_distances_add_commit(topology, handle, 0 /* don't group GPUs */);
  if (err < 0)
    goto out;

  return 0;

 out:
  free(objs);
  free(hops);
  return -1;
}

static int
hwloc_amd_smi_discover(struct hwloc_topology *topology)
{
  amdsmi_status_t ret;
  uint32_t nr_sockets, nr_procs, nr_procs_allocated, i, j;
  amdsmi_socket_handle *sockets = NULL;
  hwloc_obj_t *osdevs = NULL, *osdevs2 = NULL;
  hwloc_uint64_t *xgmi_bws = NULL, *xgmi_hops = NULL;
  amdsmi_processor_handle *procs;
  uint64_t min_xgmi_weight = UINT64_MAX;
  int added = 0;

  ret = amdsmi_init(AMDSMI_INIT_AMD_GPUS);
  if (AMDSMI_STATUS_SUCCESS != ret) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      amdsmi_status_code_to_string(ret, &status_string);
      fprintf(stderr, "hwloc/rsmi: Failed to initialize with amdsmi_init(): %s\n", status_string);
    }
    return 0;
  }

  ret = amdsmi_get_socket_handles(&nr_sockets, NULL);
  if (AMDSMI_STATUS_SUCCESS != ret) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      amdsmi_status_code_to_string(ret, &status_string);
      fprintf(stderr, "hwloc/rsmi: Failed to get number of sockets: %s\n", status_string);
    }
    goto out;
  }

  sockets = malloc(nr_sockets * sizeof(*sockets));
  if (!sockets)
    goto out;

  ret = amdsmi_get_socket_handles(&nr_sockets, sockets);
  if (AMDSMI_STATUS_SUCCESS != ret)
    goto out;

  /* build an array of all GPU proc handles */
  nr_procs_allocated = 64; /* in case we have many partitioned GPUs */
  procs = malloc(nr_procs_allocated * sizeof(*procs));
  if (!procs) {
    free(sockets);
    goto out;
  }

  nr_procs = 0;
  for(i=0; i<nr_sockets; i++) {
    /* how many GPUs in this socket */
    ret = amdsmi_get_processor_handles(sockets[i], &j, NULL);
    if (AMDSMI_STATUS_SUCCESS != ret)
      continue;

    /* realloc the array if needed */
    if (nr_procs + j > nr_procs_allocated) {
      amdsmi_processor_handle *new = realloc(procs, nr_procs_allocated * 2 * sizeof(*new));
      if (!new)
        /* stop getting proc handles */
        break;
      procs = new;
      nr_procs_allocated *= 2;
    }

    /* get those new handles */
    j = nr_procs_allocated - nr_procs;
    ret = amdsmi_get_processor_handles(sockets[i], &j, &procs[nr_procs]);
    if (AMDSMI_STATUS_SUCCESS == ret)
      nr_procs += j;
  }

  /* no need for sockets anymore */
  free(sockets);

  /* allocate osdev arrays for matrices */
  osdevs = malloc(nr_procs * sizeof(*osdevs));
  xgmi_bws = calloc(nr_procs*nr_procs, sizeof(*xgmi_bws));
  osdevs2 = malloc(nr_procs *sizeof(*osdevs2));
  xgmi_hops = calloc(nr_procs*nr_procs, sizeof(*xgmi_hops));
  if (!osdevs || !xgmi_bws || !osdevs2 || !xgmi_hops) {
    free(procs);
    goto out;
  }

  /* now iterate over proc handles */
  for(i=0; i<nr_procs; i++) {
    hwloc_obj_t osdev, parent;
    char buffer[64];
    amdsmi_enumeration_info_t enum_info;
    amdsmi_board_info_t board_info;
    unsigned uuid_length = AMDSMI_GPU_UUID_SIZE;
    char uuid[AMDSMI_GPU_UUID_SIZE];
    uint64_t total_memory;
    amdsmi_xgmi_info_t xgmi_info;
    uint64_t bdfid;

    /* get the rsmi-like index */
    ret = amdsmi_get_gpu_enumeration_info(procs[i], &enum_info);
    if (AMDSMI_STATUS_SUCCESS != ret) {
      if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
        const char *status_string;
        amdsmi_status_code_to_string(ret, &status_string);
        fprintf(stderr, "hwloc/rsmi: failed to get AMD SMI enumeration index: %s\n",
                status_string);
      }
      continue;
    }

    osdev = hwloc_alloc_setup_object(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_UNKNOWN_INDEX);
    if (!osdev)
      continue;
    snprintf(buffer, sizeof(buffer), "rsmi%u", enum_info.hip_id);
    osdev->name = strdup(buffer);
    osdev->subtype = strdup("RSMI");
    osdev->depth = HWLOC_TYPE_DEPTH_UNKNOWN;
    osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_GPU | HWLOC_OBJ_OSDEV_COPROC;

    /* only AMD GPUs are enable in amdsmi_init() */
    hwloc_obj_add_info(osdev, "GPUVendor", "AMD");

    ret = amdsmi_get_gpu_board_info(procs[i], &board_info);
    if (AMDSMI_STATUS_SUCCESS == ret && board_info.product_name[0] != '\0') {
      hwloc_obj_add_info(osdev, "GPUModel", board_info.product_name);
    }
    if (AMDSMI_STATUS_SUCCESS == ret && board_info.product_serial[0] != '\0') {
      hwloc_obj_add_info(osdev, "AMDSerial", board_info.product_serial);
    }

    ret = amdsmi_get_gpu_device_uuid(procs[i], &uuid_length, (char*) &uuid);
    if (AMDSMI_STATUS_SUCCESS == ret && uuid[0] != '\0') {
      hwloc_obj_add_info(osdev, "AMDUUID", uuid);
    }

    ret = amdsmi_get_xgmi_info(procs[i], &xgmi_info);
    if (AMDSMI_STATUS_SUCCESS == ret) {
      snprintf(buffer, sizeof(buffer), "%llx", (unsigned long long)xgmi_info.xgmi_hive_id);
      hwloc_obj_add_info(osdev, "XGMIHiveID", buffer);
    }

    ret = amdsmi_get_gpu_memory_total(procs[i], AMDSMI_MEM_TYPE_VRAM, &total_memory);
    if (AMDSMI_STATUS_SUCCESS == ret) {
      snprintf(buffer, sizeof(buffer), "%lluKiB", (unsigned long long) total_memory/1024);
      hwloc_obj_add_info(osdev, "RSMIVRAMSize", buffer);
    }
    ret = amdsmi_get_gpu_memory_total(procs[i], AMDSMI_MEM_TYPE_VIS_VRAM, &total_memory);
    if (AMDSMI_STATUS_SUCCESS == ret) {
      snprintf(buffer, sizeof(buffer), "%lluKiB", (unsigned long long) total_memory/1024);
      hwloc_obj_add_info(osdev, "RSMIVisibleVRAMSize", buffer);
    }
    ret = amdsmi_get_gpu_memory_total(procs[i], AMDSMI_MEM_TYPE_GTT, &total_memory);
    if (AMDSMI_STATUS_SUCCESS == ret) {
      snprintf(buffer, sizeof(buffer), "%lluKiB", (unsigned long long) total_memory/1024);
      hwloc_obj_add_info(osdev, "RSMIGTTSize", buffer);
    }

    parent = NULL;
    /* don't use amdsmi_get_gpu_device_bdf() because we want the partition ID too */
    ret = amdsmi_get_gpu_bdf_id(procs[i], &bdfid);
    if (AMDSMI_STATUS_SUCCESS == ret) {
      unsigned domain = (bdfid & 0xffffffff00000000ULL) >> 32;
      /*  unsigned partid = (bdfid & 0xf0000000) >> 28; unused for now */
      unsigned bus = (bdfid & 0xff00) >> 8;
      unsigned device = (bdfid & 0xf8) >> 3;
      unsigned func = (bdfid & 0x7);

      parent = hwloc_pci_get_parent_by_busid(topology, domain, bus, device, func);
      if ((!parent || parent->type != HWLOC_OBJ_PCI_DEVICE) && func > 0) {
        /* Partitioned MI devices may return the partition ID in a fake BDF func,
         * hence we would fail to find a pcidev parent.
         * Try with func=0 instead.
         * We could first check whether amdsmi_get_gpu_kfd_info().current_partition_id == func,
         * but we wouldn't have anything better to do to get the likely parent anyway.
         */
        parent = hwloc_pci_get_parent_by_busid(topology, domain, bus, device, 0);
      }
    }
    if (parent && parent->type == HWLOC_OBJ_PCI_DEVICE) {
      amdsmi_pcie_info_t pcieinfo;
      ret = amdsmi_get_pcie_info(procs[i], &pcieinfo);
      if (AMDSMI_STATUS_SUCCESS == ret
          && pcieinfo.pcie_metric.pcie_width != (uint16_t) -1
          && pcieinfo.pcie_metric.pcie_speed != (uint32_t) -1) {
        /* pcieinfo.pcie_metric.pcie_bandwidth is also -1 for now, see https://github.com/ROCm/amdsmi/issues/193 .
         * We can't use hwloc__pci_link_speed() in a plugin, and we have GT/s instead of the current revision anyway
         */
       float lanespeed = (float) pcieinfo.pcie_metric.pcie_speed; /* MT/s = MB/s */
        if (lanespeed > 50000)
          lanespeed *= 242./256.; /* Gen6+ encoding */
        else
          lanespeed *= 128./130.; /* Gen5- encoding */
        parent->attr->pcidev.linkspeed = lanespeed * pcieinfo.pcie_metric.pcie_width / 8 / 1024; /* Mb/s to GB/s */
      }
    }
    if (!parent)
      parent = hwloc_get_root_obj(topology);
    hwloc_insert_object_by_parent(topology, parent, osdev);

    osdevs[added] = osdevs2[added] = osdev;
    added++;
  }

  if (!(hwloc_topology_get_flags(topology) & HWLOC_TOPOLOGY_FLAG_NO_DISTANCES)) {
    /* iterate again for distances now that we have all osdev names */
    int got_xgmi_bws = 0;
    for(i=0; i<nr_procs; i++) {
      amdsmi_vram_info_t meminfo;

      /* use memory bandwidth for local bandwidth */
      if (AMDSMI_STATUS_SUCCESS == amdsmi_get_gpu_vram_info(procs[i], &meminfo)
          && meminfo.vram_max_bandwidth != (uint64_t) -1)
        xgmi_bws[i*nr_procs+i] = meminfo.vram_max_bandwidth * 1000;
      else
        /* Old hardware (e.g. MI250X) may report -1.
         * XGMI links are 100GB/s, so use 1TB/s for local, it somehow matches the HBM.
         */
        xgmi_bws[i*nr_procs+i] = 1000000;

      /* now look at actual other peers */
        for (j=0; j<nr_procs; j++) {
#if AMDSMI_LIB_VERSION_MAJOR >= 26
          /* ROCm 7 switched from io_link to link type. But now have XGMI = 2
           * link_type existed earlier but enum values were different.
           * iolink_type doesn't exist anymore.
           */
          amdsmi_link_type_t type;
#define HWLOC_AMDSMI_LINK_TYPE_XGMI AMDSMI_LINK_TYPE_XGMI
#else
          amdsmi_io_link_type_t type;
#define HWLOC_AMDSMI_LINK_TYPE_XGMI AMDSMI_IOLINK_TYPE_XGMI
#endif
          uint64_t hops;
          if (i == j)
            continue;
          if (AMDSMI_STATUS_SUCCESS == amdsmi_topo_get_link_type(procs[i], procs[j], &hops, &type) &&
              hops == 1 && type == HWLOC_AMDSMI_LINK_TYPE_XGMI) {
            /* hops==1 doesn't mean direct XGMI link but rather XGMI-only path between GPUs
             * (contrary to PCIe or inter-socket paths).
             * The length of XGMI path is rather given as the weight, but it needs to normalized.
             */
            uint64_t min, max, weight;
            if (AMDSMI_STATUS_SUCCESS == amdsmi_get_minmax_bandwidth_between_processors(procs[i], procs[j], &min, &max)) {
              xgmi_bws[i*nr_procs+j] = max; /* min is usually 50k while max is 50/100/200k */
              got_xgmi_bws = 1;
            }
            if (AMDSMI_STATUS_SUCCESS == amdsmi_topo_get_link_weight(procs[i], procs[j], &weight)) {
              xgmi_hops[i*nr_procs+j] = weight;
              if (weight < min_xgmi_weight)
                /* save the smallest weight to normalize them later */
                min_xgmi_weight = weight;
            }
          }
      }
    }
    if (got_xgmi_bws) {
      hwloc__rsmi_add_xgmi_bandwidth(topology, nr_procs, osdevs, xgmi_bws);
      osdevs = NULL;
      xgmi_bws = NULL;
    }
    if (min_xgmi_weight < UINT64_MAX) {
      /* Normalize XGMI weights since the amdkfd kernel driver gives 15*hops.
       * In case some values are ever between 16 and 29, round-up above so that they become 2 instead of 1.
       */
      for(i=0; i<nr_procs*nr_procs; i++)
        xgmi_hops[i] = (xgmi_hops[i]+min_xgmi_weight-1)/min_xgmi_weight;
      hwloc__rsmi_add_xgmi_hops(topology, nr_procs, osdevs2, xgmi_hops);
      /* don't free arrays, they were given to the distance internals */
      osdevs2 = NULL;
      xgmi_hops = NULL;
    }
  }

  free(procs);

 out:
  free(osdevs);
  free(xgmi_bws);
  free(osdevs2);
  free(xgmi_hops);

  if (added)
    hwloc_modify_infos(hwloc_topology_get_infos(topology), HWLOC_MODIFY_INFOS_OP_ADD, "Backend", "RSMI");
  amdsmi_shut_down();
  return 0;
}

static int
hwloc_rsmi_discover(struct hwloc_backend *backend, struct hwloc_disc_status *dstatus)
{
  /*
   * This backend uses the underlying OS.
   * However we don't enforce topology->is_thissystem so that
   * we may still force use this backend when debugging with !thissystem.
   */

  struct hwloc_topology *topology = backend->topology;
  enum hwloc_type_filter_e filter;

  assert(dstatus->phase == HWLOC_DISC_PHASE_IO);

  hwloc_topology_get_type_filter(topology, HWLOC_OBJ_OS_DEVICE, &filter);
  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
    return 0;

  return hwloc_amd_smi_discover(topology);
}

static struct hwloc_backend *
hwloc_rsmi_component_instantiate(struct hwloc_topology *topology,
				 struct hwloc_disc_component *component,
				 unsigned excluded_phases __hwloc_attribute_unused,
				 const void *_data1 __hwloc_attribute_unused,
				 const void *_data2 __hwloc_attribute_unused,
				 const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  backend = hwloc_backend_alloc(topology, component, 0);
  if (!backend)
    return NULL;
  backend->discover = hwloc_rsmi_discover;
  return backend;
}

static struct hwloc_disc_component hwloc_rsmi_disc_component = {
  "rsmi",
  HWLOC_DISC_PHASE_IO,
  HWLOC_DISC_PHASE_GLOBAL,
  hwloc_rsmi_component_instantiate,
  10, /* after pci */
  1,
  NULL
};

static int
hwloc_rsmi_component_init(unsigned long flags)
{
  if (flags)
    return -1;
  if (hwloc_plugin_check_namespace("rsmi", "hwloc_backend_alloc") < 0)
    return -1;
  return 0;
}

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_rsmi_component;
#endif

const struct hwloc_component hwloc_rsmi_component = {
  HWLOC_COMPONENT_ABI,
  hwloc_rsmi_component_init, NULL,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_rsmi_disc_component
};
