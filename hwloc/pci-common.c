/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2009-2026 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "hwloc/plugins.h"
#include "private/private.h"
#include "private/debug.h"
#include "private/misc.h"

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>

#if defined(HWLOC_WIN_SYS) && !defined(__CYGWIN__)
#include <io.h>
#define open _open
#define read _read
#define close _close
#endif



/******************************
 * Finding/Inserting IO parent
 */

/* if an object exists with this cpuset, return the highest one,
 * or create a better group.
 * if the cpuset is empty, return root.
 */
static hwloc_obj_t
hwloc__pci_find_insert_io_parent_by_cpuset(struct hwloc_topology *topology, hwloc_cpuset_t cpuset)
{
  hwloc_obj_t group_obj, largeparent, parent;

  if (hwloc_bitmap_iszero(cpuset))
    return hwloc_get_root_obj(topology);

  /* find the smallest object covering the cpuset */
  largeparent = hwloc_get_obj_covering_cpuset(topology, cpuset);

  /* if we found the exact cpuset, we're good.
   * if not and we cannot insert a better group, done too.
   */
  if (hwloc_bitmap_isequal(largeparent->cpuset, cpuset)
      || !hwloc_filter_check_keep_object_type(topology, HWLOC_OBJ_GROUP)) {
    /* walk up identical parents to return the highest one */
    while (largeparent->parent && largeparent->parent->arity == 1)
      largeparent = largeparent->parent;
    return largeparent;
  }

  /* we need to insert an intermediate group */
  group_obj = hwloc_alloc_setup_object(topology, HWLOC_OBJ_GROUP, HWLOC_UNKNOWN_INDEX);
  if (!group_obj)
    /* Failed to insert the exact Group, fallback to largeparent */
    return largeparent;

  group_obj->cpuset = hwloc_bitmap_dup(cpuset);
  hwloc_bitmap_and(cpuset, cpuset, hwloc_topology_get_topology_cpuset(topology));
  group_obj->cpuset = hwloc_bitmap_dup(cpuset);
  group_obj->attr->group.kind = HWLOC_GROUP_KIND_IO;
  parent = hwloc__insert_object_by_cpuset(topology, largeparent, group_obj, "topology:io_parent");
  if (!parent)
    /* Failed to insert the Group, maybe a conflicting cpuset */
    return largeparent;

  /* Group couldn't get merged or we would have gotten the right largeparent earlier */
  assert(parent == group_obj);

  /* Group inserted without being merged, everything OK, setup its sets */
  hwloc_obj_add_children_sets(group_obj);

  return parent;
}


/****************
 * Debug Helpers
 */

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
      hwloc_debug("%s Bridge [%04x:%04x]", busid,
		  pcidev->attr->pcidev.vendor_id, pcidev->attr->pcidev.device_id);
    if (pcidev->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI)
      hwloc_debug(" to %04x:[%02x:%02x]\n",
                  pcidev->attr->bridge.downstream.pci.domain, pcidev->attr->bridge.downstream.pci.secondary_bus, pcidev->attr->bridge.downstream.pci.subordinate_bus);
    else
      assert(0);
  } else
    hwloc_debug("%s Device [%04x:%04x (%04x:%04x) rev=%02x class=%04x]\n", busid,
		pcidev->attr->pcidev.vendor_id, pcidev->attr->pcidev.device_id,
		pcidev->attr->pcidev.subvendor_id, pcidev->attr->pcidev.subdevice_id,
		pcidev->attr->pcidev.revision, pcidev->attr->pcidev.class_id);
}

static void
hwloc_pci_traverse(void * cbdata, struct hwloc_obj *tree,
		   void (*cb)(void * cbdata, struct hwloc_obj *))
{
  hwloc_obj_t child;
  cb(cbdata, tree);
  for_each_io_child(child, tree) {
    if (child->type == HWLOC_OBJ_BRIDGE)
      hwloc_pci_traverse(cbdata, child, cb);
  }
}

static void
hwloc_pcidebug_show_tree(const char *string, struct hwloc_obj *tree)
{
  hwloc_debug("\n%s:\n", string);
  hwloc_pci_traverse(NULL, tree, hwloc_pci_traverse_print_cb);
  hwloc_debug("\n");
}

static void
hwloc_pcidebug_show_localities(const char *string, struct hwloc_topology *topology, int checkorder)
{
  struct hwloc_pci_locality_s *tmp;
  hwloc_debug("\n%s:\n", string);
  for(tmp = topology->first_pci_locality; tmp; tmp=tmp->next) {
    char busid[12];
    snprintf(busid, sizeof(busid), "%04x:%02x-%02x", tmp->domain, tmp->bus_min, tmp->bus_max);
    hwloc_debug_1arg_bitmap("    %s = %s\n", busid, tmp->cpuset);
    if (checkorder) {
      /* check order and no overlapping */
      if (tmp->prev)
        assert(tmp->domain > tmp->prev->domain
               || (tmp->domain == tmp->prev->domain && tmp->bus_min > tmp->prev->bus_max));
    }
  }
  hwloc_debug("\n");
}
#else
#define hwloc_pcidebug_show_tree(string, tree) do { /* nothing */ } while (0);
#define hwloc_pcidebug_show_localities(string, topology, checkorder) do { /* nothing */ } while (0); 
#endif


/************************
 * Locality List Helpers
 */

static void
hwloc___pci_insert_locality_before(struct hwloc_topology *topology,
                                   struct hwloc_pci_locality_s *new,
                                   struct hwloc_pci_locality_s *next)
{
  new->prev = next ? next->prev : topology->last_pci_locality;
  new->next = next;
  if (next) {
    /* insert before next */
    if (next->prev)
      next->prev->next = new;
    else
      topology->first_pci_locality = new;
    next->prev = new;
  } else {
    /* insert last */
    if (topology->last_pci_locality)
      topology->last_pci_locality->next = new;
    else
      topology->first_pci_locality = new;
    topology->last_pci_locality = new;
  }
}

