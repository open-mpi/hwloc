/*
 * Copyright © 2009-2015 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <hwloc/plugins.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HWLOC_DEBUG
static void
hwloc_pci_traverse_print_cb(void * cbdata __hwloc_attribute_unused,
			    struct hwloc_obj *pcidev)
{
  char busid[14];
  hwloc_obj_t parent;

  /* indent */
  parent = pcidev->parent;
  while (parent) {
    hwloc_debug("%s", "  ");
    parent = parent->parent;
  }

  snprintf(busid, sizeof(busid), "%04x:%02x:%02x.%01x",
           pcidev->attr->pcidev.domain, pcidev->attr->pcidev.bus, pcidev->attr->pcidev.dev, pcidev->attr->pcidev.func);

  if (pcidev->type == HWLOC_OBJ_BRIDGE) {
    if (pcidev->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_HOST)
      hwloc_debug("HostBridge");
    else
      hwloc_debug("Bridge [%04x:%04x]", busid,
		  pcidev->attr->pcidev.vendor_id, pcidev->attr->pcidev.device_id);
    hwloc_debug(" to %04x:[%02x:%02x]\n",
		pcidev->attr->bridge.downstream.pci.domain, pcidev->attr->bridge.downstream.pci.secondary_bus, pcidev->attr->bridge.downstream.pci.subordinate_bus);
  } else
    hwloc_debug("%s Device [%04x:%04x (%04x:%04x) rev=%02x class=%04x]\n", busid,
		pcidev->attr->pcidev.vendor_id, pcidev->attr->pcidev.device_id,
		pcidev->attr->pcidev.subvendor_id, pcidev->attr->pcidev.subdevice_id,
		pcidev->attr->pcidev.revision, pcidev->attr->pcidev.class_id);
}

static void
hwloc_pci__traverse(void * cbdata, struct hwloc_obj *tree,
		    void (*cb)(void * cbdata, struct hwloc_obj *))
{
  struct hwloc_obj *child = tree;
  while (child) {
    cb(cbdata, child);
    if (child->type == HWLOC_OBJ_BRIDGE && child->io_first_child)
      hwloc_pci__traverse(cbdata, child->io_first_child, cb);
    child = child->next_sibling;
  }
}

static void
hwloc_pci_traverse(void * cbdata, struct hwloc_obj *tree,
		   void (*cb)(void * cbdata, struct hwloc_obj *))
{
  hwloc_pci__traverse(cbdata, tree, cb);
}
#endif /* HWLOC_DEBUG */

enum hwloc_pci_busid_comparison_e {
  HWLOC_PCI_BUSID_LOWER,
  HWLOC_PCI_BUSID_HIGHER,
  HWLOC_PCI_BUSID_INCLUDED,
  HWLOC_PCI_BUSID_SUPERSET
};

