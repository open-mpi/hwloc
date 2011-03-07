/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2010 INRIA.  All rights reserved.
 * Copyright © 2009-2010 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <assert.h>
#define NUMA_VERSION1_COMPATIBILITY
#include <hwloc/linux-libnuma.h>

/* check the linux libnuma helpers */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_bitmap_t set, set2, nocpunodeset;
  hwloc_obj_t node;
  struct bitmask *bitmask, *bitmask2;
  nodemask_t nodemask, nodemask2;
  unsigned long mask;
  unsigned long maxnode;
  int i;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  /* convert full stuff between cpuset and libnuma */
  set = hwloc_bitmap_alloc();
  nocpunodeset = hwloc_bitmap_alloc();
  /* gather all nodes if any, or the whole system if no nodes */
  if (hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE)) {
    node = NULL;
    while ((node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NODE, node)) != NULL) {
      hwloc_bitmap_or(set, set, node->cpuset);
      if (hwloc_bitmap_iszero(node->cpuset))
	hwloc_bitmap_set(nocpunodeset, node->os_index);
    }
  } else {
    hwloc_bitmap_or(set, set, hwloc_topology_get_complete_cpuset(topology));
  }

  set2 = hwloc_bitmap_alloc();
  hwloc_cpuset_from_linux_libnuma_bitmask(topology, set2, numa_all_nodes_ptr);
  assert(hwloc_bitmap_isequal(set, set2));
  hwloc_bitmap_free(set2);

  set2 = hwloc_bitmap_alloc();
  hwloc_cpuset_from_linux_libnuma_nodemask(topology, set2, &numa_all_nodes);
  assert(hwloc_bitmap_isequal(set, set2));
  hwloc_bitmap_free(set2);

  bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, set);
  hwloc_bitmap_foreach_begin(i, nocpunodeset) { numa_bitmask_setbit(bitmask, i); } hwloc_bitmap_foreach_end();
  assert(numa_bitmask_equal(bitmask, numa_all_nodes_ptr));
  numa_bitmask_free(bitmask);

  hwloc_cpuset_to_linux_libnuma_nodemask(topology, set, &nodemask);
  hwloc_bitmap_foreach_begin(i, nocpunodeset) { nodemask_set(&nodemask, i); } hwloc_bitmap_foreach_end();
  assert(!memcmp(&nodemask, &numa_all_nodes, sizeof(nodemask_t)));

  hwloc_bitmap_free(set);
  hwloc_bitmap_free(nocpunodeset);

  /* convert full stuff between nodeset and libnuma */
  if (hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE)) {
    set = hwloc_bitmap_dup(hwloc_get_root_obj(topology)->complete_nodeset);
  } else {
    set = hwloc_bitmap_alloc();
    hwloc_bitmap_fill(set);
  }

  set2 = hwloc_bitmap_alloc();
  hwloc_nodeset_from_linux_libnuma_bitmask(topology, set2, numa_all_nodes_ptr);
  assert(hwloc_bitmap_isequal(set, set2));
  hwloc_bitmap_free(set2);

  set2 = hwloc_bitmap_alloc();
  hwloc_nodeset_from_linux_libnuma_nodemask(topology, set2, &numa_all_nodes);
  assert(hwloc_bitmap_isequal(set, set2));
  hwloc_bitmap_free(set2);

  bitmask = hwloc_nodeset_to_linux_libnuma_bitmask(topology, set);
  assert(numa_bitmask_equal(bitmask, numa_all_nodes_ptr));
  numa_bitmask_free(bitmask);

  hwloc_nodeset_to_linux_libnuma_nodemask(topology, set, &nodemask);
  assert(!memcmp(&nodemask, &numa_all_nodes, sizeof(nodemask_t)));

  hwloc_bitmap_free(set);

  /* convert empty stuff between cpuset and libnuma */
  nodemask_zero(&nodemask);
  set = hwloc_bitmap_alloc();
  hwloc_cpuset_from_linux_libnuma_nodemask(topology, set, &nodemask);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  bitmask = numa_bitmask_alloc(1);
  set = hwloc_bitmap_alloc();
  hwloc_cpuset_from_linux_libnuma_bitmask(topology, set, bitmask);
  numa_bitmask_free(bitmask);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  mask=0;
  set = hwloc_bitmap_alloc();
  hwloc_cpuset_from_linux_libnuma_ulongs(topology, set, &mask, sizeof(mask)*8);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, set);
  bitmask2 = numa_bitmask_alloc(1);
  assert(numa_bitmask_equal(bitmask, bitmask2));
  numa_bitmask_free(bitmask);
  numa_bitmask_free(bitmask2);
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  hwloc_cpuset_to_linux_libnuma_nodemask(topology, set, &nodemask);
  nodemask_zero(&nodemask2);
  assert(nodemask_equal(&nodemask, &nodemask2));
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  maxnode = sizeof(mask)*8;
  hwloc_cpuset_to_linux_libnuma_ulongs(topology, set, &mask, &maxnode);
  assert(!mask);
  assert(!maxnode);
  hwloc_bitmap_free(set);

  /* convert empty stuff between nodeset and libnuma */
  nodemask_zero(&nodemask);
  set = hwloc_bitmap_alloc();
  hwloc_nodeset_from_linux_libnuma_nodemask(topology, set, &nodemask);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  bitmask = numa_bitmask_alloc(1);
  set = hwloc_bitmap_alloc();
  hwloc_nodeset_from_linux_libnuma_bitmask(topology, set, bitmask);
  numa_bitmask_free(bitmask);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  mask=0;
  set = hwloc_bitmap_alloc();
  hwloc_nodeset_from_linux_libnuma_ulongs(topology, set, &mask, sizeof(mask)*8);
  assert(hwloc_bitmap_iszero(set));
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  bitmask = hwloc_nodeset_to_linux_libnuma_bitmask(topology, set);
  bitmask2 = numa_bitmask_alloc(1);
  assert(numa_bitmask_equal(bitmask, bitmask2));
  numa_bitmask_free(bitmask);
  numa_bitmask_free(bitmask2);
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  hwloc_nodeset_to_linux_libnuma_nodemask(topology, set, &nodemask);
  nodemask_zero(&nodemask2);
  assert(nodemask_equal(&nodemask, &nodemask2));
  hwloc_bitmap_free(set);

  set = hwloc_bitmap_alloc();
  maxnode = sizeof(mask)*8;
  hwloc_nodeset_to_linux_libnuma_ulongs(topology, set, &mask, &maxnode);
  assert(!mask);
  assert(!maxnode);
  hwloc_bitmap_free(set);

  /* convert first node between cpuset/nodeset and libnuma */
  node = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_NODE, NULL);
  if (node) {
    /* convert first node between cpuset and libnuma */
    hwloc_cpuset_to_linux_libnuma_nodemask(topology, node->cpuset, &nodemask);
    assert(nodemask_isset(&nodemask, node->os_index));
    nodemask_clr(&nodemask, node->os_index);
    nodemask_zero(&nodemask2);
    assert(nodemask_equal(&nodemask, &nodemask2));

    bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, node->cpuset);
    assert(numa_bitmask_isbitset(bitmask, node->os_index));
    numa_bitmask_clearbit(bitmask, node->os_index);
    bitmask2 = numa_bitmask_alloc(1);
    assert(numa_bitmask_equal(bitmask, bitmask2));
    numa_bitmask_free(bitmask);
    numa_bitmask_free(bitmask2);

    maxnode = sizeof(mask)*8;
    hwloc_cpuset_to_linux_libnuma_ulongs(topology, node->cpuset, &mask, &maxnode);
    if (node->os_index >= sizeof(mask)*8) {
      assert(!maxnode);
      assert(!mask);
    } else {
      assert(maxnode = node->os_index + 1);
      assert(mask == (1U<<node->os_index));
    }

    set = hwloc_bitmap_alloc();
    nodemask_zero(&nodemask);
    nodemask_set(&nodemask, node->os_index);
    hwloc_cpuset_from_linux_libnuma_nodemask(topology, set, &nodemask);
    assert(hwloc_bitmap_isequal(set, node->cpuset));
    hwloc_bitmap_free(set);

    set = hwloc_bitmap_alloc();
    bitmask = numa_bitmask_alloc(1);
    numa_bitmask_setbit(bitmask, 0);
    hwloc_cpuset_from_linux_libnuma_bitmask(topology, set, bitmask);
    numa_bitmask_free(bitmask);
    assert(hwloc_bitmap_isequal(set, node->cpuset));
    hwloc_bitmap_free(set);

    set = hwloc_bitmap_alloc();
    if (node->os_index >= sizeof(mask)*8) {
      mask = 0;
    } else {
      mask = 1 << node->os_index;
    }
    hwloc_cpuset_from_linux_libnuma_ulongs(topology, set, &mask, 1);
    assert(hwloc_bitmap_isequal(set, node->cpuset));
    hwloc_bitmap_free(set);

    /* convert first node between nodeset and libnuma */
    hwloc_nodeset_to_linux_libnuma_nodemask(topology, node->nodeset, &nodemask);
    assert(nodemask_isset(&nodemask, node->os_index));
    nodemask_clr(&nodemask, node->os_index);
    nodemask_zero(&nodemask2);
    assert(nodemask_equal(&nodemask, &nodemask2));

    bitmask = hwloc_nodeset_to_linux_libnuma_bitmask(topology, node->nodeset);
    assert(numa_bitmask_isbitset(bitmask, node->os_index));
    numa_bitmask_clearbit(bitmask, node->os_index);
    bitmask2 = numa_bitmask_alloc(1);
    assert(numa_bitmask_equal(bitmask, bitmask2));
    numa_bitmask_free(bitmask);
    numa_bitmask_free(bitmask2);

    maxnode = sizeof(mask)*8;
    hwloc_nodeset_to_linux_libnuma_ulongs(topology, node->nodeset, &mask, &maxnode);
    if (node->os_index >= sizeof(mask)*8) {
      assert(!maxnode);
      assert(!mask);
    } else {
      assert(maxnode = node->os_index + 1);
      assert(mask == (1U<<node->os_index));
    }

    set = hwloc_bitmap_alloc();
    nodemask_zero(&nodemask);
    nodemask_set(&nodemask, node->os_index);
    hwloc_nodeset_from_linux_libnuma_nodemask(topology, set, &nodemask);
    assert(hwloc_bitmap_isequal(set, node->nodeset));
    hwloc_bitmap_free(set);

    set = hwloc_bitmap_alloc();
    bitmask = numa_bitmask_alloc(1);
    numa_bitmask_setbit(bitmask, 0);
    hwloc_nodeset_from_linux_libnuma_bitmask(topology, set, bitmask);
    numa_bitmask_free(bitmask);
    assert(hwloc_bitmap_isequal(set, node->nodeset));
    hwloc_bitmap_free(set);

    set = hwloc_bitmap_alloc();
    if (node->os_index >= sizeof(mask)*8) {
      mask = 0;
    } else {
      mask = 1 << node->os_index;
    }
    hwloc_nodeset_from_linux_libnuma_ulongs(topology, set, &mask, 1);
    assert(hwloc_bitmap_isequal(set, node->nodeset));
    hwloc_bitmap_free(set);
  }


  hwloc_topology_destroy(topology);
  return 0;
}
