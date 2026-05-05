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

#ifdef HWLOC_RSMI_USE_ROCM_SMI
/**************************
 * ROCm SMI implementation
 */

#include <rocm_smi/rocm_smi.h>

/*
 * Get the name of the GPU
 *
 * dv_ind		(IN) The device index
 * device_name	(OUT) Name of GPU devices
 * size			(OUT) Size of the name
 */
static int get_device_name(uint32_t dv_ind, char *device_name, unsigned int size)
{
  rsmi_status_t rsmi_rc = rsmi_dev_name_get(dv_ind, device_name, size);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_rc = rsmi_status_string(rsmi_rc, &status_string);
      fprintf(stderr, "hwloc/rsmi: GPU(%u): Failed to get name: %s\n", (unsigned)dv_ind, status_string);
    }
    return -1;
  }
  return 0;
}

/*
 * Get the PCI Info of the GPU
 *
 * dv_ind  (IN) The device index
 * bdfid   (OUT) PCI Info of GPU devices
 */
static int get_device_pci_info(uint32_t dv_ind, uint64_t *bdfid)
{
  rsmi_status_t rsmi_rc = rsmi_dev_pci_id_get(dv_ind, bdfid);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_rc = rsmi_status_string(rsmi_rc, &status_string);
      fprintf(stderr, "hwloc/rsmi: GPU(%u): Failed to get PCI Info: %s\n", (unsigned)dv_ind, status_string);
    }
    return -1;
  }
  return 0;
}

/*
 * Get the partition ID of the GPU
 *
 * dv_ind  (IN) The device index
 * partid   (OUT) partition ID of GPU devices
 */
static int get_device_partition_id(uint32_t dv_ind __hwloc_attribute_unused, uint32_t *partid __hwloc_attribute_unused)
{
#if HAVE_DECL_RSMI_DEV_PARTITION_ID_GET
  rsmi_status_t rsmi_rc = rsmi_dev_partition_id_get(dv_ind, partid);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_rc = rsmi_status_string(rsmi_rc, &status_string);
      fprintf(stderr, "hwloc/rsmi: GPU(%u): Failed to get partition ID: %s\n", (unsigned)dv_ind, status_string);
    }
    return -1;
  }
  return 0;
#else
  errno = ENOSYS;
  return -1;
#endif
}

/*
 * Get the PCI link speed of the GPU
 *
 * dv_ind    (IN) The device index
 * linkspeed (OUT) PCI link speed of GPU devices
 */
static int get_device_pci_linkspeed(uint32_t dv_ind, float *linkspeed)
{
  rsmi_pcie_bandwidth_t bandwidth;
  uint64_t lanespeed_raw; // T/s
  uint64_t lanespeed; // (bits/s)
  uint32_t lanes;
  rsmi_status_t rsmi_rc = rsmi_dev_pci_bandwidth_get(dv_ind, &bandwidth);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    return -1;
  }
  lanespeed_raw = bandwidth.transfer_rate.frequency[bandwidth.transfer_rate.current]; // T/s
  lanespeed = lanespeed_raw <= 5000000000 ? (lanespeed_raw * 8)/10 : (lanespeed_raw * 128)/130; // bits/s
  lanes = bandwidth.lanes[bandwidth.transfer_rate.current];
  *linkspeed = (lanespeed * lanes) / 8000000000; // GB/s
  return 0;
}

/*
 * Get the Unique ID of the GPU
 *
 * dv_ind  (IN) The device index
 * buffer  (OUT) Unique ID of GPU devices
 */
static int get_device_unique_id(uint32_t dv_ind, char *buffer)
{
  uint64_t id;
  rsmi_status_t rsmi_rc = rsmi_dev_unique_id_get(dv_ind, &id);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    return -1;
  }
  sprintf(buffer, "%llx", (unsigned long long) id);
  return 0;
}

/*
 * Get the serial number of the GPU
 *
 * dv_ind  (IN) The device index
 * serial  (OUT) Serial number of GPU devices
 * size    (IN) Length of the caller provided buffer
 */
static int get_device_serial_number(uint32_t dv_ind, char *serial, unsigned int size)
{
  rsmi_status_t rsmi_rc = rsmi_dev_serial_number_get(dv_ind, serial, size);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    return -1;
  }
  return 0;
}

