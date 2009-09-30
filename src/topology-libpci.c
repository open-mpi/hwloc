/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HAVE_LIBPCI

#include <pci/pci.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#define CONFIG_SPACE_CACHESIZE 64

void
hwloc_look_libpci(struct hwloc_topology *topology)
{
  struct pci_access *pciaccess;
  struct pci_dev *pcidev;

  pciaccess = pci_alloc();
  pci_init(pciaccess);
  pci_scan_bus(pciaccess);

  pcidev = pciaccess->devices;
  while (pcidev) {
    char busid[14];
    char name[128];
    char path[256];
    char localcpus[4096];
    u8 config_space_cache[CONFIG_SPACE_CACHESIZE];
    struct hwloc_obj *obj;
    unsigned char headertype;
    unsigned os_index;
    int fd;

    /* might be useful for debugging (note that domain might be truncated) */
    os_index = (pcidev->domain << 24) + (pcidev->bus << 16) + (pcidev->dev << 8) + pcidev->func;

    obj = hwloc_alloc_setup_object(HWLOC_OBJ_PCI_DEVICE, os_index);

    pci_read_block(pcidev, 0, config_space_cache, CONFIG_SPACE_CACHESIZE);

    obj->attr->pcidev.domain = pcidev->domain;
    obj->attr->pcidev.bus = pcidev->bus;
    obj->attr->pcidev.dev = pcidev->dev;
    obj->attr->pcidev.func = pcidev->func;
    obj->attr->pcidev.vendor_id = pcidev->vendor_id;
    obj->attr->pcidev.device_id = pcidev->device_id;
    obj->attr->pcidev.class = pcidev->device_class;
    assert(PCI_REVISION_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.revision = config_space_cache[PCI_REVISION_ID];
    assert(PCI_SUBSYSTEM_VENDOR_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.subvendor_id = config_space_cache[PCI_SUBSYSTEM_VENDOR_ID];
    assert(PCI_SUBSYSTEM_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.subdevice_id = config_space_cache[PCI_SUBSYSTEM_ID];

    snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
             pcidev->domain, pcidev->bus, pcidev->dev, pcidev->func);
    printf("%s [%04x:%04x (%04x:%04x)] rev=%02x class=%04x\n",
           busid, pcidev->vendor_id, pcidev->device_id, obj->attr->pcidev.subvendor_id, obj->attr->pcidev.subdevice_id, obj->attr->pcidev.revision, pcidev->device_class);

    assert(PCI_HEADER_TYPE < CONFIG_SPACE_CACHESIZE);
    headertype = config_space_cache[PCI_HEADER_TYPE] & 0x7f;

    if (pcidev->device_class == PCI_CLASS_BRIDGE_PCI
      && headertype == PCI_HEADER_TYPE_BRIDGE) {
      assert(PCI_PRIMARY_BUS < CONFIG_SPACE_CACHESIZE);
      assert(PCI_SECONDARY_BUS < CONFIG_SPACE_CACHESIZE);
      assert(PCI_SUBORDINATE_BUS < CONFIG_SPACE_CACHESIZE);
      assert(config_space_cache[PCI_PRIMARY_BUS] == pcidev->bus);
      obj->type = HWLOC_OBJ_PCI_BRIDGE;
      obj->attr->pcibridge.secondary_bus = config_space_cache[PCI_SECONDARY_BUS];
      obj->attr->pcibridge.subordinate_bus = config_space_cache[PCI_SUBORDINATE_BUS];
      printf("  PCIBridge, buses: primary %hhx secondary %hhx subordinate %hhx\n",
	     pcidev->bus, obj->attr->pcibridge.secondary_bus, obj->attr->pcibridge.subordinate_bus);
    }

    strcpy(name, "??");
    pci_lookup_name(pciaccess, name, sizeof(name),
                    PCI_LOOKUP_VENDOR|PCI_LOOKUP_DEVICE|PCI_LOOKUP_NO_NUMBERS,
                    pcidev->vendor_id, pcidev->device_id);
    printf("  %s\n", name);

#ifdef LINUX_SYS
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/local_cpus", busid);
    fd = open(path, O_RDONLY);
    read(fd, localcpus, sizeof(localcpus));
    close(fd);
    printf("  Cpuset %s\n", localcpus);
#endif

    /* do nothing for now */
    free(obj->attr);
    free(obj);

    pcidev = pcidev->next;
  }

  pci_cleanup(pciaccess);
}

#endif /* HAVE_LIBPCI */
