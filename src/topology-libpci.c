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

static void
hwloc_pci_traverse(struct hwloc_obj *root, int depth)
{
  struct hwloc_obj *child = root->first_child;
  while (child) {
    char busid[14];
    snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
             child->attr->pcidev.domain, child->attr->pcidev.bus, child->attr->pcidev.dev, child->attr->pcidev.func);

    if (child->type == HWLOC_OBJ_PCI_BRIDGE)
      printf("%*s %s Bridge [%04x:%04x (%04x:%04x) rev=%02x class=%04x] to [%02x:%02x]\n", depth, "", busid,
	     child->attr->pcidev.vendor_id, child->attr->pcidev.device_id,
	     child->attr->pcidev.subvendor_id, child->attr->pcidev.subdevice_id,
	     child->attr->pcidev.revision, child->attr->pcidev.class,
	     child->attr->pcibridge.secondary_bus, child->attr->pcibridge.subordinate_bus);
    else
      printf("%*s %s Device [%04x:%04x (%04x:%04x) rev=%02x class=%04x]\n", depth, "", busid,
	     child->attr->pcidev.vendor_id, child->attr->pcidev.device_id,
	     child->attr->pcidev.subvendor_id, child->attr->pcidev.subdevice_id,
	     child->attr->pcidev.revision, child->attr->pcidev.class);

    hwloc_pci_traverse(child, depth+2);
    child = child->next_sibling;
  }
}

static void
hwloc_pci_set_bridge_depths(struct hwloc_obj *root, int depth)
{
  struct hwloc_obj *child = root->first_child;
  while (child) {
    if (child->type == HWLOC_OBJ_PCI_BRIDGE) {
      child->attr->pcibridge.depth = depth;
      hwloc_pci_set_bridge_depths(child, depth+1);
    }
    child = child->next_sibling;
  }
}

enum hwloc_pci_busid_comparison_e {
  HWLOC_PCI_BUSID_LOWER,
  HWLOC_PCI_BUSID_HIGHER,
  HWLOC_PCI_BUSID_INCLUDED,
  HWLOC_PCI_BUSID_SUPERSET,
};

static int
hwloc_pci_compare_busids(struct hwloc_obj *a, struct hwloc_obj *b)
{
  if (a->attr->pcidev.domain < b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.domain > b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_HIGHER;

  if (a->type == HWLOC_OBJ_PCI_BRIDGE
      && b->attr->pcidev.bus >= a->attr->pcibridge.secondary_bus
      && b->attr->pcidev.bus <= a->attr->pcibridge.subordinate_bus)
    return HWLOC_PCI_BUSID_SUPERSET;
  if (b->type == HWLOC_OBJ_PCI_BRIDGE
      && a->attr->pcidev.bus >= b->attr->pcibridge.secondary_bus
      && a->attr->pcidev.bus <= b->attr->pcibridge.subordinate_bus)
    return HWLOC_PCI_BUSID_INCLUDED;

  if (a->attr->pcidev.bus < b->attr->pcidev.bus)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.bus > b->attr->pcidev.bus)
    return HWLOC_PCI_BUSID_HIGHER;

  if (a->attr->pcidev.dev < b->attr->pcidev.dev)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.dev > b->attr->pcidev.dev)
    return HWLOC_PCI_BUSID_HIGHER;

  if (a->attr->pcidev.func < b->attr->pcidev.func)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.func > b->attr->pcidev.func)
    return HWLOC_PCI_BUSID_HIGHER;

  assert(0);
}

static void
hwloc_pci_add_child_before(struct hwloc_obj *root, struct hwloc_obj *child, struct hwloc_obj *new)
{
  if (child) {
    new->prev_sibling = child->prev_sibling;
    child->prev_sibling = new;
  } else {
    new->prev_sibling = root->last_child;
    root->last_child = new;
  }

  if (new->prev_sibling)
    new->prev_sibling->next_sibling = new;
  else
    root->first_child = new;
  new->next_sibling = child;

  root->arity++;
}

static void
hwloc_pci_remove_child(struct hwloc_obj *root, struct hwloc_obj *child)
{
  if (child->next_sibling)
    child->next_sibling->prev_sibling = child->prev_sibling;
  else
    root->last_child = child->prev_sibling;
  if (child->prev_sibling)
    child->prev_sibling->next_sibling = child->next_sibling;
  else
    root->first_child = child->next_sibling;
  child->prev_sibling = NULL;
  child->next_sibling = NULL;
  root->arity--;
}

static void hwloc_pci_add_object(struct hwloc_obj *root, struct hwloc_obj *new);

