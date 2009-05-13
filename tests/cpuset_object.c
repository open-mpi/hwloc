/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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
  obj = topo_find_cpuset_ancestor_object(topology, &set);

  assert(obj);
  fprintf(stderr, "found ancestor type %s covering cpuset %s\n",
	  topo_object_type_string(obj->type), GIVEN_CPUSET_STRING);
  assert(topo_cpuset_isincluded(&obj->cpuset, &set));

  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  fprintf(stderr, "ancestor of %s is %s, expected %s\n",
	  GIVEN_CPUSET_STRING, string, EXPECTED_CPUSET_STRING);
  assert(!strcmp(EXPECTED_CPUSET_STRING, string));

  topo_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING, &set);
  obj = topo_find_cpuset_ancestor_object(topology, &set);
  assert(obj == topo_get_machine_object(topology));
  fprintf(stderr, "found machine as ancestor of first+last cpus cpuset %s\n",
	  GIVEN_LARGESPLIT_CPUSET_STRING);

  topo_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING, &set);
  obj = topo_find_cpuset_ancestor_object(topology, &set);
  assert(!obj);
  fprintf(stderr, "found no ancestor for too-large cpuset %s\n",
	  GIVEN_TOOLARGE_CPUSET_STRING);

  return EXIT_SUCCESS;
}
