/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#include <topology.h>
#define NUMA_VERSION1_COMPATIBILITY
#include <topology/linux-libnuma.h>

/* check the linux libnuma helpers */

int main(void)
{
  topo_topology_t topology;
  topo_cpuset_t set, set2;
  topo_obj_t node;
  struct bitmask *bitmask, *bitmask2;
  nodemask_t nodemask, nodemask2;
  unsigned long mask;
  unsigned long maxnode;

  topo_topology_init(&topology);
  topo_topology_load(topology);


  /* convert full nodemask/bitmask to cpuset */
  topo_cpuset_zero(&set);
  node = NULL;
  while ((node = topo_get_next_obj(topology, TOPO_OBJ_NODE, node)) != NULL)
    topo_cpuset_orset(&set, &node->cpuset);

  topo_cpuset_from_linux_libnuma_bitmask(topology, &set2, numa_all_nodes_ptr);
  assert(topo_cpuset_isequal(&set, &set2));

  topo_cpuset_from_linux_libnuma_nodemask(topology, &set2, &numa_all_nodes);
  assert(topo_cpuset_isequal(&set, &set2));


  /* convert full cpuset to nodemask/bitmask */
  bitmask = topo_cpuset_to_linux_libnuma_bitmask(topology, &set);
  assert(numa_bitmask_equal(bitmask, numa_all_nodes_ptr));
  numa_bitmask_free(bitmask);

  topo_cpuset_to_linux_libnuma_nodemask(topology, &set, &nodemask);
  assert(!memcmp(&nodemask, &numa_all_nodes, sizeof(nodemask_t)));


  /* convert empty nodemask/bitmask to cpuset */
  nodemask_zero(&nodemask);
  topo_cpuset_from_linux_libnuma_nodemask(topology, &set, &nodemask);
  assert(topo_cpuset_iszero(&set));

  bitmask = numa_bitmask_alloc(1);
  topo_cpuset_from_linux_libnuma_bitmask(topology, &set, bitmask);
  numa_bitmask_free(bitmask);
  assert(topo_cpuset_iszero(&set));

  mask=0;
  topo_cpuset_from_linux_libnuma_ulongs(topology, &set, &mask, TOPO_BITS_PER_LONG);
  assert(topo_cpuset_iszero(&set));


  /* convert empty nodemask/bitmask from cpuset */
  topo_cpuset_zero(&set);
  bitmask = topo_cpuset_to_linux_libnuma_bitmask(topology, &set);
  bitmask2 = numa_bitmask_alloc(1);
  assert(numa_bitmask_equal(bitmask, bitmask2));
  numa_bitmask_free(bitmask);
  numa_bitmask_free(bitmask2);

  topo_cpuset_zero(&set);
  topo_cpuset_to_linux_libnuma_nodemask(topology, &set, &nodemask);
  nodemask_zero(&nodemask2);
  assert(nodemask_equal(&nodemask, &nodemask2));

  topo_cpuset_zero(&set);
  maxnode = TOPO_BITS_PER_LONG;
  topo_cpuset_to_linux_libnuma_ulongs(topology, &set, &mask, &maxnode);
  assert(!mask);
  assert(!maxnode);


  /* convert last node nodemask/bitmask from/to cpuset */
  node = topo_get_next_obj(topology, TOPO_OBJ_NODE, NULL);
  if (node) {
    topo_cpuset_to_linux_libnuma_nodemask(topology, &node->cpuset, &nodemask);
    assert(nodemask_isset(&nodemask, node->os_index));
    nodemask_clr(&nodemask, node->os_index);
    nodemask_zero(&nodemask2);
    assert(nodemask_equal(&nodemask, &nodemask2));

    bitmask = topo_cpuset_to_linux_libnuma_bitmask(topology, &node->cpuset);
    assert(numa_bitmask_isbitset(bitmask, node->os_index));
    numa_bitmask_clearbit(bitmask, node->os_index);
    bitmask2 = numa_bitmask_alloc(1);
    assert(numa_bitmask_equal(bitmask, bitmask2));
    numa_bitmask_free(bitmask);
    numa_bitmask_free(bitmask2);

    maxnode = TOPO_BITS_PER_LONG;
    topo_cpuset_to_linux_libnuma_ulongs(topology, &node->cpuset, &mask, &maxnode);
    if (node->os_index >= TOPO_BITS_PER_LONG) {
      assert(!maxnode);
      assert(!mask);
    } else {
      assert(maxnode = node->os_index + 1);
      assert(mask == (1<<node->os_index));
    }
  }


  topo_topology_destroy(topology);
  return 0;
}
