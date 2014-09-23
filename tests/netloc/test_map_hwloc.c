//
// Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
// Copyright © 2013 Inria.  All rights reserved.
//
// See COPYING in top-level directory.
//
// $HEADER$
//

#define _GNU_SOURCE
#include <netloc/map.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    netloc_map_t map;
    netloc_map_server_t server;
    char *path;
    int err;

    if (argc < 4) {
      fprintf(stderr, "%s <datadir> <hostname> <depth>\n", argv[0]);
      fprintf(stderr, "Example: %s examples/cisco2 dell043 2\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    err = netloc_map_create(&map);
    if (err) {
      fprintf(stderr, "Failed to create the map\n");
      exit(EXIT_FAILURE);
    }

    asprintf(&path, "%s/hwloc", argv[1]);
    err = netloc_map_load_hwloc_data(map, path);
    free(path);
    if (err) {
      fprintf(stderr, "Failed to load hwloc data\n");
      return -1;
    }

    asprintf(&path, "file://%s/netloc", argv[1]);
    err = netloc_map_load_netloc_data(map, path);
    free(path);
    if (err) {
      fprintf(stderr, "Failed to load netloc data\n");
      return -1;
    }

    err = netloc_map_build(map, NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC);
    if (err) {
      fprintf(stderr, "Failed to build map data\n");
      return -1;
    }

    hwloc_topology_t topo;
    netloc_map_name2server(map, argv[2], &server);
    netloc_map_server2hwloc(server, &topo);
    netloc_map_put_hwloc(map, topo);
    netloc_map_name2server(map, argv[2], &server);
    netloc_map_server2hwloc(server, &topo);
    netloc_map_put_hwloc(map, topo);

    netloc_map_find_neighbors(map, argv[2], atoi(argv[3]));

    netloc_map_destroy(map);

    return 0;
}