static struct hwloc_pci_locality_s *
hwloc___pci_add_locality_before(struct hwloc_topology *topology,
                                unsigned domain,
                                unsigned bus_min, unsigned bus_max,
                                hwloc_bitmap_t cpuset,
                                hwloc_obj_t parent,
                                struct hwloc_pci_locality_s *next)
{
  struct hwloc_pci_locality_s *new;

  assert(cpuset);
  /* parent may be NULL on XML import */

  new = malloc(sizeof *new);
  if (!new) {
    hwloc_bitmap_free(cpuset);
    return NULL;
  }

  new->domain = domain;
  new->bus_min = bus_min;
  new->bus_max = bus_max;
  new->cpuset = cpuset;
  new->parent = parent;
  hwloc___pci_insert_locality_before(topology, new, next);
  return new;
}

/* parent is mandatory */
static struct hwloc_pci_locality_s *
hwloc__pci_add_locality_before(struct hwloc_topology *topology,
                               unsigned domain,
                               unsigned bus_min, unsigned bus_max,
                               hwloc_bitmap_t cpuset,
                               hwloc_obj_t parent,
                               struct hwloc_pci_locality_s *next)
{
  assert(parent);
  return hwloc___pci_add_locality_before(topology, domain, bus_min, bus_max, cpuset, parent, next);
}

static void
hwloc__pci_remove_locality(struct hwloc_topology *topology,
                           struct hwloc_pci_locality_s *old)
{
  if (old->prev)
    old->prev->next = old->next;
  else
    topology->first_pci_locality = old->next;
  if (old->next)
    old->next->prev = old->prev;
  else
    topology->last_pci_locality = old->prev;
  free(old);
}

static void
hwloc__pci_merge_next_localities(struct hwloc_topology *topology,
                                 struct hwloc_pci_locality_s *new)
{
  while (new->next) {
    struct hwloc_pci_locality_s *next = new->next;
    if (next->domain == new->domain && next->bus_min <= new->bus_max) {
      if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_PCI|HWLOC_SHOWMSG_CRITICAL))
        fprintf(stderr, "[hwloc/pci] Merging PCI localities %04x:%02x-%02x into previous overlapping %04x:%02x-%02x even if the IO parents are different\n",
                next->domain, next->bus_min, next->bus_max,
                new->domain, new->bus_min, new->bus_max);
      if (new->bus_max < next->bus_max)
        new->bus_max = next->bus_max;
      hwloc__pci_remove_locality(topology, next);
    } else
      break;
  }
}

/* either duplicate the parent cpuset, or use the given cpuset.
 * parent is not saved since it'll be refreshed later
 */
void
hwloc_pci_xml_import_locality(struct hwloc_topology *topology,
                              unsigned domain,
                              unsigned bus_min, unsigned bus_max,
                              hwloc_obj_t parent,
                              hwloc_cpuset_t cpuset)
{
  if (parent) {
    assert(!cpuset);
    /* only parent was given, use a copy of its cpuset */
    cpuset = hwloc_bitmap_dup(parent->cpuset);
    if (!cpuset)
      return;
  } else {
    /* only a cpuset was given, keep it */
    assert(cpuset);
  }

  hwloc_bitmap_and(cpuset, cpuset, hwloc_topology_get_topology_cpuset(topology));
  hwloc___pci_add_locality_before(topology, domain, bus_min, bus_max, cpuset, NULL, NULL);
}

void
hwloc_pci_xml_import_refresh_localities(struct hwloc_topology *topology)
{
  struct hwloc_pci_locality_s *cur = topology->first_pci_locality;
  while (cur) {
    struct hwloc_pci_locality_s *next = cur->next;
    if (next
        && (next->domain < cur->domain
            || (next->domain == cur->domain && next->bus_min <= cur->bus_max))) {
      /* next isn't strictly after cur, drop it */
      static unsigned warned = 0;
      if (!warned && HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_XML)) {
        fprintf(stderr, "hwloc/xml/pci: overlapping pci localities are ignored.\n");
        warned = 1;
      }
      if (next->next)
        next->next->prev = cur;
      else
        topology->last_pci_locality = cur;
      cur->next = next->next;
      hwloc_bitmap_free(next->cpuset);
      free(next);
    } else {
      cur = cur->next;
    }
  }
  hwloc_pci_refresh(topology);
}

static struct hwloc_pci_locality_s *
hwloc__pci_find_bus_locality(struct hwloc_topology *topology,
                             unsigned domain, unsigned bus)
{
  struct hwloc_pci_locality_s *loc;
  hwloc_debug("pcidisc looking for bus %04x:%02x\n", domain, bus);
  loc = topology->first_pci_locality;
  while (loc) {
    if (loc->domain == domain && loc->bus_min <= bus && loc->bus_max >= bus) {
      hwloc_debug("  found pci locality for %04x:[%02x:%02x]\n",
		  loc->domain, loc->bus_min, loc->bus_max);
      return loc;
    }
    loc = loc->next;
  }
  return NULL;
}

/* find the first locality that isn't strictly before the given bus,
 * starting from prev if any
 */
static struct hwloc_pci_locality_s*
hwloc__pci_find_locality_notbefore_bus(struct hwloc_topology *topology,
                                       unsigned domain, unsigned bus,
                                       struct hwloc_pci_locality_s* prev)
{
  struct hwloc_pci_locality_s *loc;
  loc = prev ? prev : topology->first_pci_locality;
  while (loc &&
         (loc->domain < domain
          || (loc->domain == domain && loc->bus_max < bus)))
    loc = loc->next;
  return loc;
}

/**************************************
 * Init/Exit and Forced PCI localities
 */

