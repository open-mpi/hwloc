//
// Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
// Copyright © 2013 Inria.  All rights reserved.
//
// See COPYING in top-level directory.
//
// $HEADER$
//

#include "netloc.h"
#include "netloc/map.h"

#include "stdio.h"

int main(int argc, char *argv[])
{
    netloc_map_t map;
    int verbose = 0;
    int err;

    argc--;
    argv++;
    if (argc >= 1) {
      if (!strcmp(argv[0], "--verbose")) {
	verbose = 1;
	argc--;
	argv++;
      }
    }

    err = netloc_map_create(&map);
    if (err) {
      fprintf(stderr, "Failed to create the map\n");
      return -1;
    }

    err = netloc_map_load_hwloc_data(map, NETLOC_ABS_TOP_SRCDIR "../examples/cisco2/hwloc");
    if (err) {
      fprintf(stderr, "Failed to load hwloc data\n");
      return -1;
    }

    err = netloc_map_load_netloc_data(map, "file://" NETLOC_ABS_TOP_SRCDIR "../examples/cisco2/netloc");
    if (err) {
      fprintf(stderr, "Failed to load netloc data\n");
      return -1;
    }

    err = netloc_map_build(map, NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC);
    if (err) {
      fprintf(stderr, "Failed to build map data\n");
      return -1;
    }

    if (verbose)
      netloc_map_dump(map);

    netloc_map_find_neighbors(map, "dell043", 4);

    hwloc_topology_t topo;
    netloc_map_server_t server;
    hwloc_obj_t obj, obj2;
    netloc_map_port_t ports[3], port;
    netloc_topology_t ntopos[3];
    netloc_node_t *nnodes[3];
    netloc_edge_t *nedges[3];
    const char *name;
    unsigned nr;

    err = netloc_map_name2server(map, "dell013", &server);
    assert(!err);
    err = netloc_map_server2hwloc(server, &topo);
    printf("Found %d PU in dell013\n", hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU));

    nr = 0;
    err = netloc_map_hwloc2port(map, topo, NULL, NULL, &nr);
    printf("Returned %d/%d out of %d found ports in topology\n", nr, 0, err);
    assert(err == 2);
    assert(nr == 0);
    nr = 3;
    err = netloc_map_hwloc2port(map, topo, NULL, ports, &nr);
    printf("Returned %d/%d out of %d found ports in topology\n", nr, 3, err);
    assert(err == 2);
    assert(nr == 2);

    err = netloc_map_port2netloc(ports[0], &ntopos[0], &nnodes[0], &nedges[0]);
    assert(!err);
    err = netloc_map_port2netloc(ports[1], &ntopos[1], &nnodes[1], &nedges[1]);
    assert(!err);
    printf("Found netloc node %s\n", nnodes[1]->physical_id);
    assert(ntopos[0] != ntopos[1]);
    err = strcmp(nnodes[1]->physical_id, "0002:c903:0006:6439");
    assert(!err);
    err = netloc_map_put_hwloc(map, topo);
    assert(!err);

    err = netloc_map_netloc2port(map, ntopos[0], nnodes[0], nedges[0], &port);
    assert(!err);
    err = netloc_map_port2hwloc(port, &topo, &obj);
    assert(!err);
    err = netloc_map_hwloc2server(map, topo, &server);
    assert(!err);
    err = netloc_map_server2name(server, &name);
    assert(!err);
    printf("Found name %s back\n", name);
    err = strcmp(name, "dell013");
    assert(!err);

    nr = 0;
    assert(obj->type == HWLOC_OBJ_OS_DEVICE);
    err = netloc_map_hwloc2port(map, topo, obj, NULL, &nr);
    printf("Found %d nodes under IB osdev\n", err);
    assert(err == 2);

    obj2 = obj->next_cousin;
    assert(obj2->type == HWLOC_OBJ_OS_DEVICE);
    err = netloc_map_hwloc2port(map, topo, obj2, NULL, &nr);
    printf("Found %d nodes under another osdev\n", err);
    assert(err == 0);

    obj = obj->parent;
    assert(obj->type == HWLOC_OBJ_PCI_DEVICE);
    err = netloc_map_hwloc2port(map, topo, obj, NULL, &nr);
    printf("Found %d nodes under IB PCI dev\n", err);
    assert(err == 2);

    obj2 = obj->next_cousin;
    assert(obj2->type == HWLOC_OBJ_PCI_DEVICE);
    err = netloc_map_hwloc2port(map, topo, obj2, NULL, &nr);
    printf("Found %d nodes under another PCI dev\n", err);
    assert(err == 0);

    obj = obj->parent;
    assert(obj->type == HWLOC_OBJ_BRIDGE);
    err = netloc_map_hwloc2port(map, topo, obj, NULL, &nr);
    printf("Found %d nodes under IB PCI dev parent bridge\n", err);
    assert(err == 2);

    obj = hwloc_get_non_io_ancestor_obj(topo, obj);
    assert(obj->cpuset);
    err = netloc_map_hwloc2port(map, topo, obj, NULL, &nr);
    printf("Found %d nodes under non-IO IB PCI dev parent\n", err);
    assert(err == 2);

    /* FIXME: check with edges too */

    err = netloc_map_put_hwloc(map, topo);
    assert(!err);

    netloc_map_destroy(map);

    return 0;
}