/*
 * Get the XGMI hive id of the GPU
 *
 * dv_ind  (IN) The device index
 * hive_id (OUT) The XGMI hive id of GPU devices
 */
static int get_device_xgmi_hive_id(uint32_t dv_ind, char *buffer)
{
  uint64_t hive_id;
  rsmi_status_t rsmi_rc = rsmi_dev_xgmi_hive_id_get(dv_ind, &hive_id);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_rc = rsmi_status_string(rsmi_rc, &status_string);
      fprintf(stderr, "hwloc/rsmi: GPU(%u): Failed to get hive id: %s\n", (unsigned)dv_ind, status_string);
    }
    return -1;
  }
  sprintf(buffer, "%llx", (unsigned long long) hive_id);
  return 0;
}

/*
 * Get the IO Link type of the GPU
 *
 * dv_ind_src  (IN)  The source device index
 * dv_ind_dst  (IN)  The destination device index
 * type        (OUT) The type of IO Link
 */
static int get_device_io_link_type(uint32_t dv_ind_src, uint32_t dv_ind_dst,
                                   RSMI_IO_LINK_TYPE *type, uint64_t *hops)
{
  rsmi_status_t rsmi_rc = rsmi_topo_get_link_type(dv_ind_src, dv_ind_dst,
                                                  hops, type);

  if (rsmi_rc != RSMI_STATUS_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_rc = rsmi_status_string(rsmi_rc, &status_string);
      fprintf(stderr, "hwloc/rsmi: GPU(%u): Failed to get link type: %s\n", (unsigned)dv_ind_src, status_string);
    }
    return -1;
  }
  return 0;
}

