/*
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>

#include <jansson.h>

#include "private/netloc.h"
#include "netloc.h"

#define JSON_DRAW_FILE_LINK_ID "id"
#define JSON_DRAW_FILE_LINK_SRC "from"
#define JSON_DRAW_FILE_LINK_SRC_PORT "src_port"
#define JSON_DRAW_FILE_LINK_DST "to"
#define JSON_DRAW_FILE_LINK_DST_PORT "dst_port"
#define JSON_DRAW_FILE_LINK_WIDTH "width"
#define JSON_DRAW_FILE_LINK_SPEED "speed"
#define JSON_DRAW_FILE_LINK_GBITS "gbits"
#define JSON_DRAW_FILE_LINK_OTHER_WAY "reverse"
#define JSON_DRAW_FILE_LINK_PARTITIONS "part"
#define JSON_DRAW_FILE_EDGE_ID "id"
#define JSON_DRAW_FILE_EDGE_SRC "from"
#define JSON_DRAW_FILE_EDGE_DST "to"
#define JSON_DRAW_FILE_EDGE_GBITS "gbits"
#define JSON_DRAW_FILE_EDGE_LINKS "links"
#define JSON_DRAW_FILE_EDGE_PARTITIONS "part"
#define JSON_DRAW_FILE_EDGE_SUBEDGES "subedges"
#define JSON_DRAW_FILE_EDGE_OTHER_WAY "reverse"
#define JSON_DRAW_FILE_NODE_ID "id"
#define JSON_DRAW_FILE_NODE_EDGES "edges"
#define JSON_DRAW_FILE_NODE_MERGED "merged"
#define JSON_DRAW_FILE_NODE_SUBNODES "sub"
#define JSON_DRAW_FILE_NODE_PARTITIONS "part"
#define JSON_DRAW_FILE_NODE_DESC "desc"
#define JSON_DRAW_FILE_NODE_HOSTNAME "hostname"
#define JSON_DRAW_FILE_NODE_HWLOCTOPO "topo"
#define JSON_DRAW_FILE_NODE_EDGES "edges"
#define JSON_DRAW_FILE_NODE_TYPE "type"
#define JSON_DRAW_FILE_EDGES_LIST "list"
#define JSON_DRAW_FILE_PATH_ID "id"
#define JSON_DRAW_FILE_PATH_LINKS "links"
#define JSON_DRAW_FILE_PATHS "paths"

#define JSON_DRAW_FILE_GRAPH_TYPE "type"
#define JSON_DRAW_FILE_NODES "nodes"
#define JSON_DRAW_FILE_EDGES "edges"
#define JSON_DRAW_FILE_LINKS "links"
#define JSON_DRAW_FILE_PARTITIONS "partitions"
#define JSON_DRAW_FILE_HWLOCTOPOS "hwloctopos"

int netloc_read_hwloc(netloc_topology_t topology)
{
    // TODO
    return 0;
}

static char *remove_quote(char *string)
{
    if (string[0] == '\'')
        return strndup(string+1, strlen(string)-2);
    else
        return strndup(string, strlen(string));
}

static int handle_link(netloc_physical_link_t *link, json_t *json_links)
{
    //netloc_node_t *src, *dest;
    char *src = link->src->physical_id;
    char *dest = link->dest->physical_id;

    json_t *json_link = json_object();
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_ID,        json_integer(link->id));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_SRC,       json_string(src));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_SRC_PORT,  json_integer(link->ports[0]));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_DST,       json_string(dest));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_DST_PORT,  json_integer(link->ports[1]));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_WIDTH,     json_string(link->width));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_SPEED,     json_string(link->speed));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_GBITS,     json_real(link->gbits));
    json_object_set_new(json_link,
            JSON_DRAW_FILE_LINK_OTHER_WAY, json_integer(link->other_way_id));

    json_t *json_partitions = json_array();

    for (int p = 0; p < netloc_get_num_partitions(link); p++)
    {
        int partition = netloc_get_partition(link, p);
        json_array_append_new(json_partitions, json_integer(partition));
    }
    json_object_set_new(json_link, JSON_DRAW_FILE_LINK_PARTITIONS, json_partitions);

    json_array_append_new(json_links, json_link);

    return 0;
}

static int handle_edge(netloc_edge_t *edge, json_t *json_edges)
{
    //netloc_node_t *src, *dest;
    char *src = edge->node->physical_id;
    char *dest = edge->dest->physical_id;

    json_t *json_edge = json_object();

    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_ID, json_integer(edge->id));
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_SRC, json_string(src));
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_DST, json_string(dest));
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_GBITS, json_real(edge->total_gbits));
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_OTHER_WAY, json_integer(edge->other_way->id));

    /* Links */
    json_t *json_links = json_array();
    for (int l = 0; l < netloc_edge_get_num_links(edge); l++)
    {
        netloc_physical_link_t *link = netloc_edge_get_link(edge, l);
        json_array_append_new(json_links, json_integer(link->id));
    }
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_LINKS, json_links);

    /* Partition list */
    json_t *json_partitions = json_array();
    for (int p = 0; p < netloc_get_num_partitions(edge); p++)
    {
        int partition = netloc_get_partition(edge, p);
        json_array_append_new(json_partitions, json_integer(partition));
    }
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_PARTITIONS, json_partitions);

    /* Subnode edges */
    json_t *json_subedges = json_array();
    for (int s = 0; s < netloc_edge_get_num_subedges(edge); s++)
    {
        netloc_edge_t *subedge = netloc_edge_get_subedge(edge, s);
        json_array_append_new(json_subedges, json_integer(subedge->id));
        handle_edge(subedge, json_edges);
    }
    json_object_set_new(json_edge, JSON_DRAW_FILE_EDGE_SUBEDGES, json_subedges);

    json_array_append_new(json_edges, json_edge);

    return 0;
}

