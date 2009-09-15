/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#define NUMA_VERSION1_COMPATIBILITY
#include <hwloc/linux-libnuma.h>

/* check the linux libnuma helpers */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_cpuset_t set, set2;
  hwloc_obj_t node;
  struct bitmask *bitmask, *bitmask2;
  nodemask_t nodemask, nodemask2;
  unsigned long mask;
  unsigned long maxnode;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  /* convert full nodemask/bitmask to cpuset */
  hwloc_cpuset_zero(&set);
  /* gather all nodes if any, or the whole system if no nodes */
  if (hwloc_get_nbobjs(topology, HWLOC_OBJ_NODE)) {
    node = NULL;
    while ((node = hwloc_get_next_obj(topology, HWLOC_OBJ_NODE, node)) != NULL)
      hwloc_cpuset_orset(&set, &node->cpuset);
  } else {
    hwloc_cpuset_orset(&set, &hwloc_get_system_obj(topology)->cpuset);
  }

  hwloc_cpuset_from_linux_libnuma_bitmask(topology, &set2, numa_all_nodes_ptr);
  assert(hwloc_cpuset_isequal(&set, &set2));

  hwloc_cpuset_from_linux_libnuma_nodemask(topology, &set2, &numa_all_nodes);
  assert(hwloc_cpuset_isequal(&set, &set2));


  /* convert full cpuset to nodemask/bitmask */
  bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, &set);
  assert(numa_bitmask_equal(bitmask, numa_all_nodes_ptr));
  numa_bitmask_free(bitmask);

  hwloc_cpuset_to_linux_libnuma_nodemask(topology, &set, &nodemask);
  assert(!memcmp(&nodemask, &numa_all_nodes, sizeof(nodemask_t)));


  /* convert empty nodemask/bitmask to cpuset */
  nodemask_zero(&nodemask);
  hwloc_cpuset_from_linux_libnuma_nodemask(topology, &set, &nodemask);
  assert(hwloc_cpuset_iszero(&set));

  bitmask = numa_bitmask_alloc(1);
  hwloc_cpuset_from_linux_libnuma_bitmask(topology, &set, bitmask);
  numa_bitmask_free(bitmask);
  assert(hwloc_cpuset_iszero(&set));

  mask=0;
  hwloc_cpuset_from_linux_libnuma_ulongs(topology, &set, &mask, HWLOC_BITS_PER_LONG);
  assert(hwloc_cpuset_iszero(&set));


  /* convert empty nodemask/bitmask from cpuset */
  hwloc_cpuset_zero(&set);
  bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, &set);
  bitmask2 = numa_bitmask_alloc(1);
  assert(numa_bitmask_equal(bitmask, bitmask2));
  numa_bitmask_free(bitmask);
  numa_bitmask_free(bitmask2);

  hwloc_cpuset_zero(&set);
  hwloc_cpuset_to_linux_libnuma_nodemask(topology, &set, &nodemask);
  nodemask_zero(&nodemask2);
  assert(nodemask_equal(&nodemask, &nodemask2));

  hwloc_cpuset_zero(&set);
  maxnode = HWLOC_BITS_PER_LONG;
  hwloc_cpuset_to_linux_libnuma_ulongs(topology, &set, &mask, &maxnode);
  assert(!mask);
  assert(!maxnode);


  /* convert last node nodemask/bitmask from/to cpuset */
  node = hwloc_get_next_obj(topology, HWLOC_OBJ_NODE, NULL);
  if (node) {
    hwloc_cpuset_to_linux_libnuma_nodemask(topology, &node->cpuset, &nodemask);
    assert(nodemask_isset(&nodemask, node->os_index));
    nodemask_clr(&nodemask, node->os_index);
    nodemask_zero(&nodemask2);
    assert(nodemask_equal(&nodemask, &nodemask2));

    bitmask = hwloc_cpuset_to_linux_libnuma_bitmask(topology, &node->cpuset);
    assert(numa_bitmask_isbitset(bitmask, node->os_index));
    numa_bitmask_clearbit(bitmask, node->os_index);
    bitmask2 = numa_bitmask_alloc(1);
    assert(numa_bitmask_equal(bitmask, bitmask2));
    numa_bitmask_free(bitmask);
    numa_bitmask_free(bitmask2);

    maxnode = HWLOC_BITS_PER_LONG;
    hwloc_cpuset_to_linux_libnuma_ulongs(topology, &node->cpuset, &mask, &maxnode);
    if (node->os_index >= HWLOC_BITS_PER_LONG) {
      assert(!maxnode);
      assert(!mask);
    } else {
      assert(maxnode = node->os_index + 1);
      assert(mask == (1<<node->os_index));
    }
  }


  hwloc_topology_destroy(topology);
  return 0;
}