static int
hwloc_rocm_smi_discover(struct hwloc_topology *topology)
{
  hwloc_obj_t *osdevs = NULL, *osdevs2 = NULL;
  hwloc_uint64_t *xgmi_bws = NULL, *xgmi_hops = NULL;
  uint64_t memory;
  int got_xgmi_bws = 0;
  rsmi_version_t version;
  rsmi_status_t ret;
  int may_shutdown;
  unsigned nb, i, j, added = 0;

  ret = rsmi_init(0);
  if (RSMI_STATUS_SUCCESS != ret) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_status_string(ret, &status_string);
      fprintf(stderr, "hwloc/rsmi: Failed to initialize with rsmi_init(): %s\n", status_string);
    }
    return 0;
  }

  rsmi_version_get(&version);

  ret = rsmi_num_monitor_devices(&nb);
  if (RSMI_STATUS_SUCCESS != ret || !nb) {
    if (RSMI_STATUS_SUCCESS != ret && HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
      const char *status_string;
      rsmi_status_string(ret, &status_string);
      fprintf(stderr, "hwloc/rsmi: Failed to get number of devices with rsmi_num_monitor_devices(): %s\n", status_string);
    }
    goto out;
  }

  osdevs = malloc(nb *sizeof(*osdevs));
  xgmi_bws = calloc(nb*nb, sizeof(*xgmi_bws));
  osdevs2 = malloc(nb *sizeof(*osdevs2));
  xgmi_hops = calloc(nb*nb, sizeof(*xgmi_hops));
  if (!osdevs || !xgmi_bws || !osdevs2 || !xgmi_hops)
    goto out;

  for (i=0; i<nb; i++) {
    uint64_t bdfid = 0;
    hwloc_obj_t osdev, parent;
    char buffer[64];
    char *xgmi_peers, *xgmi_peers_ptr;

    osdev = hwloc_alloc_setup_object(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_UNKNOWN_INDEX);
    snprintf(buffer, sizeof(buffer), "rsmi%u", i);
    osdev->name = strdup(buffer);
    osdev->subtype = strdup("RSMI");
    osdev->depth = HWLOC_TYPE_DEPTH_UNKNOWN;
    osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_GPU | HWLOC_OBJ_OSDEV_COPROC;

    hwloc_obj_add_info(osdev, "GPUVendor", "AMD");

    buffer[0] = '\0';
    if (get_device_name(i, buffer, sizeof(buffer)) == -1)
      buffer[0] = '\0';
    hwloc_obj_add_info(osdev, "GPUModel", buffer);

    buffer[0] = '\0';
    if ((get_device_serial_number(i, buffer, sizeof(buffer)) == 0) && buffer[0])
      hwloc_obj_add_info(osdev, "AMDSerial", buffer);

    buffer[0] = '\0';
    if (get_device_unique_id(i, buffer) == 0)
      hwloc_obj_add_info(osdev, "AMDUUID", buffer);

    buffer[0] = '\0';
    if (get_device_xgmi_hive_id(i, buffer) == 0)
      hwloc_obj_add_info(osdev, "XGMIHiveID", buffer);

    ret = rsmi_dev_memory_total_get(i, RSMI_MEM_TYPE_VRAM, &memory);
    if (ret == RSMI_STATUS_SUCCESS) {
      char tmp[64];
      snprintf(tmp, sizeof(tmp), "%lluKiB", (unsigned long long)memory/1024);
      hwloc_obj_add_info(osdev, "RSMIVRAMSize", tmp);
    }
    ret = rsmi_dev_memory_total_get(i, RSMI_MEM_TYPE_VIS_VRAM, &memory);
    if (ret == RSMI_STATUS_SUCCESS) {
      char tmp[64];
      snprintf(tmp, sizeof(tmp), "%lluKiB", (unsigned long long)memory/1024);
      hwloc_obj_add_info(osdev, "RSMIVisibleVRAMSize", tmp);
    }
    ret = rsmi_dev_memory_total_get(i, RSMI_MEM_TYPE_GTT, &memory);
    if (ret == RSMI_STATUS_SUCCESS) {
      char tmp[64];
      snprintf(tmp, sizeof(tmp), "%lluKiB", (unsigned long long)memory/1024);
      hwloc_obj_add_info(osdev, "RSMIGTTSize", tmp);
    }
    /* there's also rsmi_dev_memory_usage_get() to get what's currently used in these memories */

    xgmi_peers = malloc(nb*15+1);  /* "rsmi" + unsigned int + space = 15 chars max, + ending \0 */
    if (xgmi_peers) {
      xgmi_peers[0] = '\0';
      xgmi_peers_ptr = xgmi_peers;
      for (j=0; j<nb; j++) {
        RSMI_IO_LINK_TYPE type;
        uint64_t hops;
        if (i == j)
          continue;
        if ((get_device_io_link_type(i, j, &type, &hops) == 0) &&
            (type == RSMI_IOLINK_TYPE_XGMI)) {
          xgmi_peers_ptr += sprintf(xgmi_peers_ptr, "rsmi%u ", j);
          xgmi_bws[i*nb+j] = 100000; /* TODO: verify the XGMI version before putting 100GB/s here? */
          xgmi_hops[i*nb+j] = hops;
          got_xgmi_bws = 1;
        }
      }
      if (xgmi_peers[0] != '\0')
        hwloc_obj_add_info(osdev, "XGMIPeers", xgmi_peers);
      free(xgmi_peers);
    }

    parent = NULL;
    if (get_device_pci_info(i, &bdfid) == 0) {
      unsigned domain, device, bus, func;
      domain = (bdfid>>32) & 0xffffffff;
      bus = ((bdfid & 0xffff)>>8) & 0xff;
      device = ((bdfid & 0xff)>>3) & 0x1f;
      func = bdfid & 0x7;
      parent = hwloc_pci_get_parent_by_busid(topology, domain, bus, device, func);
      if ((!parent || parent->type != HWLOC_OBJ_PCI_DEVICE) && func > 0) {
        /* Partitioned MI devices may return the partition ID in a fake BDF func,
         * hence we would fail to find a pcidev parent.
         * Try with func=0 instead.
         */
        uint32_t partid = 0;
        if (get_device_partition_id(i, &partid) < 0) {
          static int warned = 0;
          if ((!warned && HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_CRITICAL)) || HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_RSMI)) {
            fprintf(stderr, "hwloc/rsmi: Failed to retrieve partition ID, RSMI device locality will likely be wrong\n");
            warned = 1;
          }
        } else {
          if (func == partid)
            parent = hwloc_pci_get_parent_by_busid(topology, domain, bus, device, 0);
        }
      }
      if (parent && parent->type == HWLOC_OBJ_PCI_DEVICE)
        get_device_pci_linkspeed(i, &parent->attr->pcidev.linkspeed);
      if (!parent)
        parent = hwloc_get_root_obj(topology);
    }

    hwloc_insert_object_by_parent(topology, parent, osdev);
    osdevs[i] = osdevs2[i] = osdev;
    added++;
  }

  if (hwloc_topology_get_flags(topology) & HWLOC_TOPOLOGY_FLAG_NO_DISTANCES)
    got_xgmi_bws = 0;

  if (got_xgmi_bws) {
    /* add very high artifical values on the diagonal since local is faster than remote.
     * XGMI links are 100GB/s, so use 1TB/s for local, it somehow matches the HBM.
     */
    for(i=0; i<nb; i++)
      xgmi_bws[i*nb+i] = 1000000;

    hwloc__rsmi_add_xgmi_bandwidth(topology, nb, osdevs, xgmi_bws);

    hwloc__rsmi_add_xgmi_hops(topology, nb, osdevs2, xgmi_hops);

    /* don't free arrays, they were given to the distance internals */
    osdevs = NULL;
    xgmi_bws = NULL;
    osdevs2 = NULL;
    xgmi_hops = NULL;
  }

 out:
  may_shutdown = 0;
  if (version.major > 3 || (version.major == 3 && version.minor > 3)) {
    may_shutdown = 1;
  } else {
    /* old RSMI libs didn't have refcounting, we don't want to shutdown what the app may be using.
     * don't shutdown unless required for valgrind testing etc.
     */
    char *env = getenv("HWLOC_RSMI_SHUTDOWN");
    if (env)
      may_shutdown = 1;
  }
  if (may_shutdown)
    rsmi_shut_down();

  free(osdevs);
  free(xgmi_bws);
  free(osdevs2);
  free(xgmi_hops);

  if (added)
    hwloc_modify_infos(hwloc_topology_get_infos(topology), HWLOC_MODIFY_INFOS_OP_ADD, "Backend", "RSMI");
  return 0;
}

