/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check that object userdata is properly initialized */

static void check(topo_topology_t topology)
{
  struct topo_topology_info topoinfo;
  unsigned i,j;

  topo_topology_get_info(topology, &topoinfo);
  for(i=0; i<topoinfo.depth; i++) {
    for(j=0; j<topo_get_depth_nbobjects(topology, i); j++) {
      assert(topo_get_object(topology, i, j)->userdata == NULL);
    }
  }
}

int main(void)
{
  topo_topology_t topology;

  /* check the real topology */
  topo_topology_init(&topology);
  topo_topology_load(topology);
  check(topology);
  topo_topology_destroy(topology);

  /* check a synthetic topology */
  topo_topology_init(&topology);
  topo_topology_set_synthetic(topology, "6 5 4 3 2");
  topo_topology_load(topology);
  check(topology);
  topo_topology_destroy(topology);

  return 0;
}
