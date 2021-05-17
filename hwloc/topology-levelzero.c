/*
 * Copyright © 2020-2021 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "hwloc/plugins.h"

/* private headers allowed for convenience because this plugin is built within hwloc */
#include "private/misc.h"
#include "private/debug.h"

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

static int
hwloc_levelzero_discover(struct hwloc_backend *backend, struct hwloc_disc_status *dstatus)
{
  /*
   * This backend uses the underlying OS.
   * However we don't enforce topology->is_thissystem so that
   * we may still force use this backend when debugging with !thissystem.
   */

  struct hwloc_topology *topology = backend->topology;
  enum hwloc_type_filter_e filter;
  ze_result_t res;
  ze_driver_handle_t *drh;
  uint32_t nbdrivers, i, k, zeidx;
  int sysman_maybe_missing = 0; /* 1 if ZES_ENABLE_SYSMAN=1 was NOT set early, 2 if ZES_ENABLE_SYSMAN=0 */
  char *env;

  assert(dstatus->phase == HWLOC_DISC_PHASE_IO);

  hwloc_topology_get_type_filter(topology, HWLOC_OBJ_OS_DEVICE, &filter);
  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
    return 0;

  /* Tell L0 to create sysman devices.
   * If somebody already initialized L0 without Sysman, zesDeviceGetProperties() will fail below.
   * The lib constructor and Windows DllMain tried to set ZES_ENABLE_SYSMAN=1 early (see topology.c),
   * we try again in case they didn't.
   */
  env = getenv("ZES_ENABLE_SYSMAN");
  if (!env) {
    putenv((char *) "ZES_ENABLE_SYSMAN=1");
    /* we'll warn below if we fail to get zes devices */
    sysman_maybe_missing = 1;
  } else if (!atoi(env)) {
    sysman_maybe_missing = 2;
  }

  res = zeInit(0);
  if (res != ZE_RESULT_SUCCESS) {
    if (!hwloc_hide_errors()) {
      fprintf(stderr, "Failed to initialize LevelZero in ze_init(): %d\n", (int)res);
    }
    return 0;
  }

  nbdrivers = 0;
  res = zeDriverGet(&nbdrivers, NULL);
  if (res != ZE_RESULT_SUCCESS || !nbdrivers)
    return 0;
  drh = malloc(nbdrivers * sizeof(*drh));
  if (!drh)
    return 0;
  res = zeDriverGet(&nbdrivers, drh);
  if (res != ZE_RESULT_SUCCESS) {
    free(drh);
    return 0;
  }

  zeidx = 0;
  for(i=0; i<nbdrivers; i++) {
    uint32_t nbdevices, j;
    ze_device_handle_t *dvh;
    char buffer[13];

    nbdevices = 0;
    res = zeDeviceGet(drh[i], &nbdevices, NULL);
    if (res != ZE_RESULT_SUCCESS || !nbdevices)
      continue;
    dvh = malloc(nbdevices * sizeof(*dvh));
    if (!dvh)
      continue;
    res = zeDeviceGet(drh[i], &nbdevices, dvh);
    if (res != ZE_RESULT_SUCCESS) {
      free(dvh);
      continue;
    }

    for(j=0; j<nbdevices; j++) {
      zes_device_properties_t prop;
      unsigned nr_cqprops;
      zes_pci_properties_t pci;
      zes_device_handle_t sdvh = dvh[j];
      hwloc_obj_t osdev, parent;

      res = zesDeviceGetProperties(sdvh, &prop);
      if (res != ZE_RESULT_SUCCESS) {
        /* L0 was likely initialized without sysman, don't bother */
        if (sysman_maybe_missing == 1 && !hwloc_hide_errors())
          fprintf(stderr, "hwloc/levelzero: zesDeviceGetProperties() failed (ZES_ENABLE_SYSMAN=1 set too late?).\n");
        else if (sysman_maybe_missing == 2 && !hwloc_hide_errors())
          fprintf(stderr, "hwloc/levelzero: zesDeviceGetProperties() failed (ZES_ENABLE_SYSMAN=0).\n");
        continue;
      }

      osdev = hwloc_alloc_setup_object(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_UNKNOWN_INDEX);
      snprintf(buffer, sizeof(buffer), "ze%u", zeidx); // ze0d0 ?
      osdev->name = strdup(buffer);
      osdev->depth = HWLOC_TYPE_DEPTH_UNKNOWN;
      osdev->attr->osdev.type = HWLOC_OBJ_OSDEV_COPROC;
      osdev->subtype = strdup("LevelZero");
      hwloc_obj_add_info(osdev, "Backend", "LevelZero");

      snprintf(buffer, sizeof(buffer), "%u", i);
      hwloc_obj_add_info(osdev, "LevelZeroDriverIndex", buffer);
      snprintf(buffer, sizeof(buffer), "%u", j);
      hwloc_obj_add_info(osdev, "LevelZeroDriverDeviceIndex", buffer);

      /* these strings aren't useful with current implementations:
       * prop.vendorName is "Unknown" or "Intel(R) Corporation"
       * prop.modelName is "0x1234" (PCI device id)
       * prop.brandName is "Unknown" (subvendor name)
       * prop.serialNumber is "Unknown"
       * prop.boardNumber is "Unknown"
       */
      if (strcmp((const char *) prop.vendorName, "Unknown"))
        hwloc_obj_add_info(osdev, "LevelZeroVendor", (const char *) prop.vendorName);
      if (strcmp((const char *) prop.vendorName, "Unknown"))
        /* Model is always "0x...." in early implementations */
        hwloc_obj_add_info(osdev, "LevelZeroModel", (const char *) prop.modelName);
      if (strcmp((const char *) prop.brandName, "Unknown"))
        hwloc_obj_add_info(osdev, "LevelZeroBrand", (const char *) prop.brandName);
      if (strcmp((const char *) prop.serialNumber, "Unknown"))
        hwloc_obj_add_info(osdev, "LevelZeroSerialNumber", (const char *) prop.serialNumber);
      if (strcmp((const char *) prop.boardNumber, "Unknown"))
        hwloc_obj_add_info(osdev, "LevelZeroBoardNumber", (const char *) prop.boardNumber);

      nr_cqprops = 0;
      res = zeDeviceGetCommandQueueGroupProperties(dvh[j], &nr_cqprops, NULL);
      if (res == ZE_RESULT_SUCCESS && nr_cqprops) {
        ze_command_queue_group_properties_t *cqprops;
        cqprops = malloc(nr_cqprops * sizeof(*cqprops));
        if (cqprops) {
          res = zeDeviceGetCommandQueueGroupProperties(dvh[j], &nr_cqprops, cqprops);
          if (res == ZE_RESULT_SUCCESS) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%u", nr_cqprops);
            hwloc_obj_add_info(osdev, "LevelZeroCQGroups", tmp);
            for(k=0; k<nr_cqprops; k++) {
              char name[32];
              snprintf(name, sizeof(name), "LevelZeroCQGroup%u", k);
              snprintf(tmp, sizeof(tmp), "%u*0x%lx", (unsigned) cqprops[k].numQueues, (unsigned long) cqprops[k].flags);
              hwloc_obj_add_info(osdev, name, tmp);
            }
          }
          free(cqprops);
        }
      }

      parent = NULL;
      res = zesDevicePciGetProperties(sdvh, &pci);
      if (res == ZE_RESULT_SUCCESS) {
        parent = hwloc_pci_find_parent_by_busid(topology,
                                                pci.address.domain,
                                                pci.address.bus,
                                                pci.address.device,
                                                pci.address.function);
        if (parent && parent->type == HWLOC_OBJ_PCI_DEVICE) {
          if (pci.maxSpeed.maxBandwidth)
            parent->attr->pcidev.linkspeed = ((float)pci.maxSpeed.maxBandwidth)/1000/1000/1000;
        }
      }
      if (!parent)
        parent = hwloc_get_root_obj(topology);

      hwloc_insert_object_by_parent(topology, parent, osdev);
      zeidx++;
    }

    free(dvh);
  }

  free(drh);
  return 0;
}

static struct hwloc_backend *
hwloc_levelzero_component_instantiate(struct hwloc_topology *topology,
                                      struct hwloc_disc_component *component,
                                      unsigned excluded_phases __hwloc_attribute_unused,
                                      const void *_data1 __hwloc_attribute_unused,
                                      const void *_data2 __hwloc_attribute_unused,
                                      const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_levelzero_discover;
  return backend;
}

static struct hwloc_disc_component hwloc_levelzero_disc_component = {
  "levelzero",
  HWLOC_DISC_PHASE_IO,
  HWLOC_DISC_PHASE_GLOBAL,
  hwloc_levelzero_component_instantiate,
  10, /* after pci */
  1,
  NULL
};

static int
hwloc_levelzero_component_init(unsigned long flags)
{
  if (flags)
    return -1;
  if (hwloc_plugin_check_namespace("levelzero", "hwloc_backend_alloc") < 0)
    return -1;
  return 0;
}

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_levelzero_component;
#endif

const struct hwloc_component hwloc_levelzero_component = {
  HWLOC_COMPONENT_ABI,
  hwloc_levelzero_component_init, NULL,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_levelzero_disc_component
};
