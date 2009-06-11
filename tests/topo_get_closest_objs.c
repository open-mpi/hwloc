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
 * check topo_get_closest_objs()
 *
 * - get the last object of the last level
 * - get all closest objects
 * - get the common ancestor of last level and its less close object.
 * - check that the ancestor is the system level
 */

int
main (int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info info;
  topo_obj_t last;
  topo_obj_t *closest;
  int found;
  int err;
  unsigned numprocs;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  topo_topology_set_synthetic (topology, "2 3 4 5");

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  err = topo_topology_get_info(topology, &info);
  if (err)
    return EXIT_FAILURE;

  /* get the last object of last level */
  numprocs =  topo_get_depth_nbobjs(topology, info.depth-1);
  last = topo_get_obj(topology, info.depth-1, numprocs-1);

  /* allocate the array of closest objects */
  closest = malloc(numprocs * sizeof(*closest));
  assert(closest);

  /* get closest levels */
  found = topo_get_closest_objs (topology, last, closest, numprocs);
  printf("looked for %u closest entries, found %d\n", numprocs, found);
  assert(found == numprocs-1);

  /* check first found is closest */
  assert(closest[0] == topo_get_obj(topology, info.depth-1, numprocs-5 /* arity is 5 on last level */));
  /* check some other expected positions */
  assert(closest[found-1] == topo_get_obj(topology, info.depth-1, 1*3*4*5-1 /* last of first half */));
  assert(closest[found/2-1] == topo_get_obj(topology, info.depth-1, 1*3*4*5+2*4*5-1 /* last of second third of second half */));
  assert(closest[found/2/3-1] == topo_get_obj(topology, info.depth-1, 1*3*4*5+2*4*5+3*5-1 /* last of third quarter of third third of second half */));

  /* get ancestor of last and less close object */
  topo_obj_t ancestor = topo_get_common_ancestor_obj(last, closest[found-1]);
  assert(topo_obj_is_in_subtree(last, ancestor));
  assert(topo_obj_is_in_subtree(closest[found-1], ancestor));
  assert(ancestor == topo_get_system_obj(topology));
  printf("ancestor type %u depth %u number %u is system level\n",
	 ancestor->type, ancestor->depth, ancestor->logical_index);

  free(closest);
  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
