/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2010 inria.  All rights reserved.
 * Copyright © 2009-2010 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include <infiniband/verbs.h>
#include <hwloc.h>
#include <hwloc/openfabrics-verbs.h>

/* check the ibverbs helpers */

int main(void)
{
  hwloc_topology_t topology;
  struct ibv_device **dev_list, *dev;
  int count, i;
  int err;

  dev_list = ibv_get_device_list(&count);
  if (!dev_list) {
    fprintf(stderr, "ibv_get_device_list failed\n");
    return 0;
  }
  printf("ibv_get_device_list found %d devices\n", count);

  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
  hwloc_topology_load(topology);

  for(i=0; i<count; i++) {
    hwloc_bitmap_t set;
    dev = dev_list[i];

    set = hwloc_bitmap_alloc();
    err = hwloc_ibv_get_device_cpuset(topology, dev, set);
    if (err < 0) {
      printf("failed to get cpuset for device %d (%s)\n",
	     i, ibv_get_device_name(dev));
    } else {
      char *cpuset_string = NULL;
      hwloc_obj_t os, pci;

      hwloc_bitmap_asprintf(&cpuset_string, set);
      printf("got cpuset %s for device %d (%s)\n",
	     cpuset_string, i, ibv_get_device_name(dev));
      free(cpuset_string);

      os = hwloc_ibv_get_device_osdev(topology, dev);
      pci = hwloc_ibv_get_device_pcidev(topology, dev);
      if (os) {
	assert(os->type == HWLOC_OBJ_OS_DEVICE);
	printf("found OS object subtype %u lindex %u name %s\n",
	       (unsigned) os->attr->osdev.type, os->logical_index, os->name);
	assert(os->attr->osdev.type == HWLOC_OBJ_OSDEV_OPENFABRICS);
      }
      if (pci) {
	assert(pci->type == HWLOC_OBJ_PCI_DEVICE);
	printf("found PCI object %04x:%02x:%02x.%01x lindex %u name %s\n",
	       pci->attr->pcidev.domain, pci->attr->pcidev.bus, pci->attr->pcidev.dev, pci->attr->pcidev.func,
	       pci->logical_index, pci->name);
      }
    }
    hwloc_bitmap_free(set);
  }

  hwloc_topology_destroy(topology);

  ibv_free_device_list(dev_list);

  return 0;
}
