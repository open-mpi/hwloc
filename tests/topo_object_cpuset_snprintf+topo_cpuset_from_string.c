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

/* check topo_object_cpuset_snprintf() and topo_cpuset_from_string() */

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  char string[TOPO_CPUSET_STRING_LENGTH+1];
  topo_obj_t obj;
  topo_cpuset_t set;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, "6 5 4 3 2");
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  obj = topo_get_machine_object(topology);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("machine cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  assert(topo_cpuset_isequal(&set, &obj->cpuset));
  printf("machine cpuset converted back and forth, ok\n");

  obj = topo_get_object(topology, topoinfo.depth-1, 0);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("first cpu cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  assert(topo_cpuset_isequal(&set, &obj->cpuset));
  printf("first cpu cpuset converted back and forth, ok\n");

  obj = topo_get_object(topology, topoinfo.depth-1, topo_get_depth_nbobjects(topology, topoinfo.depth-1) - 1);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("last cpu cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  assert(topo_cpuset_isequal(&set, &obj->cpuset));
  printf("last cpu cpuset converted back and forth, ok\n");

//  topo_cpuset_from_string("1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,4,8,10,20\n", &set);
//  printf("%s\n",  TOPO_CPUSET_PRINTF_VALUE(set));
//  will be truncated after ",4," since it's too large

  topo_topology_destroy(topology);

  return 0;
}
