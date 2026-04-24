/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_RSMI_AMDSMI_H
#define HWLOC_PORT_RSMI_AMDSMI_H

/* we need to replace any RSMI-related #define that configure may have put in private/autogen/config.h */
#ifndef HWLOC_CONFIGURE_H
#error amd_smi.h must be included after private/autogen/config.h
#endif

#define AMDSMI_LIB_VERSION_MAJOR 26

typedef int amdsmi_status_t;
#define AMDSMI_STATUS_SUCCESS 0

typedef void * amdsmi_socket_handle;
typedef void * amdsmi_processor_handle;

extern amdsmi_status_t amdsmi_init(int);
#define AMDSMI_INIT_AMD_GPUS 1

extern amdsmi_status_t amdsmi_shut_down(void);

extern amdsmi_status_t amdsmi_status_code_to_string(amdsmi_status_t, const char**);

extern amdsmi_status_t amdsmi_get_socket_handles(uint32_t*, amdsmi_socket_handle*);

extern amdsmi_status_t amdsmi_get_processor_handles(amdsmi_socket_handle, uint32_t*, amdsmi_processor_handle*);

typedef struct {
  uint32_t hip_id;
} amdsmi_enumeration_info_t;
extern amdsmi_status_t amdsmi_get_gpu_enumeration_info(amdsmi_processor_handle, amdsmi_enumeration_info_t*);

typedef struct {
  char *product_name;
  char *product_serial;
} amdsmi_board_info_t;
extern amdsmi_status_t amdsmi_get_gpu_board_info(amdsmi_processor_handle, amdsmi_board_info_t*);

#define AMDSMI_GPU_UUID_SIZE 16
extern amdsmi_status_t amdsmi_get_gpu_device_uuid(amdsmi_processor_handle, uint32_t*, char*);

typedef struct {
  uint64_t xgmi_hive_id;
} amdsmi_xgmi_info_t;
extern amdsmi_status_t amdsmi_get_xgmi_info(amdsmi_processor_handle, amdsmi_xgmi_info_t*);

#define AMDSMI_MEM_TYPE_VRAM 1
#define AMDSMI_MEM_TYPE_VIS_VRAM 2
#define AMDSMI_MEM_TYPE_GTT 3
extern amdsmi_status_t amdsmi_get_gpu_memory_total(amdsmi_processor_handle, int, uint64_t*);

extern amdsmi_status_t amdsmi_get_gpu_bdf_id(amdsmi_processor_handle, uint64_t*);

typedef struct {
  struct {
    uint32_t pcie_width;
    uint32_t pcie_speed;
  } pcie_metric;
} amdsmi_pcie_info_t;

extern amdsmi_status_t amdsmi_get_pcie_info(amdsmi_processor_handle, amdsmi_pcie_info_t*);

typedef struct {
  uint64_t vram_max_bandwidth;
} amdsmi_vram_info_t;

extern amdsmi_status_t amdsmi_get_gpu_vram_info(amdsmi_processor_handle, amdsmi_vram_info_t*);

typedef int amdsmi_link_type_t;
#define AMDSMI_LINK_TYPE_XGMI 2

extern amdsmi_status_t amdsmi_topo_get_link_type(amdsmi_processor_handle, amdsmi_processor_handle, uint64_t*, amdsmi_link_type_t*);

extern amdsmi_status_t amdsmi_get_minmax_bandwidth_between_processors(amdsmi_processor_handle, amdsmi_processor_handle, uint64_t* , uint64_t*);

#endif /* HWLOC_PORT_RSMI_AMDSMI_H */