#endif /* HWLOC_RSMI_USE_ROCM_SMI */

#ifdef HWLOC_RSMI_USE_AMD_SMI
/**************************
 * AMD SMI implementation
 */
#include "amd_smi/amdsmi.h"

static int
hwloc_amd_smi_discover(struct hwloc_topology *topology)
{
  amdsmi_status_t ret;
  uint32_t nr_sockets, nr_procs, nr_procs_allocated, i, j;
  amdsmi_socket_handle *sockets = NULL;
  hwloc_obj_t *osdevs = NULL, *osdevs2 = NULL;
  hwloc_uint64_t *xgmi_bws = NULL, *xgmi_hops = NULL;
  amdsmi_processor_handle *procs;
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
         */
        /* TODO check func == partid returned by another func? */
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
      char *xgmi_peers, *xgmi_peers_ptr;

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
      xgmi_peers = malloc(nr_procs*15+1);  /* "rsmi" + unsigned int + space = 15 chars max, + ending \0 */
      if (xgmi_peers) {
        xgmi_peers[0] = '\0';
        xgmi_peers_ptr = xgmi_peers;
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
              type == HWLOC_AMDSMI_LINK_TYPE_XGMI) {
            xgmi_peers_ptr += sprintf(xgmi_peers_ptr, "%s ", osdevs[j]->name);
            xgmi_hops[i*nr_procs+j] = hops;
            /* FIXME: on Adastra/Frontier, hops is always 1 even for non-direct links (and bandwidth is 0 below).
             * Change those hops to 2 ?
             * See https://github.com/ROCm/rocm-systems/issues/5763
             */
            xgmi_bws[i*nr_procs+j] = 0;
            if (1 == hops) {
              uint64_t min, max;
              if (AMDSMI_STATUS_SUCCESS == amdsmi_get_minmax_bandwidth_between_processors(procs[i], procs[j], &min, &max)) {
                xgmi_bws[i*nr_procs+j] = max; /* min is usually 50k while max is 50/100/200k */
                got_xgmi_bws = 1;
              }
            }
          }
        }
        if (xgmi_peers[0] != '\0')
          hwloc_obj_add_info(osdevs[i], "XGMIPeers", xgmi_peers);
        free(xgmi_peers);
      }
    }
    if (got_xgmi_bws) {
      hwloc__rsmi_add_xgmi_bandwidth(topology, nr_procs, osdevs, xgmi_bws);
      hwloc__rsmi_add_xgmi_hops(topology, nr_procs, osdevs2, xgmi_hops);
      /* don't free arrays, they were given to the distance internals */
      osdevs = NULL;
      xgmi_bws = NULL;
      osdevs2 = NULL;
      xgmi_hops = NULL;
    }
  }

  free(procs);
  free(osdevs);
  free(xgmi_bws);
  free(osdevs2);
  free(xgmi_hops);

 out:
  if (added)
    hwloc_modify_infos(hwloc_topology_get_infos(topology), HWLOC_MODIFY_INFOS_OP_ADD, "Backend", "RSMI");
  amdsmi_shut_down();
  return 0;
}