static void
hwloc_pci_try_insert_siblings_below_new_bridge(struct hwloc_obj *root, struct hwloc_obj *new)
{
  struct hwloc_obj *current, *next;

  next = new->next_sibling;
  while (next) {
    current = next;
    next = current->next_sibling;

    enum hwloc_pci_busid_comparison_e comp = hwloc_pci_compare_busids(current, new);
    assert(comp != HWLOC_PCI_BUSID_SUPERSET);
    if (comp == HWLOC_PCI_BUSID_HIGHER)
      continue;
    assert(comp == HWLOC_PCI_BUSID_INCLUDED);

    /* move this object below the new bridge */
    hwloc_pci_remove_child(root, current);
    hwloc_pci_add_object(new, current);
  }
}

static void
hwloc_pci_add_object(struct hwloc_obj *root, struct hwloc_obj *new)
{
  struct hwloc_obj *current;

  current = root->first_child;
  while (current) {
    enum hwloc_pci_busid_comparison_e comp = hwloc_pci_compare_busids(new, current);
    switch (comp) {
    case HWLOC_PCI_BUSID_HIGHER:
      /* go further */
      current = current->next_sibling;
      continue;
    case HWLOC_PCI_BUSID_INCLUDED:
      /* insert below current bridge */
      hwloc_pci_add_object(current, new);
      return;
    case HWLOC_PCI_BUSID_LOWER:
    case HWLOC_PCI_BUSID_SUPERSET:
      /* insert before current object */
      hwloc_pci_add_child_before(root, current, new);
      /* walk next siblings and move them below new bridge if needed */
      hwloc_pci_try_insert_siblings_below_new_bridge(root, new);
      return;
    }
  }
  /* add to the end of the list if higher than everybody */
  hwloc_pci_add_child_before(root, NULL, new);
}

void
hwloc_look_libpci(struct hwloc_topology *topology)
{
  struct pci_access *pciaccess;
  struct pci_dev *pcidev;
  struct hwloc_obj *hostbridge = hwloc_alloc_setup_object(HWLOC_OBJ_PCI_BRIDGE, 0);

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
    HWLOC_BUILD_ASSERT(PCI_REVISION_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.revision = config_space_cache[PCI_REVISION_ID];
    HWLOC_BUILD_ASSERT(PCI_SUBSYSTEM_VENDOR_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.subvendor_id = config_space_cache[PCI_SUBSYSTEM_VENDOR_ID];
    HWLOC_BUILD_ASSERT(PCI_SUBSYSTEM_ID < CONFIG_SPACE_CACHESIZE);
    obj->attr->pcidev.subdevice_id = config_space_cache[PCI_SUBSYSTEM_ID];

    HWLOC_BUILD_ASSERT(PCI_HEADER_TYPE < CONFIG_SPACE_CACHESIZE);
    headertype = config_space_cache[PCI_HEADER_TYPE] & 0x7f;

    if (pcidev->device_class == PCI_CLASS_BRIDGE_PCI
      && headertype == PCI_HEADER_TYPE_BRIDGE) {
      HWLOC_BUILD_ASSERT(PCI_PRIMARY_BUS < CONFIG_SPACE_CACHESIZE);
      HWLOC_BUILD_ASSERT(PCI_SECONDARY_BUS < CONFIG_SPACE_CACHESIZE);
      HWLOC_BUILD_ASSERT(PCI_SUBORDINATE_BUS < CONFIG_SPACE_CACHESIZE);
      assert(config_space_cache[PCI_PRIMARY_BUS] == pcidev->bus);
      obj->type = HWLOC_OBJ_PCI_BRIDGE;
      obj->attr->pcibridge.secondary_bus = config_space_cache[PCI_SECONDARY_BUS];
      obj->attr->pcibridge.subordinate_bus = config_space_cache[PCI_SUBORDINATE_BUS];
    }

    strcpy(name, "??");
    pci_lookup_name(pciaccess, name, sizeof(name),
                    PCI_LOOKUP_VENDOR|PCI_LOOKUP_DEVICE|PCI_LOOKUP_NO_NUMBERS,
                    pcidev->vendor_id, pcidev->device_id);
    printf("  %s\n", name);

#ifdef LINUX_SYS
    snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
             pcidev->domain, pcidev->bus, pcidev->dev, pcidev->func);
    snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/local_cpus", busid);
    fd = open(path, O_RDONLY);
    read(fd, localcpus, sizeof(localcpus));
    close(fd);
    printf("  Cpuset %s\n", localcpus);
#endif

    hwloc_pci_add_object(hostbridge, obj);

    pcidev = pcidev->next;
  }

  hwloc_insert_object(topology, topology->levels[0][0], hostbridge);

  pci_cleanup(pciaccess);

  /* walk the hierarchy and set bridge depth */
  hwloc_pci_set_bridge_depths(&hostbridge, 0);

  /* just print the hierarchy for now */
  hwloc_pci_traverse(hostbridge, 0);
}

#endif /* HAVE_LIBPCI */
