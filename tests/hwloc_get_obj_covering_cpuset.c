/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check hwloc_get_obj_covering_cpuset() */

#define SYNTHETIC_TOPOLOGY_DESCRIPTION "6 5 4 3 2" /* 736bits wide topology */

#define GIVEN_CPUSET_STRING "0,0fff,f0000000"
#define EXPECTED_CPUSET_STRING "0000ffff,ff000000"
#define GIVEN_LARGESPLIT_CPUSET_STRING "8000,,,,,,,,,,,,,,,,,,,,,,1" /* first and last(735th) bit set */
#define GIVEN_TOOLARGE_CPUSET_STRING "10000,,,,,,,,,,,,,,,,,,,,,,0" /* 736th bit is too large for the 720-wide topology */

int main()
{
  hwloc_topology_t topology;
  char *string = NULL;
  hwloc_obj_t obj;
  hwloc_cpuset_t set;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, SYNTHETIC_TOPOLOGY_DESCRIPTION);
  hwloc_topology_load(topology);

  set = hwloc_cpuset_from_string(GIVEN_CPUSET_STRING);
  obj = hwloc_get_obj_covering_cpuset(topology, set);

  assert(obj);
  fprintf(stderr, "found covering object type %s covering cpuset %s\n",
	  hwloc_obj_type_string(obj->type), GIVEN_CPUSET_STRING);
  assert(hwloc_cpuset_isincluded(set, obj->cpuset));

  hwloc_cpuset_asprintf(&string, obj->cpuset);
  fprintf(stderr, "covering object of %s is %s, expected %s\n",
	  GIVEN_CPUSET_STRING, string, EXPECTED_CPUSET_STRING);
  assert(!strcmp(EXPECTED_CPUSET_STRING, string));
  free(string);
  free(set);

  set = hwloc_cpuset_from_string(GIVEN_LARGESPLIT_CPUSET_STRING);
  obj = hwloc_get_obj_covering_cpuset(topology, set);
  assert(obj == hwloc_get_system_obj(topology));
  fprintf(stderr, "found system as covering object of first+last cpus cpuset %s\n",
	  GIVEN_LARGESPLIT_CPUSET_STRING);
  free(set);

  set = hwloc_cpuset_from_string(GIVEN_TOOLARGE_CPUSET_STRING);
  obj = hwloc_get_obj_covering_cpuset(topology, set);
  assert(!obj);
  fprintf(stderr, "found no covering object for too-large cpuset %s\n",
	  GIVEN_TOOLARGE_CPUSET_STRING);
  free(set);

  hwloc_topology_destroy(topology);

  return EXIT_SUCCESS;
}
