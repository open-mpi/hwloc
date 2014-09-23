/*
 * Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2013 Inria.  All rights reserved.
 * Copyright © 2014      University of Wisconsin-La Crosse.
 *                         All rights reserved.
 *
 * See COPYING in top-level directory.
 *
 * $HEADER$
 *
 * Example: ./lsmap data/
 */

#include "netloc.h"
#include "netloc/map.h"
#include "hwloc.h"

#include "stdio.h"

int main(int argc, char *argv[])
{
    netloc_map_t map;
    netloc_topology_t *ntopos;
    netloc_map_server_t *servers;
    char *datadir, *path;
    unsigned nr_ntopos, nr_servers, i, j;
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

    if (argc < 1) {
        fprintf(stderr, "Missing data directory argument\n");
        return -1;
    }
    datadir = argv[0];
    argv++;
    argc--;

    err = netloc_map_create(&map);
    if (err) {
        fprintf(stderr, "Failed to create the map\n");
        return -1;
    }

    asprintf(&path, "%s/hwloc", datadir);
    err = netloc_map_load_hwloc_data(map, path);
    if (err) {
        fprintf(stderr, "Failed to load hwloc data from %s\n", path);
        return -1;
    }
    free(path);

    asprintf(&path, "file://%s/netloc", datadir);
    err = netloc_map_load_netloc_data(map, path);
    if (err) {
        fprintf(stderr, "Failed to load netloc data from %s\n", path);
        return -1;
    }
    free(path);

    err = netloc_map_build(map, NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC);
    if (err) {
        fprintf(stderr, "Failed to build map data\n");
        return -1;
    }

    if (verbose)
        netloc_map_dump(map);

    err = netloc_map_get_subnets(map, &nr_ntopos, &ntopos);
    if (err) {
        fprintf(stderr, "Failed to get the number of subnets\n");
        return -1;
    }

    for (i = 0; i < nr_ntopos; i++) {
        netloc_topology_t ntopo = ntopos[i];
        netloc_network_t *net = netloc_access_network_ref(ntopo);
        const char *type = netloc_decode_network_type(net->network_type);

        printf("Subnet type %s id %s\n", type, net->subnet_id);

        netloc_dt_lookup_table_t nodes;
        err = netloc_get_all_host_nodes(ntopo, &nodes);
        if (err)
            continue;

        netloc_dt_lookup_table_iterator_t iter =
            netloc_dt_lookup_table_iterator_t_construct(nodes);
        while (!netloc_lookup_table_iterator_at_end(iter)) {
            const char *key;
            netloc_node_t *nnode;
            netloc_map_port_t port;
            netloc_map_server_t server;
            hwloc_topology_t htopo;
            hwloc_obj_t hobj;
            const char *servername;

            key = netloc_lookup_table_iterator_next_key(iter);
            if (NULL == key)
                break;

            nnode =
                (netloc_node_t *) netloc_lookup_table_access(nodes, key);
            if (NETLOC_NODE_TYPE_INVALID == nnode->node_type)
                continue;

            err = netloc_map_netloc2port(map, ntopo, nnode, NULL, &port);
            if (err < 0)
                continue;

            err = netloc_map_port2hwloc(port, &htopo, &hobj);
            if (err < 0)
                continue;

            err = netloc_map_port2server(port, &server);
            if (err < 0)
                continue;

            err = netloc_map_server2name(server, &servername);
            if (err < 0)
                continue;

            printf(" node %s connected through object %s address %s\n",
                   servername, hobj->name, nnode->physical_id);

            netloc_map_put_hwloc(map, htopo);
        }

        netloc_dt_lookup_table_iterator_t_destruct(iter);
        netloc_lookup_table_destroy(nodes);
        free(nodes);
    }

    err = netloc_map_get_nbservers(map);
    if (err < 0) {
        fprintf(stderr, "Failed to get the number of servers\n");
        return -1;
    }
    nr_servers = err;

    servers = malloc(nr_servers * sizeof(*servers));
    if (!servers) {
        fprintf(stderr, "Failed to allocate servers array\n");
        return -1;
    }

    err = netloc_map_get_servers(map, 0, nr_servers, servers);
    if (err < 0) {
        fprintf(stderr, "Failed to get servers\n");
        return -1;
    }
    assert((unsigned) err == nr_servers);

    for (i = 0; i < nr_servers; i++) {
        const char *servername = "unknown";
        netloc_map_port_t *ports;
        unsigned ports_nr;

        err = netloc_map_server2name(servers[i], &servername);
        assert(!err);
        printf("Server %s\n", servername);

        err = netloc_map_get_server_ports(servers[i], &ports_nr, &ports);
        assert(!err);

        if (!ports)
            continue;

        for (j = 0; j < ports_nr; j++) {
            netloc_topology_t ntopo;
            netloc_network_t *net;
            netloc_node_t *nnode;
            netloc_edge_t *nedge;
            hwloc_topology_t htopo;
            hwloc_obj_t hobj;

            err = netloc_map_port2netloc(ports[j], &ntopo, &nnode, &nedge);
            assert(!err);
            net = netloc_access_network_ref(ntopo);

            err = netloc_map_port2hwloc(ports[j], &htopo, &hobj);
            assert(!err);

            const char *type =
                netloc_decode_network_type(net->network_type);
            printf(" port %s of device %s (id %s)\n"
                   "  on subnet type %s id %s\n"
                   "   towards node %s port %s\n",
                   nedge ? nedge->src_port_id : "<unknown>", hobj->name,
                   nnode ? nnode->physical_id : "<none>", type,
                   net->subnet_id, nedge ? nedge->dest_node_id : "<none>",
                   nedge ? nedge->dest_port_id : "<unknown>");

            netloc_map_put_hwloc(map, htopo);
        }
    }

    free(servers);
    free(ntopos);
    netloc_map_destroy(map);

    return 0;
}
