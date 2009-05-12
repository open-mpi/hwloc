/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  char string[TOPO_CPUSET_STRING_LENGTH+1];
  topo_obj_t obj;
  topo_cpuset_t set;

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  obj = topo_get_machine_object(topology);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("machine cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  printf("machine cpuset is %s\n", TOPO_CPUSET_PRINTF_VALUE(set));
  assert(topo_cpuset_isequal(&set, &obj->cpuset));

  obj = topo_get_object(topology, topoinfo.depth-1, 0);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("first cpu cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  assert(topo_cpuset_isequal(&set, &obj->cpuset));

  obj = topo_get_object(topology, topoinfo.depth-1, topo_get_depth_nbitems(topology, topoinfo.depth-1) - 1);
  topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("last cpu cpuset is %s\n", string);
  topo_cpuset_from_string(string, &set);
  assert(topo_cpuset_isequal(&set, &obj->cpuset));

  return 0;
}
