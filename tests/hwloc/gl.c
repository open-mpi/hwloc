/*
 * Copyright © 2012 Blue Brain Project, BBP/EPFL. All rights reserved.
 * Copyright © 2012-2024 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "hwloc.h"
#include "hwloc/gl.h"
#include "hwloc/helper.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

static int check_gl_backend(hwloc_topology_t topology)
{
  struct hwloc_infos_s *infos = hwloc_topology_get_infos(topology);
  unsigned i;
  for(i=0; i<infos->count; i++)
    if (!strcmp(infos->array[i].name, "Backend")
        && !strcmp(infos->array[i].value, "GL"))
      return 1;
  return 0;
}

int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t pcidev, osdev, parent;
  hwloc_obj_t firstgpu = NULL, lastgpu = NULL;
  unsigned port, device;
  char* cpuset_string;
  unsigned nr_pcidev;
  unsigned nr_osdev;
  unsigned nr_gpus;
  unsigned i;
  int has_gl_backend;
  int err;

  hwloc_topology_init(&topology); /* Topology initialization */

  /* Flags used for loading the I/O devices, bridges and their relevant info */
  hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);

  /* Perform topology detection */
  hwloc_topology_load(topology);

  has_gl_backend = check_gl_backend(topology);

  /* Case 1: Get the cpusets of the packages connecting the PCI devices in the topology */
  nr_pcidev = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PCI_DEVICE);
  for (i = 0; i < nr_pcidev; ++i) {
      pcidev = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PCI_DEVICE, i);
      parent = hwloc_get_non_io_ancestor_obj(topology, pcidev);
      /* Print the cpuset corresponding to each pci device */
      hwloc_bitmap_asprintf(&cpuset_string, parent->cpuset);
      printf(" %s | %s \n", cpuset_string, pcidev->name);
      free(cpuset_string);
    }

  /* Case 2: Get the number of connected GPUs in the topology and their attached displays */
  nr_gpus = 0;
  nr_osdev = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_OS_DEVICE);
  for (i = 0; i < nr_osdev; ++i) {
      const char *model;
      osdev = hwloc_get_obj_by_type(topology, HWLOC_OBJ_OS_DEVICE, i);

      model = hwloc_obj_get_info_by_name(osdev, "GPUModel");

      err = hwloc_gl_get_display_by_osdev(topology, osdev, &port, &device);
      if (!err) {
        assert(has_gl_backend);
	assert(osdev->attr->osdev.types == HWLOC_OBJ_OSDEV_GPU);

	if (!firstgpu)
	  firstgpu = osdev;
	lastgpu = osdev;
	printf("GPU #%u (%s) is connected to DISPLAY:%u.%u \n", nr_gpus, model, port, device);
	nr_gpus++;
      }
    }

  /* Case 3: Get the first GPU connected to a valid display, specified by its port and device */
  if (firstgpu) {
    assert(sscanf(firstgpu->name, ":%u.%u", &port, &device) == 2);
    osdev = hwloc_gl_get_display_osdev_by_port_device(topology, port, device);
    assert(osdev == firstgpu);
    pcidev = osdev->parent;
    parent = hwloc_get_non_io_ancestor_obj(topology, pcidev);
    hwloc_bitmap_asprintf(&cpuset_string, parent->cpuset);
    printf("GPU %s (PCI %04x:%02x:%02x.%01x) is connected to DISPLAY:%u.%u close to %s\n",
	   osdev->name,
	   pcidev->attr->pcidev.domain, pcidev->attr->pcidev.bus, pcidev->attr->pcidev.dev, pcidev->attr->pcidev.func,
	   port, device, cpuset_string);
    free(cpuset_string);
  }

  /* Case 4: Get the last GPU connected to a valid display, specified by its name */
  if (lastgpu) {
    assert(sscanf(lastgpu->name, ":%u.%u", &port, &device) == 2);
    osdev = hwloc_gl_get_display_osdev_by_name(topology, lastgpu->name);
    assert(osdev == lastgpu);
    pcidev = osdev->parent;
    parent = hwloc_get_non_io_ancestor_obj(topology, pcidev);
    hwloc_bitmap_asprintf(&cpuset_string, parent->cpuset);
    printf("GPU %s (PCI %04x:%02x:%02x.%01x) is connected to DISPLAY:%u.%u close to %s\n",
	   osdev->name,
	   pcidev->attr->pcidev.domain, pcidev->attr->pcidev.bus, pcidev->attr->pcidev.dev, pcidev->attr->pcidev.func,
	   port, device, cpuset_string);
    free(cpuset_string);
  }

  return 0;
}
