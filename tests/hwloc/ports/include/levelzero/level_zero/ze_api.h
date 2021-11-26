/*
 * Copyright © 2020-2021 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_L0_ZE_API_H
#define HWLOC_PORT_L0_ZE_API_H

typedef int ze_result_t;
#define ZE_RESULT_SUCCESS 0

typedef void * ze_driver_handle_t;
typedef void * ze_device_handle_t;

extern ze_result_t zeInit(int);
extern ze_result_t zeDriverGet(uint32_t *, ze_driver_handle_t *);
extern ze_result_t zeDeviceGet(ze_driver_handle_t, uint32_t *, ze_device_handle_t *);

typedef enum _ze_device_type {
  ZE_DEVICE_TYPE_GPU = 1,
  ZE_DEVICE_TYPE_CPU = 2,
  ZE_DEVICE_TYPE_FPGA = 3,
  ZE_DEVICE_TYPE_MCA = 4,
  ZE_DEVICE_TYPE_VPU = 5
} ze_device_type_t;

#define ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE (1<<1)

typedef struct ze_device_properties {
  ze_device_type_t type;
  unsigned flags;
  uint32_t numThreadsPerEU;
  uint32_t numEUsPerSubslice;
  uint32_t numSubslicesPerSlice;
  uint32_t numSlices;
} ze_device_properties_t;

extern ze_result_t zeDeviceGetProperties(ze_device_handle_t, ze_device_properties_t *);

typedef struct ze_command_queue_group_properties {
  unsigned long flags;
  unsigned numQueues;
} ze_command_queue_group_properties_t;

extern ze_result_t zeDeviceGetCommandQueueGroupProperties(ze_driver_handle_t, uint32_t *, ze_command_queue_group_properties_t *);

extern ze_result_t zeDeviceGetSubDevices(ze_device_handle_t, uint32_t *, ze_device_handle_t*);

#endif /* HWLOC_PORT_L0_ZE_API_H */
