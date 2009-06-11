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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_get_shared_cache_above() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION_SHARED "6 5 4 3 2" /* 736bits wide topology */
#define SYNTHETIC_TOPOLOGY_DESCRIPTION_NONSHARED "6 5 4 1 2" /* 736bits wide topology */

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t obj, cache;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION_SHARED);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  obj = topo_get_obj(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX/2/3);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check no shared cache above the L2 cache */
  obj = topo_get_obj(topology, 3, 0);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  /* check no shared cache above the node */
  obj = topo_get_obj(topology, 1, 0);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);


  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION_NONSHARED);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* check the cache above a given cpu */
#define CPUINDEX 180
  obj = topo_get_obj(topology, 5, CPUINDEX);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(cache);
  assert(cache->type == TOPO_OBJ_CACHE);
  assert(cache->logical_index == CPUINDEX/2/1);
  assert(topo_obj_is_in_subtree(obj, cache));

  /* check no shared-cache above the core */
  obj = topo_get_obj(topology, 4, CPUINDEX/2);
  assert(obj);
  cache = topo_get_shared_cache_above(topology, obj);
  assert(!cache);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
