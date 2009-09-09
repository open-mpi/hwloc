/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_get_cpuset_covering_obj() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

#define GIVEN_CPUSET_STRING "0,0fff,f0000000"
#define EXPECTED_CPUSET_STRING "0000ffff,ff000000"
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */

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
  obj = topo_get_cpuset_covering_obj(topology, &set);

  assert(obj);
  fprintf(stderr, "found covering object type %s covering cpuset %s\n",
	  topo_obj_type_string(obj->type), GIVEN_CPUSET_STRING);
  assert(topo_cpuset_isincluded(&set, &obj->cpuset));

  topo_obj_cpuset_snprintf(string, sizeof(string), 1, &obj);
  fprintf(stderr, "covering object of %s is %s, expected %s\n",
	  GIVEN_CPUSET_STRING, string, EXPECTED_CPUSET_STRING);
  assert(!strcmp(EXPECTED_CPUSET_STRING, string));

  topo_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING, &set);
  obj = topo_get_cpuset_covering_obj(topology, &set);
  assert(obj == topo_get_system_obj(topology));
  fprintf(stderr, "found system as covering object of first+last cpus cpuset %s\n",
	  GIVEN_LARGESPLIT_CPUSET_STRING);

  topo_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING, &set);
  obj = topo_get_cpuset_covering_obj(topology, &set);
  assert(!obj);
  fprintf(stderr, "found no covering object for too-large cpuset %s\n",
	  GIVEN_TOOLARGE_CPUSET_STRING);

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