static enum hwloc_pci_busid_comparison_e
hwloc_pci_compare_busids(struct hwloc_obj *a, struct hwloc_obj *b)
{
  if (a->type == HWLOC_OBJ_BRIDGE)
    assert(a->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI);
  if (b->type == HWLOC_OBJ_BRIDGE)
    assert(b->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI);

  if (a->attr->pcidev.domain < b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.domain > b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_HIGHER;

  if (a->type == HWLOC_OBJ_BRIDGE
      && b->attr->pcidev.bus >= a->attr->bridge.downstream.pci.secondary_bus
      && b->attr->pcidev.bus <= a->attr->bridge.downstream.pci.subordinate_bus)
    return HWLOC_PCI_BUSID_SUPERSET;
  if (b->type == HWLOC_OBJ_BRIDGE
      && a->attr->pcidev.bus >= b->attr->bridge.downstream.pci.secondary_bus
      && a->attr->pcidev.bus <= b->attr->bridge.downstream.pci.subordinate_bus)
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

  /* Should never reach here.  Abort on both debug builds and
     non-debug builds */
  assert(0);
  fprintf(stderr, "Bad assertion in hwloc %s:%d (aborting)\n", __FILE__, __LINE__);
  exit(1);
}

static void
hwloc_pci_add_object(struct hwloc_obj *parent, struct hwloc_obj **parent_io_first_child_p, struct hwloc_obj *new)
{
  struct hwloc_obj **curp, **childp;

  curp = parent_io_first_child_p;
  while (*curp) {
    enum hwloc_pci_busid_comparison_e comp = hwloc_pci_compare_busids(new, *curp);
    switch (comp) {
    case HWLOC_PCI_BUSID_HIGHER:
      /* go further */
      curp = &(*curp)->next_sibling;
      continue;
    case HWLOC_PCI_BUSID_INCLUDED:
      /* insert new below current bridge */
      hwloc_pci_add_object(*curp, &(*curp)->io_first_child, new);
      return;
    case HWLOC_PCI_BUSID_LOWER:
    case HWLOC_PCI_BUSID_SUPERSET: {
      /* insert new before current */
      new->next_sibling = *curp;
      *curp = new;
      new->parent = parent;
      if (new->type == HWLOC_OBJ_BRIDGE) {
	/* look at remaining siblings and move some below new */
	childp = &new->io_first_child;
	curp = &new->next_sibling;
	while (*curp) {
	  if (hwloc_pci_compare_busids(new, *curp) == HWLOC_PCI_BUSID_LOWER) {
	    /* this sibling remains under root, after new */
	    curp = &(*curp)->next_sibling;
	    /* even if the list is sorted by busid, we can't break because the current bridge creates a bus that may be higher. some object may have to go there */
	  } else {
	    /* this sibling goes under new */
	    *childp = *curp;
	    *curp = (*curp)->next_sibling;
	    (*childp)->parent = new;
	    (*childp)->next_sibling = NULL;
	    childp = &(*childp)->next_sibling;
	  }
	}
      }
      return;
    }
    }
  }
  /* add to the end of the list if higher than everybody */
  new->parent = parent;
  new->next_sibling = NULL;
  *curp = new;
}

void
hwloc_pci_tree_insert_by_busid(struct hwloc_obj **treep,
			       struct hwloc_obj *obj)
{
  hwloc_pci_add_object(NULL /* no parent on top of tree */, treep, obj);
}

int
hwloc_pci_tree_attach_belowroot(struct hwloc_topology *topology, struct hwloc_obj *old_tree)
{
  struct hwloc_obj **next_hb_p;
  unsigned current_hostbridge;

  if (!old_tree)
    /* found nothing, exit */
    return 0;

#ifdef HWLOC_DEBUG
  hwloc_debug("%s", "\nPCI hierarchy:\n");
  hwloc_pci_traverse(NULL, old_tree, hwloc_pci_traverse_print_cb);
  hwloc_debug("%s", "\n");
#endif

  next_hb_p = &hwloc_get_root_obj(topology)->io_first_child;
  while (*next_hb_p)
    next_hb_p = &((*next_hb_p)->next_sibling);

  /*
   * tree points to all objects connected to any upstream bus in the machine.
   * We now create one real hostbridge object per upstream bus.
   * It's not actually a PCI device so we have to create it.
   */
  current_hostbridge = 0;
  while (old_tree) {
    /* start a new host bridge */
    struct hwloc_obj *hostbridge = hwloc_alloc_setup_object(HWLOC_OBJ_BRIDGE, current_hostbridge++);
    struct hwloc_obj **dstnextp = &hostbridge->io_first_child;
    struct hwloc_obj **srcnextp = &old_tree;
    struct hwloc_obj *child = *srcnextp;
    unsigned short current_domain = child->attr->pcidev.domain;
    unsigned char current_bus = child->attr->pcidev.bus;
    unsigned char current_subordinate = current_bus;

    hwloc_debug("Starting new PCI hostbridge %04x:%02x\n", current_domain, current_bus);

  next_child:
    /* remove next child from tree */
    *srcnextp = child->next_sibling;
    /* append it to hostbridge */
    *dstnextp = child;
    child->parent = hostbridge;
    child->next_sibling = NULL;
    dstnextp = &child->next_sibling;

    /* compute hostbridge secondary/subordinate buses */
    if (child->type == HWLOC_OBJ_BRIDGE
	&& child->attr->bridge.downstream.pci.subordinate_bus > current_subordinate)
      current_subordinate = child->attr->bridge.downstream.pci.subordinate_bus;

    /* use next child if it has the same domains/bus */
    child = *srcnextp;
    if (child
	&& child->attr->pcidev.domain == current_domain
	&& child->attr->pcidev.bus == current_bus)
      goto next_child;

    /* finish setting up this hostbridge */
    hostbridge->attr->bridge.upstream_type = HWLOC_OBJ_BRIDGE_HOST;
    hostbridge->attr->bridge.downstream_type = HWLOC_OBJ_BRIDGE_PCI;
    hostbridge->attr->bridge.downstream.pci.domain = current_domain;
    hostbridge->attr->bridge.downstream.pci.secondary_bus = current_bus;
    hostbridge->attr->bridge.downstream.pci.subordinate_bus = current_subordinate;
    hwloc_debug("New PCI hostbridge %04x:[%02x-%02x]\n",
		current_domain, current_bus, current_subordinate);

    *next_hb_p = hostbridge;
    next_hb_p = &hostbridge->next_sibling;
    topology->modified = 1; /* needed in case somebody reconnects levels before the core calls hwloc_pci_belowroot_apply_locality()
			     * or if hwloc_pci_belowroot_apply_locality() keeps hostbridges below root.
			     */
  }

  return 1;
}

static struct hwloc_obj *
hwloc_pci_fixup_busid_parent(struct hwloc_topology *topology __hwloc_attribute_unused,
			     struct hwloc_pcidev_attr_s *busid,
			     struct hwloc_obj *parent)
{
  /* Xeon E5v3 in cluster-on-die mode only have PCI on the first NUMA node of each package.
   * but many dual-processor host report the second PCI hierarchy on 2nd NUMA of first package.
   */
  if (parent->depth >= 2
      && parent->type == HWLOC_OBJ_NUMANODE
      && parent->sibling_rank == 1 && parent->parent->arity == 2
      && parent->parent->type == HWLOC_OBJ_PACKAGE
      && parent->parent->sibling_rank == 0 && parent->parent->parent->arity == 2) {
    const char *cpumodel = hwloc_obj_get_info_by_name(parent->parent, "CPUModel");
    if (cpumodel && strstr(cpumodel, "Xeon")) {
      if (!hwloc_hide_errors()) {
	fprintf(stderr, "****************************************************************************\n");
	fprintf(stderr, "* hwloc %s has encountered an incorrect PCI locality information.\n", HWLOC_VERSION);
	fprintf(stderr, "* PCI bus %04x:%02x is supposedly close to 2nd NUMA node of 1st package,\n",
		busid->domain, busid->bus);
	fprintf(stderr, "* however hwloc believes this is impossible on this architecture.\n");
	fprintf(stderr, "* Therefore the PCI bus will be moved to 1st NUMA node of 2nd package.\n");
	fprintf(stderr, "*\n");
	fprintf(stderr, "* If you feel this fixup is wrong, disable it by setting in your environment\n");
	fprintf(stderr, "* HWLOC_PCI_%04x_%02x_LOCALCPUS= (empty value), and report the problem\n",
		busid->domain, busid->bus);
	fprintf(stderr, "* to the hwloc's user mailing list together with the XML output of lstopo.\n");
	fprintf(stderr, "*\n");
	fprintf(stderr, "* You may silence this message by setting HWLOC_HIDE_ERRORS=1 in your environment.\n");
	fprintf(stderr, "****************************************************************************\n");
      }
      return parent->parent->next_sibling->first_child;
    }
  }

  return parent;
}

static struct hwloc_obj *
hwloc__pci_find_busid_parent(struct hwloc_topology *topology, struct hwloc_pcidev_attr_s *busid)
{
  hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
  hwloc_obj_t parent;
  const char *env;
  int forced = 0;
  char envname[256];
  int err;

  /* override the cpuset with the environment if given */
  snprintf(envname, sizeof(envname), "HWLOC_PCI_%04x_%02x_LOCALCPUS",
	   busid->domain, busid->bus);
  env = getenv(envname);
  if (env)
    /* if env exists but is empty, don't let quirks change what the OS reports */
    forced = 1;
  if (env && *env) {
    /* force the cpuset */
    hwloc_debug("Overriding localcpus using %s in the environment\n", envname);
    hwloc_bitmap_sscanf(cpuset, env);
  } else {
    /* get the cpuset by asking the OS backend. */
    err = hwloc_backends_get_pci_busid_cpuset(topology, busid, cpuset);
    if (err < 0)
      /* if we got nothing, assume this PCI bus is attached to the top of hierarchy */
      hwloc_bitmap_copy(cpuset, hwloc_topology_get_topology_cpuset(topology));
  }

  hwloc_debug_bitmap("Attaching PCI tree to cpuset %s\n", cpuset);

  parent = hwloc_find_insert_io_parent_by_complete_cpuset(topology, cpuset);
  if (parent) {
    if (!forced)
      /* We found a valid parent. Check that the OS didn't report invalid locality */
      parent = hwloc_pci_fixup_busid_parent(topology, busid, parent);
  } else {
    /* Fallback to root */
    parent = hwloc_get_root_obj(topology);
  }

  hwloc_bitmap_free(cpuset);
  return parent;
}

struct hwloc_obj *
hwloc_pci_find_busid_parent(struct hwloc_topology *topology,
			    unsigned domain, unsigned bus, unsigned dev, unsigned func)
{
  struct hwloc_pcidev_attr_s busid;
  busid.domain = domain;
  busid.bus = bus;
  busid.dev = dev;
  busid.func = func;
  return hwloc__pci_find_busid_parent(topology, &busid);
}

int
hwloc_pci_belowroot_apply_locality(struct hwloc_topology *topology)
{
  struct hwloc_obj *root = hwloc_get_root_obj(topology);
  struct hwloc_obj **listp, *obj;

  /* root->io_first_child contains some PCI hierarchies, any maybe some non-PCI things.
   * insert the PCI trees according to their PCI-locality.
   */
  listp = &root->io_first_child;
  while ((obj = *listp) != NULL) {
    struct hwloc_pcidev_attr_s *busid;
    struct hwloc_obj *parent;

    /* skip non-PCI objects */
    if (obj->type != HWLOC_OBJ_PCI_DEVICE
	&& !(obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI)
	&& !(obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)) {
      listp = &obj->next_sibling;
      continue;
    }

    /* don't support PCI devices without hostbridges yet */
    assert(obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI);

    if (obj->type == HWLOC_OBJ_PCI_DEVICE
	|| (obj->type == HWLOC_OBJ_BRIDGE
	    && obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI))
      busid = &obj->attr->pcidev;
    else
      /* hostbridges don't have a PCI busid for looking up locality */
      busid = &obj->io_first_child->attr->pcidev;

    /* attach the object (and children) where it belongs */
    parent = hwloc__pci_find_busid_parent(topology, busid);
    if (parent == root) {
      /* keep this object here */
      listp = &obj->next_sibling;
    } else {
      /* dequeue this object */
      *listp = obj->next_sibling;
      obj->next_sibling = NULL;
      hwloc_insert_object_by_parent(topology, parent, obj);
    }
  }

  return 0;
}

static struct hwloc_obj *
hwloc__pci_belowroot_find_by_busid(hwloc_obj_t parent,
				   unsigned domain, unsigned bus, unsigned dev, unsigned func)
{
  hwloc_obj_t child = parent->io_first_child;

  for ( ; child; child = child->next_sibling) {
    if (child->type == HWLOC_OBJ_PCI_DEVICE
	|| (child->type == HWLOC_OBJ_BRIDGE
	    && child->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)) {
      if (child->attr->pcidev.domain == domain
	  && child->attr->pcidev.bus == bus
	  && child->attr->pcidev.dev == dev
	  && child->attr->pcidev.func == func)
	/* that's the right bus id */
	return child;
      if (child->attr->pcidev.domain > domain
	  || (child->attr->pcidev.domain == domain
	      && child->attr->pcidev.bus > bus))
	/* bus id too high, won't find anything later, return parent */
	return parent;
      if (child->type == HWLOC_OBJ_BRIDGE
	  && child->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
	  && child->attr->bridge.downstream.pci.domain == domain
	  && child->attr->bridge.downstream.pci.secondary_bus <= bus
	  && child->attr->bridge.downstream.pci.subordinate_bus >= bus)
	/* not the right bus id, but it's included in the bus below that bridge */
	return hwloc__pci_belowroot_find_by_busid(child, domain, bus, dev, func);

    } else if (child->type == HWLOC_OBJ_BRIDGE
	       && child->attr->bridge.upstream_type != HWLOC_OBJ_BRIDGE_PCI
	       && child->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
	       /* non-PCI to PCI bridge, just look at the subordinate bus */
	       && child->attr->bridge.downstream.pci.domain == domain
	       && child->attr->bridge.downstream.pci.secondary_bus <= bus
	       && child->attr->bridge.downstream.pci.subordinate_bus >= bus) {
      /* contains our bus, recurse */
      return hwloc__pci_belowroot_find_by_busid(child, domain, bus, dev, func);
    }
  }
  /* didn't find anything, return parent */
  return parent;
}

struct hwloc_obj *
hwloc_pci_belowroot_find_by_busid(struct hwloc_topology *topology,
				  unsigned domain, unsigned bus, unsigned dev, unsigned func)
{
  hwloc_obj_t root = hwloc_get_root_obj(topology);
  hwloc_obj_t parent = hwloc__pci_belowroot_find_by_busid(root, domain, bus, dev, func);
  if (parent == root)
    return NULL;
  else
    return parent;
}

#define HWLOC_PCI_STATUS 0x06
#define HWLOC_PCI_STATUS_CAP_LIST 0x10
#define HWLOC_PCI_CAPABILITY_LIST 0x34
#define HWLOC_PCI_CAP_LIST_ID 0
#define HWLOC_PCI_CAP_LIST_NEXT 1

unsigned
hwloc_pci_find_cap(const unsigned char *config, unsigned cap)
{
  unsigned char seen[256] = { 0 };
  unsigned char ptr; /* unsigned char to make sure we stay within the 256-byte config space */

  if (!(config[HWLOC_PCI_STATUS] & HWLOC_PCI_STATUS_CAP_LIST))
    return 0;

  for (ptr = config[HWLOC_PCI_CAPABILITY_LIST] & ~3;
       ptr; /* exit if next is 0 */
       ptr = config[ptr + HWLOC_PCI_CAP_LIST_NEXT] & ~3) {
    unsigned char id;

    /* Looped around! */
    if (seen[ptr])
      break;
    seen[ptr] = 1;

    id = config[ptr + HWLOC_PCI_CAP_LIST_ID];
    if (id == cap)
      return ptr;
    if (id == 0xff) /* exit if id is 0 or 0xff */
      break;
  }
  return 0;
}

#define HWLOC_PCI_EXP_LNKSTA 0x12
#define HWLOC_PCI_EXP_LNKSTA_SPEED 0x000f
#define HWLOC_PCI_EXP_LNKSTA_WIDTH 0x03f0

int
hwloc_pci_find_linkspeed(const unsigned char *config,
			 unsigned offset, float *linkspeed)
{
  unsigned linksta, speed, width;
  float lanespeed;

  memcpy(&linksta, &config[offset + HWLOC_PCI_EXP_LNKSTA], 4);
  speed = linksta & HWLOC_PCI_EXP_LNKSTA_SPEED; /* PCIe generation */
  width = (linksta & HWLOC_PCI_EXP_LNKSTA_WIDTH) >> 4; /* how many lanes */
  /* PCIe Gen1 = 2.5GT/s signal-rate per lane with 8/10 encoding    = 0.25GB/s data-rate per lane
   * PCIe Gen2 = 5  GT/s signal-rate per lane with 8/10 encoding    = 0.5 GB/s data-rate per lane
   * PCIe Gen3 = 8  GT/s signal-rate per lane with 128/130 encoding = 1   GB/s data-rate per lane
   */
  lanespeed = speed <= 2 ? 2.5 * speed * 0.8 : 8.0 * 128/130; /* Gbit/s per lane */
  *linkspeed = lanespeed * width / 8; /* GB/s */
  return 0;
}

#define HWLOC_PCI_HEADER_TYPE 0x0e
#define HWLOC_PCI_HEADER_TYPE_BRIDGE 1
#define HWLOC_PCI_CLASS_BRIDGE_PCI 0x0604
#define HWLOC_PCI_PRIMARY_BUS 0x18
#define HWLOC_PCI_SECONDARY_BUS 0x19
#define HWLOC_PCI_SUBORDINATE_BUS 0x1a

int
hwloc_pci_prepare_bridge(hwloc_obj_t obj,
			 const unsigned char *config)
{
  unsigned char headertype;
  unsigned isbridge;
  struct hwloc_pcidev_attr_s *pattr = &obj->attr->pcidev;
  struct hwloc_bridge_attr_s *battr;

  headertype = config[HWLOC_PCI_HEADER_TYPE] & 0x7f;
  isbridge = (pattr->class_id == HWLOC_PCI_CLASS_BRIDGE_PCI
	      && headertype == HWLOC_PCI_HEADER_TYPE_BRIDGE);

  if (!isbridge)
    return 0;

  battr = &obj->attr->bridge;

  if (config[HWLOC_PCI_PRIMARY_BUS] != pattr->bus)
    hwloc_debug("  %04x:%02x:%02x.%01x bridge with (ignored) invalid PCI_PRIMARY_BUS %02x\n",
		pattr->domain, pattr->bus, pattr->dev, pattr->func, config[HWLOC_PCI_PRIMARY_BUS]);

  obj->type = HWLOC_OBJ_BRIDGE;
  battr->upstream_type = HWLOC_OBJ_BRIDGE_PCI;
  battr->downstream_type = HWLOC_OBJ_BRIDGE_PCI;
  battr->downstream.pci.domain = pattr->domain;
  battr->downstream.pci.secondary_bus = config[HWLOC_PCI_SECONDARY_BUS];
  battr->downstream.pci.subordinate_bus = config[HWLOC_PCI_SUBORDINATE_BUS];

  return 0;
}
