//
// Copyright © 2013-2016 Inria.  All rights reserved.
//
// Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
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
  netloc_map_server_t srcserver, dstserver;
  hwloc_topology_t srctopo, dsttopo;
  hwloc_obj_t srcobj, dstobj;
  netloc_map_paths_t paths;
  unsigned nr_paths, nr_edges, i, j;
  struct netloc_map_edge_s *edges;
  unsigned flags = 0x3;
  char *path;
  int err;

  if (argc > 2) {
    if (!strcmp(argv[1], "--flags")) {
      flags = (unsigned) strtoul(argv[2], NULL, 0);
      argc -= 2;
      argv += 2;
    }
  }

  if (argc < 6) {
    fprintf(stderr, "%s [options] <datadir> <srcserver> <srcpu> <dstserver> <dstpu>\n", argv[0]);
    fprintf(stderr, "Example: %s mynetlocdata server2 1 server3 7\n", argv[0]);
    fprintf(stderr, "  Loads netloc map from 'mynetlocdata' directory and display all paths\n");
    fprintf(stderr, "  from server 'server2' PU #1 to server 'server3' PU #7.\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --flags N    Use value N as map paths flags. Default is 3 which displays all edges.\n");
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

  err = netloc_map_build(map, 0);
  if (err) {
    fprintf(stderr, "Failed to build map data\n");
    return -1;
  }

  err = netloc_map_name2server(map, argv[2], &srcserver);
  if (err) {
    fprintf(stderr, "Could not find src server %s\n", argv[2]);
    return -1;
  }
  err = netloc_map_server2hwloc(srcserver, &srctopo);
  if (err) {
    fprintf(stderr, "Could not find src server %s hwloc topology\n", argv[2]);
    return -1;
  }
  srcobj = hwloc_get_obj_by_type(srctopo, HWLOC_OBJ_PU, atoi(argv[3]));
  if (!srcobj) {
    fprintf(stderr, "Could not find src server %s PU #%s\n", argv[2], argv[3]);
    return -1;
  }

  err = netloc_map_name2server(map, argv[4], &dstserver);
  if (err) {
    fprintf(stderr, "Could not find dst server %s\n", argv[4]);
    return -1;
  }
  err = netloc_map_server2hwloc(dstserver, &dsttopo);
  if (err) {
    fprintf(stderr, "Could not find dst server %s hwloc topology\n", argv[4]);
    return -1;
  }
  dstobj = hwloc_get_obj_by_type(dsttopo, HWLOC_OBJ_PU, atoi(argv[5]));
  if (!dstobj) {
    fprintf(stderr, "Could not find dst server %s PU #%s\n", argv[4], argv[5]);
    return -1;
  }

  err = netloc_map_paths_build(map,
			       srctopo, srcobj,
			       dsttopo, dstobj,
			       flags,
			       &paths, &nr_paths);
  if (err < 0) {
    fprintf(stderr, "Failed to build paths\n");
    return -1;
  }

  printf("got %u paths\n", nr_paths);
  for(i=0; i<nr_paths; i++) {
    printf(" path #%u:\n", i);

    err = netloc_map_paths_get(paths, i, &edges, &nr_edges);
    assert(!err);
    printf("  %u edges\n", nr_edges);
    for(j=0; j<nr_edges; j++) {
      struct netloc_map_edge_s *edge = &edges[j];
      printf("   edge #%u type %d: ", j, edge->type);
      switch (edge->type) {
      case NETLOC_MAP_EDGE_TYPE_NETLOC:
        printf("netloc from %s to %s in subnet type %s id %s\n",
	       edge->netloc.edge->src_node_id,
	       edge->netloc.edge->dest_node_id,
	       netloc_network_type_decode(netloc_topology_get_network(edge->netloc.topology)->network_type),
	       netloc_topology_get_network(edge->netloc.topology)->subnet_id);
	break;
      case NETLOC_MAP_EDGE_TYPE_HWLOC_PARENT:
	printf("hwloc UP from %s:%u (%s) to parent %s:%u (%s) weight %u\n",
	       hwloc_type_name(edge->hwloc.src_obj->type), edge->hwloc.src_obj->logical_index, edge->hwloc.src_obj->name ? : "<unnamed>",
	       hwloc_type_name(edge->hwloc.dest_obj->type), edge->hwloc.dest_obj->logical_index, edge->hwloc.dest_obj->name ? : "<unnamed>",
	       edge->hwloc.weight);
	break;
      case NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL:
	printf("hwloc HORIZONTAL from %s:%u (%s) to cousin %s:%u (%s) weight %u\n",
	       hwloc_type_name(edge->hwloc.src_obj->type), edge->hwloc.src_obj->logical_index, edge->hwloc.src_obj->name ? : "<unnamed>",
	       hwloc_type_name(edge->hwloc.dest_obj->type), edge->hwloc.dest_obj->logical_index, edge->hwloc.dest_obj->name ? : "<unnamed>",
	       edge->hwloc.weight);
	break;
      case NETLOC_MAP_EDGE_TYPE_HWLOC_CHILD:
	printf("hwloc DOWN from %s:%u (%s) to child %s:%u (%s) weight %u\n",
	       hwloc_type_name(edge->hwloc.src_obj->type), edge->hwloc.src_obj->logical_index, edge->hwloc.src_obj->name ? : "<unnamed>",
	       hwloc_type_name(edge->hwloc.dest_obj->type), edge->hwloc.dest_obj->logical_index, edge->hwloc.dest_obj->name ? : "<unnamed>",
	       edge->hwloc.weight);
	break;
      case NETLOC_MAP_EDGE_TYPE_HWLOC_PCI:
	printf("hwloc PCI from %s:%u (%s) to child %s:%u (%s) weight %u\n",
	       hwloc_type_name(edge->hwloc.src_obj->type), edge->hwloc.src_obj->logical_index, edge->hwloc.src_obj->name ? : "<unnamed>",
	       hwloc_type_name(edge->hwloc.dest_obj->type), edge->hwloc.dest_obj->logical_index, edge->hwloc.dest_obj->name ? : "<unnamed>",
	       edge->hwloc.weight);
	break;
      }
    }
  }

  netloc_map_paths_destroy(paths);

  netloc_map_put_hwloc(map, srctopo);
  netloc_map_put_hwloc(map, dsttopo);
  netloc_map_destroy(map);
  return 0;
}