static void
hwloc_pci_forced_locality_parse_one(struct hwloc_topology *topology,
				    const char *string /* must contain a ' ' */)
{
  struct hwloc_pci_locality_s *next;
  unsigned domain, bus_first, bus_last, dummy;
  hwloc_bitmap_t set;
  hwloc_obj_t parent;
  char *tmp;

  if (sscanf(string, "%x:%x-%x %x", &domain, &bus_first, &bus_last, &dummy) == 4) {
    /* fine */
  } else if (sscanf(string, "%x:%x %x", &domain, &bus_first, &dummy) == 3) {
    bus_last = bus_first;
  } else if (sscanf(string, "%x %x", &domain, &dummy) == 2) {
    bus_first = 0;
    bus_last = 255;
  } else {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_CRITICAL|HWLOC_SHOWMSG_PCI|HWLOC_SHOWMSG_USER))
      fprintf(stderr, "hwloc/pci: Ignoring unparseable HWLOC_PCI_LOCALITY line `%s'\n",
              string);
    return;
  }

  /* find the first locality that isn't strictly before us */
  next = hwloc__pci_find_locality_notbefore_bus(topology, domain, bus_first, NULL);

  /* next isn't before us, check if it intersects */
  if (next && next->domain == domain
      && (next->bus_min <= bus_last || next->bus_max <= bus_first)) {
    if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_CRITICAL|HWLOC_SHOWMSG_PCI|HWLOC_SHOWMSG_USER))
      fprintf(stderr, "hwloc/pci: Ignoring HWLOC_PCI_LOCALITY line `%s', intersects with previous ones\n",
              string);
    return;
  }

  /* parse the cpuset */
  tmp = strchr(string, ' ');
  if (!tmp) /* things like c7-c8... match %x %x in the 3rd case above?! */
    return;
  tmp++;

  set = hwloc_bitmap_alloc();
  if (!set)
    goto out;
  hwloc_bitmap_sscanf(set, tmp);

  hwloc_bitmap_and(set, set, hwloc_topology_get_topology_cpuset(topology));

  parent = hwloc__pci_find_insert_io_parent_by_cpuset(topology, set);
  hwloc__pci_add_locality_before(topology, domain, bus_first, bus_last, set, parent, next);
  /* no need to free set */

  return;

 out:
  return;
}

static void
hwloc_pci_forced_locality_parse(struct hwloc_topology *topology, const char *_env)
{
  char *env = strdup(_env);
  char *tmp = env;

  while (1) {
    size_t len = strcspn(tmp, ";\r\n");
    char *next = NULL;

    if (tmp[len] != '\0') {
      tmp[len] = '\0';
      if (tmp[len+1] != '\0')
	next = &tmp[len]+1;
    }

    if (tmp[0] != '#' && tmp[0] != '/') /* ignore comments */
      hwloc_pci_forced_locality_parse_one(topology, tmp);

    if (next)
      tmp = next;
    else
      break;
  }

  free(env);
}

void
hwloc_pci_init(struct hwloc_topology *topology)
{
  topology->first_pci_locality = topology->last_pci_locality = NULL;
}

void
hwloc_pci_prepare(struct hwloc_topology *topology)
{
  const char *dmi_board_name;
  char *env;

  env = getenv("HWLOC_PCI_LOCALITY");
  if (env) {
    int fd;

    fd = open(env, O_RDONLY);
    if (fd >= 0) {
      struct stat st;
      char *buffer;
      int err = fstat(fd, &st);
      if (!err) {
	if (st.st_size <= 64*1024) { /* random limit large enough to store multiple cpusets for thousands of PUs */
	  buffer = malloc(st.st_size+1);
	  if (buffer && read(fd, buffer, st.st_size) == st.st_size) {
	    buffer[st.st_size] = '\0';
	    hwloc_pci_forced_locality_parse(topology, buffer);
	  }
	  free(buffer);
	} else {
          if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_CRITICAL|HWLOC_SHOWMSG_PCI|HWLOC_SHOWMSG_USER))
            fprintf(stderr, "hwloc/pci: Ignoring HWLOC_PCI_LOCALITY file `%s' too large (%lu bytes)\n",
                    env, (unsigned long) st.st_size);
	}
      }
      close(fd);
    } else
      hwloc_pci_forced_locality_parse(topology, env);

    /* don't apply quirks */
    return;
  }

  dmi_board_name = hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "DMIBoardName");
  if (dmi_board_name && !strcmp(dmi_board_name, "HPE CRAY EX235A")) {
    /* AMD Trento has xGMI ports connected to individual CCDs (8 cores + L3)
     * instead of NUMA nodes (pairs of CCDs within Trento) as is usual in AMD EPYC CPUs.
     * This is not described by the ACPI tables, hence we need to manually hardwire
     * the xGMI locality for the (currently single) server that currently uses that CPU.
     * It's not clear if ACPI tables can/will ever be fixed (would require one initiator
     * proximity domain per CCD), or if Linux can/will work around the issue.
     */
    unsigned i;
    hwloc_debug("Enabling for PCI locality quirk for HPE Cray EX235A\n");
    for(i=0; i<8; i++) {
      unsigned bus_min, bus_max, pu_stride;
      hwloc_obj_t parent;
      hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
      if (cpuset) {
        switch (i) {
          /* buses must be inserted in order since we just append to the locality list */
        case 0: pu_stride = 6; bus_min = 0xc0; bus_max = 0xc1; break;
        case 1: pu_stride = 7; bus_min = 0xc4; bus_max = 0xc6; break;
        case 2: pu_stride = 2; bus_min = 0xc8; bus_max = 0xc9; break;
        case 3: pu_stride = 3; bus_min = 0xcc; bus_max = 0xce; break;
        case 4: pu_stride = 0; bus_min = 0xd0; bus_max = 0xd1; break;
        case 5: pu_stride = 1; bus_min = 0xd4; bus_max = 0xd6; break;
        case 6: pu_stride = 4; bus_min = 0xd8; bus_max = 0xd9; break;
        case 7: pu_stride = 5; bus_min = 0xdc; bus_max = 0xde; break;
        default: assert(0);
        }
        hwloc_bitmap_zero(cpuset);
        hwloc_bitmap_set_range(cpuset, pu_stride*8, pu_stride*8+7);
        hwloc_bitmap_set_range(cpuset, pu_stride*8+64, pu_stride*8+71);
        parent = hwloc__pci_find_insert_io_parent_by_cpuset(topology, cpuset);
        hwloc__pci_add_locality_before(topology, 0, bus_min, bus_max, cpuset, parent, NULL);
      }
    }
    return;
  }

  env = getenv("HWLOC_PCI_LOCALITY_QUIRK_FAKE");
  if (env && atoi(env)) {
    unsigned last;
    hwloc_cpuset_t cpuset;
    struct hwloc_obj *lastpu;
    hwloc_debug("Enabling for PCI locality fake quirk (attaching the entire domain 0000 to last PU)\n");
    last = hwloc_bitmap_last(hwloc_topology_get_topology_cpuset(topology));
    lastpu = topology->levels[0][last];
    cpuset = hwloc_bitmap_dup(lastpu->cpuset);
    if (cpuset)
      hwloc__pci_add_locality_before(topology, 0, 0, 255, cpuset, lastpu, NULL);
    return;
  }
}

