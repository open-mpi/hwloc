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

/*
 * check topo_get_next_obj_above_cpuset_by_depth()
 * and topo_get_next_obj_above_cpuset()
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  topo_cpuset_t set;
  topo_obj_t obj;
  unsigned depth;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  topo_cpuset_from_string("00008f18", &set);

  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, NULL);
  assert(obj == topo_get_obj_by_depth(topology, 1, 1));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 2));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 4));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 5));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(obj == topo_get_obj_by_depth(topology, 1, 7));
  obj = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, obj);
  assert(!obj);

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  topo_topology_set_synthetic (topology, "ndoes:2 socket:5 cores:3 4");
  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  topo_cpuset_from_string("0ff08000", &set);

  depth = topo_get_type_depth(topology, TOPO_OBJ_SOCKET);
  assert(depth == 2);
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, NULL);
  assert(obj == topo_get_obj_by_depth(topology, depth, 1));
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, obj);
  assert(obj == topo_get_obj_by_depth(topology, depth, 2));
  obj = topo_get_next_obj_above_cpuset_by_depth(topology, &set, depth, obj);
  assert(!obj);

  topo_topology_set_synthetic (topology, "nodes:8 cores:2 1");



  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
