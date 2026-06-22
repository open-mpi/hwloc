/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2025-2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <assert.h>
#include <stdio.h>

#include <vulkan/vulkan.h>

#include "hwloc.h"
#include "hwloc/vulkan.h"

static int check_vulkan_backend(hwloc_topology_t topology) {
  struct hwloc_infos_s *infos = hwloc_topology_get_infos(topology);
  unsigned i;
  for (i = 0; i < infos->count; i++)
    if (!strcmp(infos->array[i].name, "Backend") &&
        !strcmp(infos->array[i].value, "Vulkan"))
      return 1;
  return 0;
}

int main(void) {
  hwloc_topology_t topology;
  VkInstance instance;
  VkApplicationInfo appinfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "hwloc-test",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "hwloc",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo createinfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appinfo,
  };
  VkResult res;
  uint32_t nbdevices, i, count;
  VkPhysicalDevice *devices;
  hwloc_obj_t osdev;
  int has_vulkan_backend;
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PCI_DEVICE,
                                 HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_OS_DEVICE,
                                 HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
  hwloc_topology_load(topology);

  has_vulkan_backend = check_vulkan_backend(topology);

  res = vkCreateInstance(&createinfo, NULL, &instance);
  if (res != VK_SUCCESS) {
    printf("No Vulkan support (or no devices)\n");
    hwloc_topology_destroy(topology);
    return 0;
  }

  nbdevices = 0;
  res = vkEnumeratePhysicalDevices(instance, &nbdevices, NULL);
  if (res != VK_SUCCESS || !nbdevices) {
    printf("No Vulkan devices\n");
    vkDestroyInstance(instance, NULL);
    hwloc_topology_destroy(topology);
    return 0;
  }

  devices = malloc(nbdevices * sizeof(*devices));
  if (!devices) {
    vkDestroyInstance(instance, NULL);
    hwloc_topology_destroy(topology);
    return 0;
  }

  res = vkEnumeratePhysicalDevices(instance, &nbdevices, devices);
  if (res != VK_SUCCESS) {
    free(devices);
    vkDestroyInstance(instance, NULL);
    hwloc_topology_destroy(topology);
    return 0;
  }

  printf("found %u Vulkan physical devices\n", nbdevices);

  count = 0;
  for (i = 0; i < nbdevices; i++) {
    VkPhysicalDevice device = devices[i];
    VkPhysicalDeviceProperties props;
    hwloc_bitmap_t set;
    hwloc_obj_t osdev2, ancestor;
    unsigned p, d;
    const char *value;

    vkGetPhysicalDeviceProperties(device, &props);

    osdev = hwloc_vulkan_get_device_osdev_by_index(topology, 0, i);
    if (!osdev) {
      printf("No OSDev for device %u\n", i);
      continue;
    }

    osdev2 = hwloc_vulkan_get_device_osdev(topology, device);
    if (osdev2) {
      assert(osdev == osdev2);
    }

    ancestor = hwloc_get_non_io_ancestor_obj(topology, osdev);

    set = hwloc_bitmap_alloc();
    err = hwloc_vulkan_get_device_cpuset(topology, device, set);
    if (err < 0) {
      printf("no cpuset for device %u\n", i);
    } else {
      char *cpuset_string = NULL;
      hwloc_bitmap_asprintf(&cpuset_string, set);
      printf("got cpuset %s for device %u\n", cpuset_string, i);
      free(cpuset_string);
      if (hwloc_bitmap_isequal(hwloc_topology_get_complete_cpuset(topology),
                               hwloc_topology_get_topology_cpuset(topology)))
        assert(hwloc_bitmap_isequal(set, ancestor->cpuset));
    }
    hwloc_bitmap_free(set);

    printf("found OSDev %s\n", osdev->name);
    err = sscanf(osdev->name, "vulkan%ud%u", &p, &d);
    assert(err == 2);
    assert(p == 0);
    assert(d == i);

    assert(has_vulkan_backend);

    assert((osdev->attr->osdev.types == HWLOC_OBJ_OSDEV_COPROC) ||
           (osdev->attr->osdev.types ==
            (HWLOC_OBJ_OSDEV_COPROC | HWLOC_OBJ_OSDEV_GPU)));

    value = osdev->subtype;
    assert(value);
    err = strcmp(value, "Vulkan");
    assert(!err);

    value = hwloc_obj_get_info_by_name(osdev, "VulkanDeviceName");
    printf("found OSDev model %s\n", value);

    value = hwloc_obj_get_info_by_name(osdev, "VulkanDeviceType");
    printf("found OSDev type %s\n", value);

    value = hwloc_obj_get_info_by_name(osdev, "VulkanComputeOnly");
    if (value) {
      printf("VulkanComputeOnly=%s\n", value);
      assert(osdev->attr->osdev.types == HWLOC_OBJ_OSDEV_COPROC);
      err = strcmp(value, "1");
      assert(!err);
    }

    value = hwloc_obj_get_info_by_name(osdev, "VK_KHR_external_memory");
    if (value) {
      printf("VK_KHR_external_memory=%s\n", value);
      err = strcmp(value, "0") && strcmp(value, "1");
      assert(!err); // must be "0" or "1"
    }

    value = hwloc_obj_get_info_by_name(osdev, "VK_KHR_buffer_device_address");
    if (value) {
      printf("VK_KHR_buffer_device_address=%s\n", value);
      err = strcmp(value, "0") && strcmp(value, "1");
      assert(!err);
    }

    value = hwloc_obj_get_info_by_name(osdev, "VK_KHR_timeline_semaphores");
    if (value) {
      printf("VK_KHR_timeline_semaphores=%s\n", value);
      err = strcmp(value, "0") && strcmp(value, "1");
      assert(!err);
    }

    count++;
  }

  free(devices);
  vkDestroyInstance(instance, NULL);
  hwloc_topology_destroy(topology);

  return 0;
}