void
hwloc_pci_refresh(struct hwloc_topology *topology)
{
  struct hwloc_pci_locality_s *cur;

  cur = topology->first_pci_locality;
  while (cur) {
    struct hwloc_pci_locality_s *next = cur->next;
    hwloc_obj_t largeparent;

    /* refresh the cpuset */
    hwloc_bitmap_and(cur->cpuset, cur->cpuset, hwloc_topology_get_topology_cpuset(topology));

    /* refresh the parent */
    cur->parent = NULL;
    largeparent = hwloc_get_obj_covering_cpuset(topology, cur->cpuset);
    if (largeparent) {
      while (largeparent ->parent && largeparent->parent->arity == 1)
        largeparent = largeparent->parent;
      cur->parent = largeparent;
    }
    if (!cur->parent)
      cur->parent = hwloc_get_root_obj(topology);

    cur = next;
  }

  hwloc_pcidebug_show_localities("  Refreshed PCI localities", topology, 0);
}

int
hwloc_pci_dup(hwloc_topology_t new, hwloc_topology_t old)
{
  struct hwloc_tma *tma = new->tma;
  struct hwloc_pci_locality_s *oldcur;

  oldcur = old->first_pci_locality;
  while (oldcur) {
    struct hwloc_pci_locality_s *newcur = hwloc_tma_malloc(tma, sizeof(*newcur));
    if (!newcur)
      goto error;
    memcpy(newcur, oldcur, sizeof(*newcur));
    newcur->cpuset = hwloc_bitmap_tma_dup(tma, oldcur->cpuset);
    if (!newcur) {
      assert(!tma || !tma->dontfree); /* this tma cannot fail to allocate */
      free(newcur);
      goto error;
    }
    newcur->parent = NULL; /* we'll refresh below */
    hwloc___pci_insert_locality_before(new, newcur, NULL);

    oldcur = oldcur->next;
  }

  hwloc_topology_refresh(new);
  return 0;

 error:
  /* in case some localities got duplicated before failure */
  hwloc_topology_refresh(new);
  return -1;
}

void
hwloc_pci_exit(struct hwloc_topology *topology)
{
  struct hwloc_pci_locality_s *cur;

  cur = topology->first_pci_locality;
  while (cur) {
    struct hwloc_pci_locality_s *next = cur->next;
    hwloc_bitmap_free(cur->cpuset);
    free(cur);
    cur = next;
  }

  hwloc_pci_init(topology);
}


/*********************************
 * Finding PCI objects or parents
 */

/* recurse under a PCI tree parent to return
 * the object matching the given busid (and set *exact = 1)
 * or the smallest object that should contain it
 */
static struct hwloc_obj *
hwloc__pci_recurse_in_tree_for_busid(hwloc_obj_t parent,
                                     unsigned domain, unsigned bus, unsigned dev, unsigned func,
                                     int *exactp)
{
  hwloc_obj_t child;

  for_each_io_child(child, parent) {
    if (child->type == HWLOC_OBJ_PCI_DEVICE
	|| (child->type == HWLOC_OBJ_BRIDGE
	    && child->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)) {
      if (child->attr->pcidev.domain == domain
	  && child->attr->pcidev.bus == bus
	  && child->attr->pcidev.dev == dev
	  && child->attr->pcidev.func == func) {
	/* that's the right bus id */
        *exactp = 1;
	return child;
      }
      if (child->attr->pcidev.domain > domain
	  || (child->attr->pcidev.domain == domain
	      && child->attr->pcidev.bus > bus)) {
	/* bus id too high, won't find anything later, return parent */
        *exactp = 0;
	return parent;
      }
      if (child->type == HWLOC_OBJ_BRIDGE
	  && child->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
	  && child->attr->bridge.downstream.pci.domain == domain
	  && child->attr->bridge.downstream.pci.secondary_bus <= bus
	  && child->attr->bridge.downstream.pci.subordinate_bus >= bus)
	/* not the right bus id, but it's included in the bus below that bridge */
	return hwloc__pci_recurse_in_tree_for_busid(child, domain, bus, dev, func, exactp);

    } else if (child->type == HWLOC_OBJ_BRIDGE
	       && child->attr->bridge.upstream_type != HWLOC_OBJ_BRIDGE_PCI
	       && child->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
	       /* non-PCI to PCI bridge, just look at the subordinate bus */
	       && child->attr->bridge.downstream.pci.domain == domain
	       && child->attr->bridge.downstream.pci.secondary_bus <= bus
	       && child->attr->bridge.downstream.pci.subordinate_bus >= bus) {
      /* contains our bus, recurse */
      return hwloc__pci_recurse_in_tree_for_busid(child, domain, bus, dev, func, exactp);
    }
  }
  /* didn't find anything, return parent */
  *exactp = 0;
  return parent;
}

static int
hwloc__pci_get_busid_cpuset(struct hwloc_topology *topology,
                            hwloc_cpuset_t cpuset,
                            struct hwloc_pcidev_attr_s *busid)
{
  int err;

  hwloc_debug("Looking for cpuset of PCI busid %04x:%02x:%02x.%01x\n",
	      busid->domain, busid->bus, busid->dev, busid->func);

