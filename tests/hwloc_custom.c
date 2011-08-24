/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
  hwloc_topology_t local, global;
  hwloc_obj_t sw1, sw2, sw11, sw12, sw21, sw22, root;

  printf("Loading the local topology...\n");
  hwloc_topology_init(&local);
  hwloc_topology_set_synthetic(local, "n:2 s:2 ca:1 core:2 ca:2 pu:2");
  hwloc_topology_load(local);

  printf("Creating a custom topology...\n");
  hwloc_topology_init(&global);
  hwloc_topology_set_custom(global);

  printf("Inserting the local topology into the global one...\n");
  root = hwloc_get_root_obj(global);

  sw1 = hwloc_topology_insert_misc_object_by_parent(global, root, "Switch");
  sw11 = hwloc_topology_insert_misc_object_by_parent(global, sw1, "Switch");
  hwloc_topology_insert_topology(global, sw11, local);
  hwloc_topology_insert_topology(global, sw11, local);
  sw12 = hwloc_topology_insert_misc_object_by_parent(global, sw1, "Switch");
  hwloc_topology_insert_topology(global, sw12, local);
  hwloc_topology_insert_topology(global, sw12, local);

  sw2 = hwloc_topology_insert_misc_object_by_parent(global, root, "Switch");
  sw21 = hwloc_topology_insert_misc_object_by_parent(global, sw2, "Switch");
  hwloc_topology_insert_topology(global, sw21, local);
  hwloc_topology_insert_topology(global, sw21, local);
  sw22 = hwloc_topology_insert_misc_object_by_parent(global, sw2, "Switch");
  hwloc_topology_insert_topology(global, sw22, local);
  hwloc_topology_insert_topology(global, sw22, local);

  hwloc_topology_destroy(local);

  printf("Building the global topology...\n");
  hwloc_topology_load(global);
  hwloc_topology_check(global);

  assert(hwloc_topology_get_depth(global) == 8);
  assert(hwloc_get_depth_type(global, 0) == HWLOC_OBJ_SYSTEM);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_SYSTEM) == 1);
  assert(hwloc_get_depth_type(global, 1) == HWLOC_OBJ_MACHINE);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_MACHINE) == 8);
  assert(hwloc_get_depth_type(global, 2) == HWLOC_OBJ_NODE);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_NODE) == 16);
  assert(hwloc_get_depth_type(global, 3) == HWLOC_OBJ_SOCKET);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_SOCKET) == 32);
  assert(hwloc_get_depth_type(global, 4) == HWLOC_OBJ_CACHE);
  assert(hwloc_get_nbobjs_by_depth(global, 4) == 32);
  assert(hwloc_get_depth_type(global, 5) == HWLOC_OBJ_CORE);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_CORE) == 64);
  assert(hwloc_get_depth_type(global, 6) == HWLOC_OBJ_CACHE);
  assert(hwloc_get_nbobjs_by_depth(global, 6) == 128);
  assert(hwloc_get_depth_type(global, 7) == HWLOC_OBJ_PU);
  assert(hwloc_get_nbobjs_by_type(global, HWLOC_OBJ_PU) == 256);

  hwloc_topology_destroy(global);

  return 0;
}