static int handle_node(netloc_node_t *node, json_t *json_nodes, json_t *json_edges, int merged)
{
    //unsigned long uid = node->physical_id_int;
    //char *node_label = node->physical_id;
    char *id = node->physical_id;
    char *desc = remove_quote(node->description);
    char *hostname = node->hostname;
    //int topoIdx = node->hwlocTopoIdx; 
    int topoIdx = 0; //  TODO XXX

    json_t *json_node = json_object();
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_ID, json_string(id));
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_DESC, json_string(desc));
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_HOSTNAME, json_string(hostname));
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_HWLOCTOPO, json_integer(topoIdx));
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_MERGED, json_integer(merged));

    /* Subnodes */
    json_t *json_subnodes = json_array();
    for (int s = 0; s < netloc_node_get_num_subnodes(node); s++)
    {
        netloc_node_t *subnode = netloc_node_get_subnode(node, s);
        handle_node(subnode, json_nodes, json_edges, 1);
        json_array_append_new(json_subnodes, json_string(subnode->physical_id));
    }
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_SUBNODES, json_subnodes);

    /* Edges */
    json_t *json_edge_ids = json_array();
    netloc_edge_t *edge, *edge_tmp;
    netloc_node_iter_edges(node, edge, edge_tmp) {
        json_array_append_new(json_edge_ids, json_integer(edge->id));
        handle_edge(edge, json_edges);
    }
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_EDGES, json_edge_ids);

    /* Partitions */
    json_t *json_partitions = json_array();
    for (int p = 0; p < netloc_get_num_partitions(node); p++)
    {
        int partition = netloc_get_partition(node, p);
        json_array_append_new(json_partitions, json_integer(partition));
    }
    json_object_set_new(json_node, JSON_DRAW_FILE_NODE_PARTITIONS, json_partitions);

    if (netloc_node_is_host(node)) {
        json_object_set_new(json_node, JSON_DRAW_FILE_NODE_TYPE, json_string("host"));
    }
    else if (netloc_node_is_switch(node)) {
        json_object_set_new(json_node, JSON_DRAW_FILE_NODE_TYPE, json_string("switch"));
    }
    else {
        json_object_set_new(json_node, JSON_DRAW_FILE_NODE_TYPE, json_string("unknown"));
    }

    json_array_append_new(json_nodes, json_node);

    free(desc);
    return 0;
}

static int handle_path(netloc_node_t *node, json_t *json_paths)
{
    //unsigned long uid = node->physical_id_int;
    //char *node_label = node->physical_id;
    char *id = node->physical_id;

    json_t *json_node_paths = json_object();
    json_object_set_new(json_node_paths, JSON_DRAW_FILE_PATH_ID, json_string(id));

    /* Paths */
    json_t *json_path_list = json_array();
    netloc_path_t *path, *path_tmp;
    netloc_node_iter_paths(node, path, path_tmp) {
        json_t *json_node_path = json_object();
        json_object_set_new(json_node_path, JSON_DRAW_FILE_PATH_ID,
                json_string(path->dest_id));

        json_t *json_links = json_array();
        netloc_physical_link_t **plink;
        netloc_path_iter_links(path,plink) {
            json_array_append_new(json_links, json_integer((*plink)->id));
        }
        json_object_set_new(json_node_path, JSON_DRAW_FILE_PATH_LINKS,
                json_links);
        json_array_append_new(json_path_list, json_node_path);
    }
    json_object_set_new(json_node_paths, JSON_DRAW_FILE_PATHS, json_path_list);

    json_array_append_new(json_paths, json_node_paths);

    return 0;
}