  /* get the cpuset by asking the backend that provides the relevant hook, if any. */
  struct hwloc_backend *backend = topology->get_pci_busid_cpuset_backend;
  if (backend)
    err = backend->get_pci_busid_cpuset(backend, busid, cpuset);
  else
    err = -1;
  if (err < 0)
    /* if we got nothing, assume this PCI bus is attached to the top of hierarchy */
    hwloc_bitmap_copy(cpuset, hwloc_topology_get_topology_cpuset(topology));
  else
    /* otherwise sanitize it */
    hwloc_bitmap_and(cpuset, cpuset, hwloc_topology_get_topology_cpuset(topology));

  hwloc_debug_bitmap("  got PCI bus cpuset %s\n", cpuset);
  return err;
}

struct hwloc_obj *
hwloc_pci_find_parent_by_busid(struct hwloc_topology *topology,
			       unsigned domain, unsigned bus, unsigned dev, unsigned func)
{
  struct hwloc_pcidev_attr_s busid;
  hwloc_bitmap_t cpuset;
  hwloc_obj_t parent;

  /* try to find that exact busid */
  parent = hwloc_pci_find_by_busid(topology, domain, bus, dev, func);
  if (parent)
    return parent;

  /* try to find the actual locality of that bus from OS */
  cpuset = hwloc_bitmap_alloc();
  if (!cpuset)
    return hwloc_get_root_obj(topology);

  busid.domain = domain;
  busid.bus = bus;
  busid.dev = dev;
  busid.func = func;
  if (hwloc__pci_get_busid_cpuset(topology, cpuset, &busid) < 0) {
    hwloc_bitmap_free(cpuset);
    return hwloc_get_root_obj(topology);
  }

  parent = hwloc__pci_find_insert_io_parent_by_cpuset(topology, cpuset);
  hwloc_bitmap_free(cpuset);
  return parent;
}

struct hwloc_obj *
hwloc_pci_find_by_busid(struct hwloc_topology *topology,
			unsigned domain, unsigned bus, unsigned dev, unsigned func)
{
  struct hwloc_pci_locality_s *loc;
  hwloc_obj_t parent;
  hwloc_obj_t obj = NULL;
  int exact = 0;

  /* find the parent of the tree that contains that bus */
  loc = hwloc__pci_find_bus_locality(topology, domain, bus);
  if (loc)
    parent = loc->parent;
  else
    /* otherwise look in the tree attached at root */
    parent = hwloc_get_root_obj(topology);

  /* now find the object in that tree */
  hwloc_debug("  recursively looking for busid %04x:%02x:%02x.%01x below %s P#%u\n",
	      domain, bus, dev, func,
	      hwloc_obj_type_string(parent->type), parent->os_index);
  obj = hwloc__pci_recurse_in_tree_for_busid(parent, domain, bus, dev, func, &exact);
  if (!exact) {
    hwloc_debug("  couldn't find the exact matching busid, ignoring\n");
    return NULL;
  } else {
    if (obj->type == HWLOC_OBJ_PCI_DEVICE
	|| (obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI))
      hwloc_debug("  found busid %04x:%02x:%02x.%01x\n",
		  obj->attr->pcidev.domain, obj->attr->pcidev.bus,
		  obj->attr->pcidev.dev, obj->attr->pcidev.func);
    else
      hwloc_debug("  found obj %s P#%u\n",
		  hwloc_obj_type_string(obj->type), obj->os_index);
    return obj;
  }
}


/******************************
 * Inserting in Tree by Bus ID
 */

enum hwloc_pci_busid_comparison_e {
  HWLOC_PCI_BUSID_LOWER,
  HWLOC_PCI_BUSID_HIGHER,
  HWLOC_PCI_BUSID_INCLUDED,
  HWLOC_PCI_BUSID_SUPERSET,
  HWLOC_PCI_BUSID_EQUAL
};

