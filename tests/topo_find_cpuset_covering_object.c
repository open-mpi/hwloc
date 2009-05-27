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

/* check topo_find_cpuset_covering_object() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

#if TOPO_BITS_PER_LONG == 32
#define GIVEN_CPUSET_STRING "0,0fff,f0000000"
#define EXPECTED_CPUSET_STRING "0000ffff,ff000000"
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */
#elif TOPO_BITS_PER_LONG == 64
#define GIVEN_CPUSET_STRING "0,,0ffff0000000"
#define EXPECTED_CPUSET_STRING "0000ffffff000000"
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */
#endif

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  char string[TOPO_CPUSET_STRING_LENGTH+1];
  topo_obj_t obj;
  topo_cpuset_t set;

  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  topo_cpuset_from_string(GIVEN_CPUSET_STRING, &set);
  obj = topo_find_cpuset_covering_object(topology, &set);

  assert(obj);
  fprintf(stderr, "found covering object type %s covering cpuset %s\n",
	  topo_object_type_string(obj->type), GIVEN_CPUSET_STRING);
  assert(topo_cpuset_isincluded(&set, &obj->cpuset));

  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  fprintf(stderr, "covering object of %s is %s, expected %s\n",
	  GIVEN_CPUSET_STRING, string, EXPECTED_CPUSET_STRING);
  assert(!strcmp(EXPECTED_CPUSET_STRING, string));

  topo_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING, &set);
  obj = topo_find_cpuset_covering_object(topology, &set);
  assert(obj == topo_get_machine_object(topology));
  fprintf(stderr, "found machine as covering object of first+last cpus cpuset %s\n",
	  GIVEN_LARGESPLIT_CPUSET_STRING);

  topo_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING, &set);
  obj = topo_find_cpuset_covering_object(topology, &set);
  assert(!obj);
  fprintf(stderr, "found no covering object for too-large cpuset %s\n",
	  GIVEN_TOOLARGE_CPUSET_STRING);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
