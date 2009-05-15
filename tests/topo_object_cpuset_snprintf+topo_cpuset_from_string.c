/* Copyright 2009 INRIA, Université Bordeaux 1  */

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

  obj = topo_get_object(topology, topoinfo.depth-1, topo_get_depth_nbitems(topology, topoinfo.depth-1) - 1);
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