static int handle_partitions(netloc_topology_t topology, json_t *json_partitions)
{
    char **ppartition;
    netloc_topology_iter_partitions(topology, ppartition) {
        json_array_append_new(json_partitions, json_string(*ppartition));
    }
    return 0; // TODO
}

static int handle_topos(netloc_topology_t topology, json_t *json_topos)
{
    char **ptopo;
    netloc_topology_iter_hwloctopos(topology, ptopo) {
        json_array_append_new(json_topos, json_string(*ptopo));
    }
    return 0; // TODO
}

static int json_write(netloc_topology_t topology, FILE *output)
{
    json_t *json_root = json_object();

    /* Graph type */
    json_object_set_new(json_root, JSON_DRAW_FILE_GRAPH_TYPE, json_string("tree"));

    /* Nodes */
    json_t *json_nodes = json_array();
    json_t *json_edges = json_array();
    netloc_node_t *node, *node_tmp;
    HASH_ITER(hh, topology->nodes, node, node_tmp) {
        handle_node(node, json_nodes, json_edges, 0);
    }
    json_object_set_new(json_root, JSON_DRAW_FILE_NODES, json_nodes);
    json_object_set_new(json_root, JSON_DRAW_FILE_EDGES, json_edges);

    /* Physical links */
    json_t *json_links = json_array();
    netloc_physical_link_t *link, *link_tmp;
    HASH_ITER(hh, topology->physical_links, link, link_tmp) {
        handle_link(link, json_links);
    }
    json_object_set_new(json_root, JSON_DRAW_FILE_LINKS, json_links);

    /* Paths */
    json_t *json_paths = json_array();
    HASH_ITER(hh, topology->nodes, node, node_tmp) {
        handle_path(node, json_paths);
    }
    json_object_set_new(json_root, JSON_DRAW_FILE_PATHS, json_paths);

    /* Partitions */
    json_t *json_partitions = json_array();
    handle_partitions(topology, json_partitions);
    json_object_set_new(json_root, JSON_DRAW_FILE_PARTITIONS, json_partitions);

    /* Hwloc topologies */
    json_t *json_topos = json_array();
    handle_topos(topology, json_topos);
    json_object_set_new(json_root, JSON_DRAW_FILE_HWLOCTOPOS, json_topos);

    char *json = json_dumps(json_root, 0);
    fprintf(output, "%s", json);

    /* Free json objects */
    json_decref(json_root);

    return 0; // TODO
}

int netloc_to_json_draw(netloc_topology_t topology, int simplify)
{
    static FILE *output;
    char *node_uri = topology->network->node_uri;
    int basename_len = strlen(node_uri)-10;
    char *basename = (char *)malloc((basename_len+1)*sizeof(char));
    char *draw;

    netloc_read_hwloc(topology);

    strncpy(basename, node_uri, basename_len);
    basename[basename_len] = '\0';

    asprintf(&draw, "%s-%s%s.json", basename, "draw", simplify ? "-simple": "");
    output = fopen(draw, "w");

    json_write(topology, output);

    fclose(output);
    return 0; // TODO
}

static char *read_param(int *argc, char ***argv)
{
    if (!*argc)
        return NULL;

    char *ret = **argv;
    (*argv)++;
    (*argc)--;

    return ret;
}

int main(int argc, char **argv)
{
    int ret;

    if (argc < 2 || argc > 3) {
        goto wrong_params;
    }

    int cargc = argc;
    char **cargv = argv;
    char *netloc_dir;
    char *param;
    int simplify = 0;

    read_param(&cargc, &cargv);
    char *path = read_param(&cargc, &cargv);
    asprintf(&netloc_dir, "file://%s/%s", path, "netloc");

    if ((param = read_param(&cargc, &cargv))) {
        simplify = atoi(param);
    }

    netloc_network_t *network = NULL;
    netloc_topology_t topology;

    // Find a specific InfiniBand network
    network = netloc_dt_network_t_construct();
    network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;

    // Search for the specific network
    ret = netloc_find_network(netloc_dir, network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: network not found!\n");
        netloc_dt_network_t_destruct(network);
        return NETLOC_ERROR;
    }
    printf("Found Network: %s\n", netloc_pretty_print_network_t(network));

    // Attach to the network
    ret = netloc_attach(&topology, *network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_attach returned an error (%d)\n", ret);
        return ret;
    }

    // TODO do this automatically as before
    support_load_datafile(topology);

    netloc_to_json_draw(topology, simplify);

    return 0;

wrong_params:
    printf("Usage: %s <netloc_dir> [simplify (1 or 0)]\n", argv[0]);
    return -1;
}

