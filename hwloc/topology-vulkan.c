/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2009-2025 Christopher Taylor.  All rights reserved.
 * Copyright © 2025-2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "hwloc.h"
#include "hwloc/plugins.h"
#include "private/autogen/config.h"

#include "private/debug.h"
#include "private/misc.h"

#include <vulkan/vulkan.h>

#include "hwloc/vulkan.h"

#define HWLOC_SHOWMSG_VULKAN 16

static int hwloc_vulkan_check_extension(VkPhysicalDevice device, const char *ext) {
    uint32_t extcount = 0;
    VkExtensionProperties *extprops;
    VkResult res;
    int found = 0;
    uint32_t i;

    res = vkEnumerateDeviceExtensionProperties(device, NULL, &extcount, NULL);
    if (res != VK_SUCCESS || extcount == 0)
        return 0;

    extprops = malloc(extcount * sizeof(*extprops));
    if (!extprops)
        return 0;

    res = vkEnumerateDeviceExtensionProperties(device, NULL, &extcount, extprops);
    if (res == VK_SUCCESS) {
        for (i = 0; i < extcount; i++) {
            if (strcmp(extprops[i].extensionName, ext) == 0) {
                found = 1;
                break;
            }
        }
    }

    free(extprops);
    return found;
}

static int hwloc_vulkan_device_has_compute_support(VkPhysicalDevice device) {
  uint32_t queue_family_count = 0;
  VkQueueFamilyProperties *queue_families;
  uint32_t i;
  int has_compute_queue = 0;

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  if (queue_family_count == 0)
    return 0;

  queue_families = malloc(queue_family_count * sizeof(*queue_families));
  if (!queue_families)
    return 0;

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  for (i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      has_compute_queue = 1;
      break;
    }
  }

  free(queue_families);
  return has_compute_queue;
}

static int hwloc_vulkan_device_has_graphics_support(VkPhysicalDevice device) {
  uint32_t queue_family_count = 0;
  VkQueueFamilyProperties *queue_families;
  uint32_t i;
  int has_graphics_queue = 0;

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
  if (queue_family_count == 0)
    return 0;

  queue_families = malloc(queue_family_count * sizeof(*queue_families));
  if (!queue_families)
    return 0;

  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           queue_families);

  for (i = 0; i < queue_family_count; i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      has_graphics_queue = 1;
      break;
    }
  }

  free(queue_families);
  return has_graphics_queue;
}

