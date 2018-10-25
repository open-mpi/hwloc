/*
 * Copyright © 2013-2018 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_OPENCL_CL_CL_EXT_H
#define HWLOC_PORT_OPENCL_CL_CL_EXT_H

#include <CL/cl.h>

#define CL_DEVICE_TOPOLOGY_AMD                      0x4037
typedef union {
  struct { cl_uint type; cl_uint data[5]; } raw;
  struct { cl_uint type; cl_char unused[17]; cl_char bus; cl_char device; cl_char function; } pcie;
} cl_device_topology_amd;
#define CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD            1

#define CL_DEVICE_BOARD_NAME_AMD                    0x4038

#endif /* HWLOC_PORT_OPENCL_CL_CL_EXT_H */
