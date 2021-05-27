/*
 * Copyright © 2020 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_L0_ZET_API_H
#define HWLOC_PORT_L0_ZET_API_H

#include "ze_api.h"

typedef void * zes_device_handle_t;

typedef struct {
  char *vendorName;
  char *brandName;
  char *modelName;
  char *serialNumber;
  char *boardNumber;
} zes_device_properties_t;

typedef struct {
  struct {
    unsigned domain, bus, device, function;
  } address;
  struct {
    unsigned gen;
    unsigned lanes;
    unsigned maxBandwidth;
  } maxSpeed;
} zes_pci_properties_t;

extern ze_result_t zesDeviceGetProperties(zes_device_handle_t, zes_device_properties_t *);
extern ze_result_t zesDevicePciGetProperties(zes_device_handle_t, zes_pci_properties_t *);

#endif /* HWLOC_PORT_L0_ZET_API_H */
