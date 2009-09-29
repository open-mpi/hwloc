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
    unsigned char headertype;
    int fd;

    snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
             pcidev->domain, pcidev->bus, pcidev->dev, pcidev->func);
    printf("%s [%04x:%04x] class=%04x\n",
           busid, pcidev->vendor_id, pcidev->device_id, pcidev->device_class);

    pci_read_block(pcidev, PCI_HEADER_TYPE, &headertype, 1);
    headertype &= 0x7f;
      printf("  type %d\n", headertype);

    if (pcidev->device_class == PCI_CLASS_BRIDGE_PCI
        && headertype == PCI_HEADER_TYPE_BRIDGE) {
      unsigned char prim,sec,subor;
      pci_read_block(pcidev, PCI_PRIMARY_BUS, &prim, 1);
      pci_read_block(pcidev, PCI_SECONDARY_BUS, &sec, 1);
      pci_read_block(pcidev, PCI_SUBORDINATE_BUS, &subor, 1);
      printf("pci bridge with buses: primary %hhx secondary %hhx subordinate %hhx\n",
	     prim, sec, subor);
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

    pcidev = pcidev->next;
  }

  pci_cleanup(pciaccess);
}

#endif /* HAVE_LIBPCI */
