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

/* check topo_get_cpuset_objs() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

#if TOPO_BITS_PER_LONG == 32
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */
#define GIVEN_HARD_CPUSET_STRING "07ff,ffffffff,e0000000"
#elif TOPO_BITS_PER_LONG == 64
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */
#define GIVEN_HARD_CPUSET_STRING "07ff,ffffffffe0000000"
#endif

#define OBJ_MAX 16

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t objs[OBJ_MAX];
  topo_obj_t obj;
  topo_cpuset_t set;
  int ret;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  /* just get the system object */
  obj = topo_get_system_obj(topology);
  ret = topo_get_cpuset_objs(topology, &obj->cpuset, objs, 1);
  assert(ret == 1);
  assert(objs[0] == obj);

  /* just get the very last object */
  obj = topo_get_obj(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1)-1);
  ret = topo_get_cpuset_objs(topology, &obj->cpuset, objs, 1);
  assert(ret == 1);
  assert(objs[0] == obj);

  /* try an empty one */
  topo_cpuset_zero(&set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == 0);

  /* try an impossible one */
  topo_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == -1);

  /* try a harder one with 1 obj instead of 2 needed */
  topo_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, 1);
  assert(ret == 1);
  assert(objs[0] == topo_get_obj(topology, topoinfo.depth-1, 0));
  /* try a harder one with lots of objs instead of 2 needed */
  ret = topo_get_cpuset_objs(topology, &set, objs, 2);
  assert(ret == 2);
  assert(objs[0] == topo_get_obj(topology, topoinfo.depth-1, 0));
  assert(objs[1] == topo_get_obj(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1)-1));

  /* try a very hard one */
  topo_cpuset_from_string(GIVEN_HARD_CPUSET_STRING, &set);
  ret = topo_get_cpuset_objs(topology, &set, objs, OBJ_MAX);
  assert(objs[0] == topo_get_obj(topology, 5, 29));
  assert(objs[1] == topo_get_obj(topology, 3, 5));
  assert(objs[2] == topo_get_obj(topology, 3, 6));
  assert(objs[3] == topo_get_obj(topology, 3, 7));
  assert(objs[4] == topo_get_obj(topology, 2, 2));
  assert(objs[5] == topo_get_obj(topology, 4, 36));
  assert(objs[6] == topo_get_obj(topology, 5, 74));

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
