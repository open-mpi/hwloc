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
    unsigned char headertype;
    int fd;

    snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
             pcidev->domain, pcidev->bus, pcidev->dev, pcidev->func);
    printf("%s [%04x:%04x] class=%04x\n",
           busid, pcidev->vendor_id, pcidev->device_id, pcidev->device_class);

    pci_read_block(pcidev, 0, config_space_cache, CONFIG_SPACE_CACHESIZE);

    assert(PCI_HEADER_TYPE < CONFIG_SPACE_CACHESIZE);
    headertype = config_space_cache[PCI_HEADER_TYPE] & 0x7f;
      printf("  type %d\n", headertype);

    switch (pcidev->device_class) {
    case PCI_CLASS_BRIDGE_PCI:
      if (headertype == PCI_HEADER_TYPE_BRIDGE) {
	unsigned char prim,sec,subor;
	assert(PCI_PRIMARY_BUS < CONFIG_SPACE_CACHESIZE);
	assert(PCI_SECONDARY_BUS < CONFIG_SPACE_CACHESIZE);
	assert(PCI_SUBORDINATE_BUS < CONFIG_SPACE_CACHESIZE);
	prim = config_space_cache[PCI_PRIMARY_BUS];
	sec = config_space_cache[PCI_SECONDARY_BUS];
	subor = config_space_cache[PCI_SUBORDINATE_BUS];
	printf("  PCIBridge, buses: primary %hhx secondary %hhx subordinate %hhx\n",
	       prim, sec, subor);
      }
      break;
    case PCI_CLASS_DISPLAY_VGA:
      printf("  GPU\n");
      break;   
    case PCI_CLASS_NETWORK_ETHERNET:
      printf("  NIC\n");
      break;
    case PCI_CLASS_SERIAL_INFINIBAND:
      printf("  InfiniBand\n");
      break;
    case PCI_CLASS_SYSTEM_OTHER:
      printf("  Could be DMA Engine\n");
      break;
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
