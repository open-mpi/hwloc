/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <assert.h>
#include <hwloc.h>
#include <hwloc/plpa.h>

/* check the PLPA wrappers */

int main(void)
{
  hwloc_topology_t topology;
  hwloc_plpa_api_type_t api_type;
  int value, value2;
  int err;

  err = hwloc_plpa_init(&topology);
  assert(!err);

  /* reload a fake topology */
  hwloc_topology_set_synthetic(topology, "socket:3 core:3 proc:2");
  hwloc_topology_load(topology);

  /* there should be no binding support for synthetic topology */
  err = hwloc_plpa_api_probe(topology, &api_type);
  assert(!err);
  assert(api_type == HWLOC_PLPA_PROBE_NOT_SUPPORTED);  

  /* there should be topology information for synthetic topology */
  err = hwloc_plpa_have_topology_information(topology, &value);
  assert(!err);
  assert(value == 1);

  /* core#4 in socket#1 contains processors #8 and #9 */
  err = hwloc_plpa_map_to_processor_id(topology, 1, 4, &value);
  assert(!err);
  assert(value == 8);

  /* core#4 is not in socket#2 */
  err = hwloc_plpa_map_to_processor_id(topology, 2, 4, &value);
  assert(err == -1);

  /* processor#14 in in core#7 in socket #2 */
  err = hwloc_plpa_map_to_socket_core(topology, 14, &value, &value2);
  assert(!err);
  assert(value == 2);
  assert(value2 == 7);

  /* processor#45 does not exist */
  err = hwloc_plpa_map_to_socket_core(topology, 45, &value, &value2);
  assert(err == -1);

  /* there are 18 processors, from 0 to 17 */
  err = hwloc_plpa_get_processor_data(topology, &value, &value2);
  assert(!err);
  assert(value == 18);
  assert(value2 == 17);

  /* processor logical#4 is #4 */
  err = hwloc_plpa_get_processor_id(topology, 4, &value);
  assert(!err);
  assert(value == 4);

  /* processor logical#34 does not exist */
  err = hwloc_plpa_get_processor_id(topology, 34, &value);
  assert(err == -1);

  /* processor#4 exists and is online */
  err = hwloc_plpa_get_processor_flags(topology, 4, &value, &value2);
  assert(!err);
  assert(value == 1);
  assert(value2 == 1);

  /* there are 3 sockets, from 0 to 2 */
  err = hwloc_plpa_get_socket_info(topology, &value, &value2);
  assert(!err);
  assert(value == 3);
  assert(value2 == 2);

  /* socket logical#2 is #2 */
  err = hwloc_plpa_get_socket_id(topology, 2, &value);
  assert(!err);
  assert(value == 2);

  /* socket logical#3 does not exist */
  err = hwloc_plpa_get_socket_id(topology, 3, &value);
  assert(err == -1);

  /* there are 3 cores in socket#1, from 3 to 5 */
  err = hwloc_plpa_get_core_info(topology, 1, &value, &value2);
  assert(!err);
  assert(value == 3);
  assert(value2 == 5);

  /* core logical#2 in socket #2 is #8 */
  err = hwloc_plpa_get_core_id(topology, 2, 2, &value);
  assert(!err);
  assert(value == 8);

  /* core logical#4 in socket #1 does not exist */
  err = hwloc_plpa_get_core_id(topology, 1, 4, &value);
  assert(err ==  -1);

  /* core#7 in socket#2 exists */
  err = hwloc_plpa_get_core_flags(topology, 2, 7, &value);
  assert(!err);
  assert(value == 1);

  /* core#13 in socket#1 does not exist */
  err = hwloc_plpa_get_core_flags(topology, 1, 13, &value);
  assert(!err);
  assert(!value);

  err = hwloc_plpa_finalize(topology);
  assert(!err);

  return 0;
}