static int hwloc_vulkan_discover(struct hwloc_backend *backend,
                                 struct hwloc_disc_status *dstatus) {
  struct hwloc_topology *topology = backend->topology;
  enum hwloc_type_filter_e filter;
  VkInstance instance;
  VkResult res;
  uint32_t nr_physical_devices;
  VkPhysicalDevice *physical_devices;
  uint32_t i, j;
  unsigned compute_index = 0;
  unsigned added = 0;

  assert(dstatus->phase == HWLOC_DISC_PHASE_IO);

  hwloc_topology_get_type_filter(topology, HWLOC_OBJ_OS_DEVICE, &filter);
  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE) {
    return 0;
  }

  VkApplicationInfo appinfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "hwloc",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "hwloc",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  VkInstanceCreateInfo createinfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appinfo,
  };

  res = vkCreateInstance(&createinfo, NULL, &instance);
  if (res != VK_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_VULKAN)) {
      fprintf(stderr,
              "hwloc/vulkan: Failed to create instance with "
              "vkCreateInstance(): %d\n",
              res);
    }
    return -1;
  }

  res = vkEnumeratePhysicalDevices(instance, &nr_physical_devices, NULL);
  if (res != VK_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_VULKAN)) {
      fprintf(stderr,
              "hwloc/vulkan: Failed to enumerate physical devices with "
              "vkEnumeratePhysicalDevices(): %d\n",
              res);
    }
    vkDestroyInstance(instance, NULL);
    return -1;
  }

  if (nr_physical_devices == 0) {
    vkDestroyInstance(instance, NULL);
    return 0;
  }

  hwloc_debug("%u Vulkan physical devices\n", nr_physical_devices);

  physical_devices = malloc(nr_physical_devices * sizeof(*physical_devices));
  if (!physical_devices) {
    vkDestroyInstance(instance, NULL);
    return -1;
  }

  res = vkEnumeratePhysicalDevices(instance, &nr_physical_devices,
                                   physical_devices);
  if (res != VK_SUCCESS) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_VULKAN)) {
      fprintf(stderr,
              "hwloc/vulkan: Failed to enumerate physical devices with "
              "vkEnumeratePhysicalDevices(): %d\n",
              res);
    }
    free(physical_devices);
    vkDestroyInstance(instance, NULL);
    return -1;
  }

  for (j = 0; j < nr_physical_devices; j++) {
    VkPhysicalDevice device = physical_devices[j];
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceType device_type;
    unsigned pcidomain, pcibus, pcidev, pcifunc;
    hwloc_obj_t osdev, parent;
    char buffer[64];
    const char *type_str;

    if (!hwloc_vulkan_device_has_compute_support(device)) {
      hwloc_debug("Skipping Vulkan device %u - no compute support\n", j);
      continue;
    }

    vkGetPhysicalDeviceProperties(device, &props);
    device_type = props.deviceType;

    switch (device_type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      type_str = "GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      type_str = "Integrated GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      type_str = "Virtual GPU";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      type_str = "CPU";
      break;
    default:
      type_str = "Unknown";
      break;
    }

    hwloc_debug("This is vulkan%u%u\n", 0, compute_index);

    osdev = hwloc_alloc_setup_object(topology, HWLOC_OBJ_OS_DEVICE,
                                     HWLOC_UNKNOWN_INDEX);
    snprintf(buffer, sizeof(buffer), "vulkan%u%u", 0, compute_index);
    osdev->name = strdup(buffer);
    osdev->depth = HWLOC_TYPE_DEPTH_UNKNOWN;

    osdev->subtype = strdup("Vulkan");

    hwloc_obj_add_info(osdev, "VulkanDeviceType", type_str);

    if (device_type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
        device_type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
      if (hwloc_vulkan_device_has_graphics_support(device)) {
        osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_COPROC | HWLOC_OBJ_OSDEV_GPU;
      } else {
        osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_COPROC;
      }
    } else {
      osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_COPROC;
    }

    if ((osdev->attr->osdev.types &
         (HWLOC_OBJ_OSDEV_GPU | HWLOC_OBJ_OSDEV_COPROC)) ==
        HWLOC_OBJ_OSDEV_COPROC) {
      hwloc_obj_add_info(osdev, "VulkanComputeOnly", "1");
    }

    snprintf(buffer, sizeof(buffer), "0x%04x", props.vendorID);
    hwloc_obj_add_info(osdev, "VulkanVendorID", buffer);

    if (props.deviceName[0] != '\0') {
      hwloc_obj_add_info(osdev, "VulkanDeviceName", props.deviceName);
    }

    snprintf(buffer, sizeof(buffer), "%u", props.driverVersion);
    hwloc_obj_add_info(osdev, "VulkanDriverVersion", buffer);

    snprintf(buffer, sizeof(buffer), "%d",
            hwloc_vulkan_check_extension(device, "VK_KHR_external_memory"));
    hwloc_obj_add_info(osdev, "VK_KHR_external_memory", buffer);

    snprintf(buffer, sizeof(buffer), "%d",
            hwloc_vulkan_check_extension(device,
                                       "VK_KHR_buffer_device_address"));
    hwloc_obj_add_info(osdev, "VK_KHR_buffer_device_address", buffer);

    snprintf(buffer, sizeof(buffer), "%d",
            hwloc_vulkan_check_extension(device, "VK_KHR_timeline_semaphores"));
    hwloc_obj_add_info(osdev, "VK_KHR_timeline_semaphores", buffer);

    snprintf(buffer, sizeof(buffer), "%u", compute_index);
    hwloc_obj_add_info(osdev, "VulkanDeviceIndex", buffer);

    compute_index++;

    parent = NULL;
    if (hwloc_vulkan_get_device_pci_busid(device, &pcidomain, &pcibus, &pcidev,
                                          &pcifunc) == 0) {
      parent = hwloc_pci_get_parent_by_busid(topology, pcidomain, pcibus,
                                             pcidev, pcifunc);
    } else {
      hwloc_debug("Failed to find the PCI id of the Vulkan device\n");
    }
    if (!parent)
      parent = hwloc_get_root_obj(topology);

    hwloc_insert_object_by_parent(topology, parent, osdev);
    added++;
  }

  free(physical_devices);
  vkDestroyInstance(instance, NULL);

  if (added)
    hwloc_modify_infos(hwloc_topology_get_infos(topology),
                       HWLOC_MODIFY_INFOS_OP_ADD, "Backend", "Vulkan");

  return 0;
}

static struct hwloc_backend *hwloc_vulkan_component_instantiate(
    struct hwloc_topology *topology, struct hwloc_disc_component *component,
    unsigned excluded_phases __hwloc_attribute_unused,
    const void *_data1 __hwloc_attribute_unused,
    const void *_data2 __hwloc_attribute_unused,
    const void *_data3 __hwloc_attribute_unused) {
  struct hwloc_backend *backend;

  backend = hwloc_backend_alloc(topology, component, 0);
  if (!backend)
    return NULL;
  backend->discover = hwloc_vulkan_discover;
  return backend;
}

static struct hwloc_disc_component hwloc_vulkan_disc_component = {
    "vulkan",
    HWLOC_DISC_PHASE_IO,
    HWLOC_DISC_PHASE_GLOBAL,
    hwloc_vulkan_component_instantiate,
    10,
    1,
    NULL};

static int hwloc_vulkan_component_init(unsigned long flags) {
  if (flags)
    return -1;
  if (hwloc_plugin_check_namespace("vulkan", "hwloc_backend_alloc") < 0)
    return -1;
  return 0;
}

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_vulkan_component;
#endif

const struct hwloc_component hwloc_vulkan_component = {
    HWLOC_COMPONENT_ABI,
    hwloc_vulkan_component_init,
    NULL,
    HWLOC_COMPONENT_TYPE_DISC,
    0,
    &hwloc_vulkan_disc_component};