#endif /* HWLOC_RSMI_USE_AMD_SMI */

static int
hwloc_rsmi_discover(struct hwloc_backend *backend, struct hwloc_disc_status *dstatus)
{
  /*
   * This backend uses the underlying OS.
   * However we don't enforce topology->is_thissystem so that
   * we may still force use this backend when debugging with !thissystem.
   */

  int ret = 0;
  const char *env = getenv("HWLOC_RSMI_BACKEND");
  struct hwloc_topology *topology = backend->topology;
  enum hwloc_type_filter_e filter;
  enum { HWLOC_RSMI_BACKEND_FIRST, HWLOC_RSMI_BACKEND_AMD, HWLOC_RSMI_BACKEND_ROCM, HWLOC_RSMI_BACKEND_BOTH } try = HWLOC_RSMI_BACKEND_FIRST;

  assert(dstatus->phase == HWLOC_DISC_PHASE_IO);

  hwloc_topology_get_type_filter(topology, HWLOC_OBJ_OS_DEVICE, &filter);
  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
    return 0;

  if (env) {
    if (!strcmp(env, "amd"))
      try = HWLOC_RSMI_BACKEND_AMD;
    else if (!strcmp(env, "rocm"))
      try = HWLOC_RSMI_BACKEND_ROCM;
    else if (!strcmp(env, "both"))
      try = HWLOC_RSMI_BACKEND_BOTH;
  }

  switch (try) {
  case HWLOC_RSMI_BACKEND_FIRST:
    /* use AMD first if available, ROCm otherwise */
#if (defined HWLOC_RSMI_USE_AMD_SMI)
    ret = hwloc_amd_smi_discover(topology);
#elif (defined HWLOC_RSMI_USE_ROCM_SMI)
    ret = hwloc_rocm_smi_discover(topology);
#else
    assert(0); /* not possible at configure */
#endif
    break;
  case HWLOC_RSMI_BACKEND_AMD:
    /* only use AMD */
#ifdef HWLOC_RSMI_USE_AMD_SMI
    ret = hwloc_amd_smi_discover(topology);
#else
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_USER))
      fprintf(stderr, "hwloc/rsmi: cannot enable AMD SMI in RSMI backend, not available\n");
#endif
    break;
  case HWLOC_RSMI_BACKEND_ROCM:
    /* only use ROCm */
#ifdef HWLOC_RSMI_USE_ROCM_SMI
    ret = hwloc_rocm_smi_discover(topology);
#else
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_USER))
      fprintf(stderr, "hwloc/rsmi: cannot enable ROCm SMI in RSMI backend, not available\n");
#endif
    break;
  case HWLOC_RSMI_BACKEND_BOTH:
    /* use both for debuggin/comparing the outputs */
#ifdef HWLOC_RSMI_USE_AMD_SMI
    ret = hwloc_amd_smi_discover(topology);
#endif /* HWLOC_RSMI_USE_AMD_SMI */
#ifdef HWLOC_RSMI_USE_ROCM_SMI
    ret = hwloc_rocm_smi_discover(topology);
#endif /* HWLOC_RSMI_USE_ROCM_SMI */
    break;
  }
  return ret;
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