static enum hwloc_pci_busid_comparison_e
hwloc_pci_compare_busids(struct hwloc_obj *a, struct hwloc_obj *b)
{
#ifdef HWLOC_DEBUG
  if (a->type == HWLOC_OBJ_BRIDGE)
    assert(a->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI);
  if (b->type == HWLOC_OBJ_BRIDGE)
    assert(b->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI);
#endif

  if (a->attr->pcidev.domain < b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_LOWER;
  if (a->attr->pcidev.domain > b->attr->pcidev.domain)
    return HWLOC_PCI_BUSID_HIGHER;

  if (a->type == HWLOC_OBJ_BRIDGE && a->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
      && b->attr->pcidev.bus >= a->attr->bridge.downstream.pci.secondary_bus
      && b->attr->pcidev.bus <= a->attr->bridge.downstream.pci.subordinate_bus)
    return HWLOC_PCI_BUSID_SUPERSET;
  if (b->type == HWLOC_OBJ_BRIDGE && b->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
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

  /* Should never reach here. */
  return HWLOC_PCI_BUSID_EQUAL;
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
      if (new->type == HWLOC_OBJ_BRIDGE && new->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI) {
	/* look at remaining siblings and move some below new */
	childp = &new->io_first_child;
	curp = &new->next_sibling;
	while (*curp) {
	  hwloc_obj_t cur = *curp;
	  if (hwloc_pci_compare_busids(new, cur) == HWLOC_PCI_BUSID_LOWER) {
	    /* this sibling remains under root, after new. */
	    if (cur->attr->pcidev.domain > new->attr->pcidev.domain
		|| cur->attr->pcidev.bus > new->attr->bridge.downstream.pci.subordinate_bus)
	      /* this sibling is even above new's subordinate bus, no other sibling could go below new */
	      return;
	    curp = &cur->next_sibling;
	  } else {
	    /* this sibling goes under new */
	    *childp = cur;
	    *curp = cur->next_sibling;
	    (*childp)->parent = new;
	    (*childp)->next_sibling = NULL;
	    childp = &(*childp)->next_sibling;
	  }
	}
      }
      return;
    }
    case HWLOC_PCI_BUSID_EQUAL: {
      static int reported = 0;
      if (!reported && HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_CRITICAL|HWLOC_SHOWMSG_PCI)) {
        fprintf(stderr, "*********************************************************\n");
        fprintf(stderr, "* hwloc %s received invalid PCI information.\n", HWLOC_VERSION);
        fprintf(stderr, "*\n");
        fprintf(stderr, "* Trying to insert PCI object %04x:%02x:%02x.%01x at %04x:%02x:%02x.%01x\n",
                new->attr->pcidev.domain, new->attr->pcidev.bus, new->attr->pcidev.dev, new->attr->pcidev.func,
                (*curp)->attr->pcidev.domain, (*curp)->attr->pcidev.bus, (*curp)->attr->pcidev.dev, (*curp)->attr->pcidev.func);
        fprintf(stderr, "*\n");
        fprintf(stderr, "* hwloc will now ignore this object and continue.\n");
        fprintf(stderr, "*********************************************************\n");
        reported = 1;
      }
      hwloc_free_unlinked_object(new);
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
hwloc_pcicommon_tree_insert_by_busid(struct hwloc_obj **treep,
                                     struct hwloc_obj *obj)
{
  hwloc_pci_add_object(NULL /* no parent on top of tree */, treep, obj);
}


/**********************
 * Attaching PCI Trees
 */

static struct hwloc_obj *
hwloc_pcicommon_tree_add_hostbridges(struct hwloc_topology *topology,
                                     struct hwloc_obj *old_tree)
{
  struct hwloc_obj * new = NULL, **newp = &new;

  /*
   * tree points to all objects connected to any upstream bus in the machine.
   * We now create one real hostbridge object per upstream bus.
   * It's not actually a PCI device so we have to create it.
   */
  while (old_tree) {
    /* start a new host bridge */
    struct hwloc_obj *hostbridge;
    struct hwloc_obj **dstnextp;
    struct hwloc_obj **srcnextp;
    struct hwloc_obj *child;
    unsigned current_domain;
    unsigned char current_bus;
    unsigned char current_subordinate;

    hostbridge = hwloc_alloc_setup_object(topology, HWLOC_OBJ_BRIDGE, HWLOC_UNKNOWN_INDEX);
    if (!hostbridge) {
      /* just queue remaining things without hostbridges and return */
      *newp = old_tree;
      return new;
    }
    dstnextp = &hostbridge->io_first_child;

    srcnextp = &old_tree;
    child = *srcnextp;
    current_domain = child->attr->pcidev.domain;
    current_bus = child->attr->pcidev.bus;
    current_subordinate = current_bus;

    hwloc_debug("Adding new PCI hostbridge %04x:%02x\n", current_domain, current_bus);

  next_child:
    /* remove next child from tree */
    *srcnextp = child->next_sibling;
    /* append it to hostbridge */
    *dstnextp = child;
    child->parent = hostbridge;
    child->next_sibling = NULL;
    dstnextp = &child->next_sibling;

    /* compute hostbridge secondary/subordinate buses */
    if (child->type == HWLOC_OBJ_BRIDGE && child->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI
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
    hwloc_debug("  new PCI hostbridge covers %04x:[%02x-%02x]\n",
		current_domain, current_bus, current_subordinate);

    *newp = hostbridge;
    newp = &hostbridge->next_sibling;
  }

  return new;
}

int
hwloc_pcicommon_tree_attach(struct hwloc_topology *topology, struct hwloc_obj *tree)
{
  struct hwloc_pci_locality_s *next, *prev, *last_used;
  enum hwloc_type_filter_e bfilter;

  if (!tree)
    /* found nothing, exit */
    return 0;

  hwloc_pcidebug_show_tree("PCI hierarchy", tree);
  hwloc_pcidebug_show_localities("  Forced PCI localities", topology, 0);

  bfilter = topology->type_filter[HWLOC_OBJ_BRIDGE];
  if (bfilter != HWLOC_TYPE_FILTER_KEEP_NONE) {
    tree = hwloc_pcicommon_tree_add_hostbridges(topology, tree);
  }

  last_used = NULL;

  hwloc_debug("\nAttaching PCI trees...\n");

  while (tree) {
    struct hwloc_obj *obj, *pciobj;
    struct hwloc_obj *parent = NULL;
    hwloc_cpuset_t cpuset;
    unsigned domain, bus_min, bus_max;

    obj = tree;

    /* hostbridges don't have a PCI busid for looking up locality, use their first child */
    if (obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_HOST)
      pciobj = obj->io_first_child;
    else
      pciobj = obj;
    /* now we have a pci device or a pci bridge */
    assert(pciobj->type == HWLOC_OBJ_PCI_DEVICE
	   || (pciobj->type == HWLOC_OBJ_BRIDGE && pciobj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI));

    if (obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI) {
      domain = obj->attr->bridge.downstream.pci.domain;
      bus_min = obj->attr->bridge.downstream.pci.secondary_bus;
      bus_max = obj->attr->bridge.downstream.pci.subordinate_bus;
    } else {
      domain = pciobj->attr->pcidev.domain;
      bus_min = pciobj->attr->pcidev.bus;
      bus_max = pciobj->attr->pcidev.bus;
    }

    /* Find where to attach that PCI bus.
     * PCI buses are added in order, so we usually insert in/after the last used locality.
     * That's not always the last of the list because some localities could have been forced for later buses.
     * Hence, start walking the list at the last used locality.
     */
    next = hwloc__pci_find_locality_notbefore_bus(topology, domain, bus_min, last_used);

    /* do we overlap with that next one? */
    if (next && domain == next->domain
        && !(bus_min > next->bus_max || next->bus_min > bus_max)) {
      /* the new bus range overlaps with that existing locality, extend the existing one if needed and reuse it */
      if (next->bus_max < bus_max)
        next->bus_max = bus_max;
      if (next->bus_min > bus_min)
        next->bus_min = bus_min;
      hwloc_debug("  Extending overlapping locality %04x:%02x-%02x for %04x:%02x-%02x\n",
                  next->domain, next->bus_min, next->bus_max,
                  domain, bus_min, bus_max);
      hwloc__pci_merge_next_localities(topology, next);
      last_used = next;
      parent = next->parent;
      goto done;
    }
    prev = next ? next->prev : topology->last_pci_locality;

    /* no matching locality, ask the OS */
    hwloc_debug("  No existing locality covers %04x:%02x-%02x\n",
                domain, bus_min, bus_max);
    hwloc_debug("    Looking for parent of PCI busid %04x:%02x:%02x.%01x\n",
                domain, bus_min, pciobj->attr->pcidev.dev, pciobj->attr->pcidev.func);
    cpuset = hwloc_bitmap_alloc();
    if (!cpuset) {
      /* couldn't get the locality, just attach to root without setting locality info */
      parent = hwloc_get_root_obj(topology);
      goto done;
    }
    if (hwloc__pci_get_busid_cpuset(topology, cpuset, &pciobj->attr->pcidev) < 0) {
      /* couldn't get the locality, just attach to root without setting locality info */
      parent = hwloc_get_root_obj(topology);
      hwloc_bitmap_free(cpuset);
      goto done;
    }
    hwloc_debug_bitmap("    will attach PCI bus to cpuset %s\n", cpuset);
    /* we will go between prev and next */

    /* can we extend the previous locality? */
    if (prev
        && hwloc_bitmap_isequal(cpuset, prev->cpuset)
        && domain == prev->domain
        && (bus_min == prev->bus_max
            || bus_min == prev->bus_max+1)) {
      hwloc_debug("  Reusing PCI locality %04x:%02x-%02x up to bus %04x:%02x\n",
                  prev->domain, prev->bus_min, prev->bus_max,
                  domain, bus_max);
      prev->bus_max = bus_max;
      hwloc__pci_merge_next_localities(topology, prev);
      parent = prev->parent;
      last_used = prev;
      hwloc_bitmap_free(cpuset);
      goto done;
    }

    /* can we extend the next locality? */
    if (next
        && hwloc_bitmap_isequal(cpuset, next->cpuset)
        && domain == next->domain
        && (bus_max == next->bus_min
            || bus_max+1 == next->bus_min)) {
      hwloc_debug("  Reusing PCI locality %04x:%02x-%02x down to bus %04x:%02x\n",
                  next->domain, next->bus_min, next->bus_max,
                  domain, bus_min);
      next->bus_min = bus_min;
      /* no need to merge since we would have extended prev instead above */
      last_used = next;
      parent = next->parent;
      hwloc_bitmap_free(cpuset);
      goto done;
    }

    /* create a new locality */
    parent = hwloc__pci_find_insert_io_parent_by_cpuset(topology, cpuset);
    hwloc_debug("Adding PCI locality %s P#%u for bus %04x:[%02x:%02x]\n",
                hwloc_obj_type_string(parent->type), parent->os_index, domain, bus_min, bus_max);

    last_used = hwloc__pci_add_locality_before(topology, domain, bus_min, bus_max, cpuset, parent, next);
    if (!last_used) {
      /* fallback to attaching to root */
      parent = hwloc_get_root_obj(topology);
      last_used = NULL;
    }

  done:
    /* dequeue this object */
    tree = obj->next_sibling;
    obj->next_sibling = NULL;
    hwloc_insert_object_by_parent(topology, parent, obj);
  }

  hwloc_pcidebug_show_localities("  PCI localities after inserting trees", topology, 1);

  return 0;
}


/*******************************************
 * Helpers for Parsing the PCI Config Space
 */

#define HWLOC_PCI_STATUS 0x06
#define HWLOC_PCI_STATUS_CAP_LIST 0x10
#define HWLOC_PCI_CAPABILITY_LIST 0x34
#define HWLOC_PCI_CAP_LIST_ID 0
#define HWLOC_PCI_CAP_LIST_NEXT 1

unsigned
hwloc_pcicommon_configspace_find_cap(const unsigned char *config, unsigned cap)
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
hwloc_pcicommon_configspace_find_linkspeed(const unsigned char *config,
                                           unsigned offset, float *linkspeed)
{
  unsigned linksta, speed, width;

  memcpy(&linksta, &config[offset + HWLOC_PCI_EXP_LNKSTA], 4);
  speed = linksta & HWLOC_PCI_EXP_LNKSTA_SPEED; /* PCIe generation */
  width = (linksta & HWLOC_PCI_EXP_LNKSTA_WIDTH) >> 4; /* how many lanes */

  *linkspeed = hwloc__pci_link_speed(speed, width);
  return 0;
}

#define HWLOC_PCI_HEADER_TYPE 0x0e
#define HWLOC_PCI_HEADER_TYPE_BRIDGE 1
#define HWLOC_PCI_CLASS_BRIDGE_PCI 0x0604

hwloc_obj_type_t
hwloc_pcicommon_configspace_check_bridge_type(unsigned device_class, const unsigned char *config)
{
  unsigned char headertype;

  if (device_class != HWLOC_PCI_CLASS_BRIDGE_PCI)
    return HWLOC_OBJ_PCI_DEVICE;

  headertype = config[HWLOC_PCI_HEADER_TYPE] & 0x7f;
  return (headertype == HWLOC_PCI_HEADER_TYPE_BRIDGE)
    ? HWLOC_OBJ_BRIDGE : HWLOC_OBJ_PCI_DEVICE;
}

#define HWLOC_PCI_PRIMARY_BUS 0x18
#define HWLOC_PCI_SECONDARY_BUS 0x19
#define HWLOC_PCI_SUBORDINATE_BUS 0x1a

int
hwloc_pcicommon_configspace_find_bridge_buses(unsigned domain, unsigned bus, unsigned dev, unsigned func,
                                              unsigned *secondary_busp, unsigned *subordinate_busp,
                                              const unsigned char *config)
{
  unsigned secondary_bus, subordinate_bus;

  if (config[HWLOC_PCI_PRIMARY_BUS] != bus) {
    /* Sometimes the config space contains 00 instead of the actual primary bus number.
     * Always trust the bus ID because it was built by the system which has more information
     * to workaround such problems (e.g. ACPI information about PCI parent/children).
     */
    hwloc_debug("  %04x:%02x:%02x.%01x bridge with (ignored) invalid PCI_PRIMARY_BUS %02x\n",
		domain, bus, dev, func, config[HWLOC_PCI_PRIMARY_BUS]);
  }

  secondary_bus = config[HWLOC_PCI_SECONDARY_BUS];
  subordinate_bus = config[HWLOC_PCI_SUBORDINATE_BUS];

  if (secondary_bus <= bus
      || subordinate_bus <= bus
      || secondary_bus > subordinate_bus) {
    /* This should catch most cases of invalid bridge information
     * (e.g. 00 for secondary and subordinate).
     * Ideally we would also check that [secondary-subordinate] is included
     * in the parent bridge [secondary+1:subordinate]. But that's hard to do
     * because objects may be discovered out of order (especially in the fsroot case).
     */
    hwloc_debug("  %04x:%02x:%02x.%01x bridge has invalid secondary-subordinate buses [%02x-%02x]\n",
		domain, bus, dev, func,
		secondary_bus, subordinate_bus);
    return -1;
  }

  *secondary_busp = secondary_bus;
  *subordinate_busp = subordinate_bus;
  return 0;
}


/****************
 * Class Strings
 */

const char *
hwloc_pci_class_string(unsigned short class_id)
{
  /* See https://pci-ids.ucw.cz/read/PD/ */
  switch ((class_id & 0xff00) >> 8) {
    case 0x00:
      switch (class_id) {
	case 0x0001: return "VGA";
      }
      break;
    case 0x01:
      switch (class_id) {
	case 0x0100: return "SCSI";
	case 0x0101: return "IDE";
	case 0x0102: return "Floppy";
	case 0x0103: return "IPI";
	case 0x0104: return "RAID";
	case 0x0105: return "ATA";
	case 0x0106: return "SATA";
	case 0x0107: return "SAS";
	case 0x0108: return "NVMExp";
      }
      return "Storage";
    case 0x02:
      switch (class_id) {
	case 0x0200: return "Ethernet";
	case 0x0201: return "TokenRing";
	case 0x0202: return "FDDI";
	case 0x0203: return "ATM";
	case 0x0204: return "ISDN";
	case 0x0205: return "WorldFip";
	case 0x0206: return "PICMG";
	case 0x0207: return "InfiniBand";
	case 0x0208: return "Fabric";
      }
      return "Network";
    case 0x03:
      switch (class_id) {
	case 0x0300: return "VGA";
	case 0x0301: return "XGA";
	case 0x0302: return "3D";
      }
      return "Display";
    case 0x04:
      switch (class_id) {
	case 0x0400: return "MultimediaVideo";
	case 0x0401: return "MultimediaAudio";
	case 0x0402: return "Telephony";
	case 0x0403: return "AudioDevice";
      }
      return "Multimedia";
    case 0x05:
      switch (class_id) {
	case 0x0500: return "RAM";
	case 0x0501: return "Flash";
        case 0x0502: return "CXLMem";
      }
      return "Memory";
    case 0x06:
      switch (class_id) {
	case 0x0600: return "HostBridge";
	case 0x0601: return "ISABridge";
	case 0x0602: return "EISABridge";
	case 0x0603: return "MicroChannelBridge";
	case 0x0604: return "PCIBridge";
	case 0x0605: return "PCMCIABridge";
	case 0x0606: return "NubusBridge";
	case 0x0607: return "CardBusBridge";
	case 0x0608: return "RACEwayBridge";
	case 0x0609: return "SemiTransparentPCIBridge";
	case 0x060a: return "InfiniBandPCIHostBridge";
      }
      return "Bridge";
    case 0x07:
      switch (class_id) {
	case 0x0700: return "Serial";
	case 0x0701: return "Parallel";
	case 0x0702: return "MultiportSerial";
	case 0x0703: return "Model";
	case 0x0704: return "GPIB";
	case 0x0705: return "SmartCard";
      }
      return "Communication";
    case 0x08:
      switch (class_id) {
	case 0x0800: return "PIC";
	case 0x0801: return "DMA";
	case 0x0802: return "Timer";
	case 0x0803: return "RTC";
	case 0x0804: return "PCIHotPlug";
	case 0x0805: return "SDHost";
	case 0x0806: return "IOMMU";
      }
      return "SystemPeripheral";
    case 0x09:
      switch (class_id) {
	case 0x0900: return "Keyboard";
	case 0x0901: return "DigitizerPen";
	case 0x0902: return "Mouse";
	case 0x0903: return "Scanern";
	case 0x0904: return "Gameport";
      }
      return "Input";
    case 0x0a:
      return "DockingStation";
    case 0x0b:
      switch (class_id) {
	case 0x0b00: return "386";
	case 0x0b01: return "486";
	case 0x0b02: return "Pentium";
/* 0x0b03 and 0x0b04 might be Pentium and P6 ? */
	case 0x0b10: return "Alpha";
	case 0x0b20: return "PowerPC";
	case 0x0b30: return "MIPS";
	case 0x0b40: return "Co-Processor";
      }
      return "Processor";
    case 0x0c:
      switch (class_id) {
	case 0x0c00: return "FireWire";
	case 0x0c01: return "ACCESS";
	case 0x0c02: return "SSA";
	case 0x0c03: return "USB";
	case 0x0c04: return "FibreChannel";
	case 0x0c05: return "SMBus";
	case 0x0c06: return "InfiniBand";
	case 0x0c07: return "IPMI-SMIC";
	case 0x0c08: return "SERCOS";
	case 0x0c09: return "CANBUS";
      }
      return "SerialBus";
    case 0x0d:
      switch (class_id) {
	case 0x0d00: return "IRDA";
	case 0x0d01: return "ConsumerIR";
	case 0x0d10: return "RF";
	case 0x0d11: return "Bluetooth";
	case 0x0d12: return "Broadband";
	case 0x0d20: return "802.1a";
	case 0x0d21: return "802.1b";
      }
      return "Wireless";
    case 0x0e:
      switch (class_id) {
	case 0x0e00: return "I2O";
      }
      return "Intelligent";
    case 0x0f:
      return "Satellite";
    case 0x10:
      return "Encryption";
    case 0x11:
      return "SignalProcessing";
    case 0x12:
      return "ProcessingAccelerator";
    case 0x13:
      return "Instrumentation";
    case 0x40:
      return "Co-Processor";
  }
  return "Other";
}
