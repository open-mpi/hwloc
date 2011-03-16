/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>

#include <assert.h>

/* check hwloc_get_next_pcidev() */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t obj;

  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_IO);
  hwloc_topology_load(topology);

  obj = NULL;
  while ((obj = hwloc_get_next_pcidev(topology, obj)) != NULL) {
    assert(obj->type == HWLOC_OBJ_PCI_DEVICE);
    printf("Found PCI device class %04x vendor %04x model %04x\n",
	   obj->attr->pcidev.class_id, obj->attr->pcidev.vendor_id, obj->attr->pcidev.device_id);
  }

  hwloc_topology_destroy(topology);

  return 0;
}
