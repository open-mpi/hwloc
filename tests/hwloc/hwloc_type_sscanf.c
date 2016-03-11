/*
 * Copyright © 2016 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <errno.h>

#include <hwloc.h>

static void check(hwloc_obj_t obj, int verbose)
{
  hwloc_obj_t child;
  char buffer[64];
  hwloc_obj_type_t type;
  union hwloc_obj_attr_u attr;
  int err;

  err = hwloc_obj_type_snprintf(buffer, sizeof(buffer), obj, verbose);
  assert(err > 0);
  err = hwloc_type_sscanf(buffer, &type, &attr, sizeof(attr));
  assert(!err);
  assert(obj->type == type);
  if (hwloc_obj_type_is_cache(type)) {
    assert(attr.cache.type == obj->attr->cache.type);
    assert(attr.cache.depth == obj->attr->cache.depth);
  } else if (type == HWLOC_OBJ_GROUP) {
    assert(attr.group.depth == obj->attr->group.depth);
  } else if (type == HWLOC_OBJ_BRIDGE) {
    assert(attr.bridge.upstream_type == obj->attr->bridge.upstream_type);
    assert(attr.bridge.downstream_type == obj->attr->bridge.downstream_type);
  } else if (type == HWLOC_OBJ_OS_DEVICE) {
    assert(attr.osdev.type == obj->attr->osdev.type);
  }

  for(child = obj->first_child; child; child = child->next_sibling)
    check(child, verbose);
  for(child = obj->io_first_child; child; child = child->next_sibling)
    check(child, verbose);
  for(child = obj->misc_first_child; child; child = child->next_sibling)
    check(child, verbose);
}

/* check whether type_sscanf() understand what type_snprintf() wrote */
int main(void)
{
  int err;
  hwloc_topology_t topology;

  err = hwloc_topology_init(&topology);
  assert(!err);
  hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
  err = hwloc_topology_load(topology);
  assert(!err);

  check(hwloc_get_root_obj(topology), 0);
  check(hwloc_get_root_obj(topology), 1);

  hwloc_topology_destroy(topology);
}
