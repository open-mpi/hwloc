/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check hwloc_obj_cpuset_snprintf() and hwloc_cpuset_from_string() */

int main()
{
  hwloc_topology_t topology;
  struct hwloc_topology_info topoinfo;
  char string[HWLOC_CPUSET_STRING_LENGTH+1];
  hwloc_obj_t obj;
  hwloc_cpuset_t set;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "6 5 4 3 2");
  hwloc_topology_load(topology);
  hwloc_topology_get_info(topology, &topoinfo);

  obj = hwloc_get_system_obj(topology);
  hwloc_obj_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("system cpuset is %s\n", string);
  hwloc_cpuset_from_string(string, &set);
  assert(hwloc_cpuset_isequal(&set, &obj->cpuset));
  printf("system cpuset converted back and forth, ok\n");

  obj = hwloc_get_obj_by_depth(topology, topoinfo.depth-1, 0);
  hwloc_obj_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("first cpu cpuset is %s\n", string);
  hwloc_cpuset_from_string(string, &set);
  assert(hwloc_cpuset_isequal(&set, &obj->cpuset));
  printf("first cpu cpuset converted back and forth, ok\n");

  obj = hwloc_get_obj_by_depth(topology, topoinfo.depth-1, hwloc_get_depth_nbobjs(topology, topoinfo.depth-1) - 1);
  hwloc_obj_cpuset_snprintf(string, sizeof(string), 1, &obj);
  printf("last cpu cpuset is %s\n", string);
  hwloc_cpuset_from_string(string, &set);
  assert(hwloc_cpuset_isequal(&set, &obj->cpuset));
  printf("last cpu cpuset converted back and forth, ok\n");

//  hwloc_cpuset_from_string("1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,4,8,10,20\n", &set);
//  printf("%s\n",  HWLOC_CPUSET_PRINTF_VALUE(&set));
//  will be truncated after ",4," since it's too large

  hwloc_topology_destroy(topology);

  return 0;
}
