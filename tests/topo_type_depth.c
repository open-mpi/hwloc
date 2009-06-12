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

#include <stdio.h>
#include <assert.h>

/* check topo_get_type{,_or_above,_or_below}_depth()
 * and topo_get_depth_type()
 */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "machine:3 misc:2 core:3 cache:2 cache:2 2"

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  assert(topoinfo.depth == 7);

  assert(topo_get_depth_type(topology, 0) == TOPO_OBJ_SYSTEM);
  assert(topo_get_depth_type(topology, 1) == TOPO_OBJ_MACHINE);
  assert(topo_get_depth_type(topology, 2) == TOPO_OBJ_MISC);
  assert(topo_get_depth_type(topology, 3) == TOPO_OBJ_CORE);
  assert(topo_get_depth_type(topology, 4) == TOPO_OBJ_CACHE);
  assert(topo_get_depth_type(topology, 5) == TOPO_OBJ_CACHE);
  assert(topo_get_depth_type(topology, 6) == TOPO_OBJ_PROC);

  assert(topo_get_type_depth(topology, TOPO_OBJ_MACHINE) == 1);
  assert(topo_get_type_depth(topology, TOPO_OBJ_MISC) == 2);
  assert(topo_get_type_depth(topology, TOPO_OBJ_CORE) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_PROC) == 6);

  assert(topo_get_type_depth(topology, TOPO_OBJ_NODE) == TOPO_TYPE_DEPTH_UNKNOWN);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_NODE) == 2);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_NODE) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_SOCKET) == TOPO_TYPE_DEPTH_UNKNOWN);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_SOCKET) == 2);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_SOCKET) == 3);
  assert(topo_get_type_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);
  assert(topo_get_type_or_above_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);
  assert(topo_get_type_or_below_depth(topology, TOPO_OBJ_CACHE) == TOPO_TYPE_DEPTH_MULTIPLE);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
