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
 * check topo_get_obj_below_cpuset*()
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info info;
  topo_obj_t obj, root;
  int err;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "nodes:2 sockets:3 caches:4 cores:5 6");

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  err = topo_topology_get_info(topology, &info);
  if (err)
    return EXIT_FAILURE;

  /* there is no second system object */
  root = topo_get_system_obj (topology);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, 1);
  assert(!obj);

  /* first system object is the top-level object of the topology */
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, 0);
  assert(obj == topo_get_system_obj(topology));

  /* first next-object object is the top-level object of the topology */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, NULL);
  assert(obj == topo_get_system_obj(topology));
  /* there is no next object after the system object */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SYSTEM, obj);
  assert(!obj);

  /* check last proc */
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, 2*3*4*5*6-1);
  assert(obj == topo_get_obj_by_depth(topology, 5, 2*3*4*5*6-1));
  /* there is no next proc after the last one */
  obj = topo_get_next_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, obj);
  assert(!obj);


  /* check there are 20 cores below first socket */
  root = topo_get_obj_by_depth(topology, 2, 0);
  assert(topo_get_nbobjs_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CORE) == 20);

  /* check there are 12 caches below last node */
  root = topo_get_obj_by_depth(topology, 1, 1);
  assert(topo_get_nbobjs_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CACHE) == 12);


  /* check first proc of second socket */
  root = topo_get_obj_by_depth(topology, 2, 1);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_PROC, 0);
  assert(obj == topo_get_obj_by_depth(topology, 5, 4*5*6));

  /* check third core of third socket */
  root = topo_get_obj_by_depth(topology, 2, 2);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_CORE, 2);
  assert(obj == topo_get_obj_by_depth(topology, 4, 2*4*5+2));

  /* check first socket of second node */
  root = topo_get_obj_by_depth(topology, 1, 1);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_SOCKET, 0);
  assert(obj == topo_get_obj_by_depth(topology, 2, 3));

  /* there is no node below sockets */
  root = topo_get_obj_by_depth(topology, 2, 0);
  obj = topo_get_obj_below_cpuset(topology, &root->cpuset, TOPO_OBJ_NODE, 0);
  assert(!obj);

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
