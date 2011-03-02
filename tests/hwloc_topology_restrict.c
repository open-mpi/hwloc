/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static hwloc_topology_t topology;

static void check(unsigned nbnodes, unsigned nbcores, unsigned nbpus)
{
  unsigned depth;
  unsigned nb;
  unsigned long long total_memory;

  /* sanity checks */
  depth = hwloc_topology_get_depth(topology);
  assert(depth == 4);
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE);
  assert(depth == 1);
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE);
  assert(depth == 2);
  depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
  assert(depth == 3);

  /* actual checks */
  nb = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
  assert(nb == nbnodes);
  nb = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
  assert(nb == nbcores);
  nb = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  assert(nb == nbpus);
  total_memory = hwloc_get_root_obj(topology)->memory.total_memory;
  assert(total_memory == nbnodes * 1024*1024*1024); /* synthetic topology puts 1GB per node */
}

int main(void)
{
  hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "node:3 core:2 pu:4");
  hwloc_topology_load(topology);

  /* entire topology */
  printf("starting from full topology\n");
  check(3, 6, 24);

  /* restrict to nothing, impossible */
  printf("restricting to nothing, must fail\n");
  hwloc_bitmap_zero(cpuset);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(err < 0 && errno == EINVAL);
  check(3, 6, 24);

  /* restrict to everything, will do nothing */
  printf("restricting to nothing, does nothing\n");
  hwloc_bitmap_fill(cpuset);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(3, 6, 24);

  /* remove a single pu (second PU of second core of second node) */
  printf("removing one PU\n");
  hwloc_bitmap_fill(cpuset);
  hwloc_bitmap_clr(cpuset, 13);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(3, 6, 23);

  /* remove the entire second core of first node */
  printf("removing one core\n");
  hwloc_bitmap_fill(cpuset);
  hwloc_bitmap_clr_range(cpuset, 4, 7);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(3, 5, 19);

  /* remove the entire third node */
  printf("removing one node\n");
  hwloc_bitmap_fill(cpuset);
  hwloc_bitmap_clr_range(cpuset, 16, 23);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(2, 3, 11);

  /* restrict to the third node, impossible */
  printf("restricting to only some already removed node, must fail\n");
  hwloc_bitmap_zero(cpuset);
  hwloc_bitmap_set_range(cpuset, 16, 23);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(err == -1 && errno == EINVAL);
  check(2, 3, 11);

  /* only keep three PUs (first and last of first core, and last of last core of second node) */
  printf("restricting to 3 PUs\n");
  hwloc_bitmap_zero(cpuset);
  hwloc_bitmap_set(cpuset, 0);
  hwloc_bitmap_set(cpuset, 3);
  hwloc_bitmap_set(cpuset, 15);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(2, 2, 3);

  /* only keep one PU (last of last core of second node) */
  printf("restricting to a single PU\n");
  hwloc_bitmap_zero(cpuset);
  hwloc_bitmap_set(cpuset, 15);
  err = hwloc_topology_restrict(topology, cpuset);
  assert(!err);
  check(1, 1, 1);

  /* TODO: check distance matrix too */

  hwloc_topology_destroy(topology);

  hwloc_bitmap_free(cpuset);

  return 0;
}
