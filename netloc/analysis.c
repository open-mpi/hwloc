/*
 * Copyright © 2016-2016 Inria.  All rights reserved.
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
#include <sys/types.h>
#include <dirent.h>

#include <private/netloc.h>
#include <hwloc.h>
#include "support.h"

static int compare_edges(void const *a, void const *b)
{
    netloc_edge_t *edge1 = *(netloc_edge_t **) a;
    netloc_edge_t *edge2 = *(netloc_edge_t **) b;
    return (edge1->dest_node > edge2->dest_node) - (edge1->dest_node < edge2->dest_node);
}

netloc_tree_data *netloc_get_tree_data(netloc_topology_analysis *analysis)
{
    return (netloc_tree_data *)analysis->data;
}

// level starts at 1, not 0
static int node_set_level(netloc_node_t *node, int level)
{
    int num_edges = node->num_edges;

    /** If this node has already been handled */
    if (node->userdata && ((netloc_analysis_data *)node->userdata)->level <= level) {
        return 0; // TODO
    }

    netloc_analysis_data *analysis_data;
    if (!node->userdata)
    {
        node->userdata = (void *)malloc(sizeof(netloc_analysis_data));
        analysis_data = (netloc_analysis_data *)node->userdata;
    }
    else
    {
        analysis_data = (netloc_analysis_data *)node->userdata;
    }

    analysis_data->level = level;

    for (int e = 0; e < num_edges; e++) {
        netloc_edge_t *edge = node->edges[e];
        netloc_node_t *dest_node = edge->dest_node;
        node_set_level(dest_node, level+1);
    }
    return 0; // TODO
}

static int node_max_level(netloc_node_t *node, int *max_level)
{
    int level = ((netloc_analysis_data *)node->userdata)->level;
    if (level > *max_level)
    {
        *max_level = level;
    }

    return 0; // TODO
}

// Level starts from 0 now
static int node_inverse_level(netloc_node_t *node, int max_level)
{
    int *plevel = &((netloc_analysis_data *)node->userdata)->level;
    *plevel = max_level-*plevel;
    return 0; // TODO
}

static int tree_set_levels(netloc_topology_analysis *analysis)
{
    int max_level = -1;
    netloc_node_t *node;

    netloc_topology_t topology = analysis->topology;

    netloc_dt_lookup_table_t hosts;
    netloc_get_all_host_nodes(topology, &hosts);

    netloc_dt_lookup_table_iterator_t hti =
        netloc_dt_lookup_table_iterator_t_construct(hosts);
    while ((node =
            (netloc_node_t *)netloc_lookup_table_iterator_next_entry(hti))) {
        node_set_level(node, 1);
    }

    // we compute the depth of the tree
    netloc_dt_lookup_table_t nodes;
    netloc_get_all_nodes(topology, &nodes);
    hti = netloc_dt_lookup_table_iterator_t_construct(nodes);
    while ((node =
            (netloc_node_t *)netloc_lookup_table_iterator_next_entry(hti))) {
        node_max_level(node, &max_level);
    }

    hti = netloc_dt_lookup_table_iterator_t_construct(nodes);
    int *num_nodes_by_level = (int *)calloc(max_level, sizeof(int));
    netloc_node_t ***nodes_by_level = (netloc_node_t ***)
        malloc(max_level*sizeof(netloc_node_t **));
    int num_nodes = topology->num_nodes;
    for (int l = 0; l < max_level; l++) {
        nodes_by_level[l] = (netloc_node_t **)malloc(num_nodes*sizeof(netloc_node_t *));
    }
    while ((node =
            (netloc_node_t *)netloc_lookup_table_iterator_next_entry(hti))) {
        // we need to inverse the level because leaves are level 0 right now
        node_inverse_level(node, max_level);

        // Lists of nodes by level
        int level = ((netloc_analysis_data *)node->userdata)->level;
        nodes_by_level[level][num_nodes_by_level[level]] = node;

        num_nodes_by_level[level]++;
    }
    for (int l = 0; l < max_level; l++) {
        nodes_by_level[l] = realloc(nodes_by_level[l],
                num_nodes_by_level[l]*sizeof(netloc_node_t));
    }

    netloc_tree_data *data = netloc_get_tree_data(analysis);
    data->num_nodes_by_level = num_nodes_by_level;
    data->nodes_by_level = nodes_by_level;

    return max_level;
}

static netloc_tree_data *tree_data()
{
    return (netloc_tree_data *)malloc(sizeof(netloc_tree_data));
}

static int netloc_node_merge_edges(netloc_topology_t topology, netloc_node_t *node)
{
    static int virtual_count = 0;
    netloc_edge_t **edges = node->edges;

    /* we sort the edges by destination */
    qsort(edges, node->num_edges, sizeof(netloc_edge_t *), compare_edges);

    netloc_node_t *old_neighbour = NULL;
    netloc_node_t *neighbour;
    int repeat = 0;
    for (int e = 0; e < node->num_edges; e++)
    {
        neighbour = node->edges[e]->dest_node;
        if (neighbour == old_neighbour)
        {
            repeat = 1;
            break;
        }
        old_neighbour = neighbour;
    }

    /* we merge the edges by adding the gflops */
    if (repeat)
    {
        int idx = 0;
        int firstIdx = 0;
        netloc_node_t *old_neighbour = node->edges[0]->dest_node;
        netloc_node_t *neighbour;

        for (int e = 1; e < node->num_edges+1; e++)
        {
            if (e == node->num_edges)
                neighbour = NULL;
            else
                neighbour = node->edges[e]->dest_node;

            // if the edge is different from the previous one
            if (neighbour != old_neighbour)
            {
                int num_edges = e-firstIdx;
                if (num_edges > 1) {
                    /* we use explist because we can have more edges if we deal
                    with virtual edges */
                    netloc_explist_t *exp_real_edges = netloc_explist_init(num_edges);

                    /* Creation of the virtual edge based on the first edge */
                    /* NOTE: we cannot create a new edge by calling
                     * netloc_dt_edge_t_construct since the resulting
                     * edge_id could already exist.
                     * Thus the virtual edge will have the same id (FIXME) */
                    netloc_edge_t *virtual_edge = (netloc_edge_t *)
                        malloc(sizeof(netloc_edge_t));
                    memcpy(virtual_edge, node->edges[firstIdx], sizeof(netloc_edge_t));
                    virtual_edge->src_port_id = NULL;
                    virtual_edge->dest_port_id = NULL;
                    virtual_edge->speed = NULL;
                    virtual_edge->width = NULL;
                    asprintf(&virtual_edge->description, "virtual%d", virtual_count++);
                    virtual_edge->partitions = (int *)
                        malloc(node->edges[firstIdx]->num_partitions*sizeof(int));
                    memcpy(virtual_edge->partitions, node->edges[firstIdx]->partitions,
                            node->edges[firstIdx]->num_partitions*sizeof(int));
                       /* work only if all the same */


                    for (int f = firstIdx; f < e; f++)
                    {
                        netloc_edge_t *edge = node->edges[f];
                        edge->virtual_edge = virtual_edge;
                        /* we remove the merged edges from the edge list */
                        char *key;
                        asprintf(&key, "%d", edge->edge_uid);
                        // XXX garder l'arete dans la table !!!! mais la marquer comme supprimee
                        // TODO TODO TODO TODO
                        netloc_lookup_table_remove(topology->edges, key);
                        /* sum the gbits from the merged edges */
                        virtual_edge->gbits += edge->gbits;
                        free(key);

                        /* Creation of the list of the real edges */
                        if (edge->num_real_edges) {
                            for (int e = 0; e < edge->num_real_edges; e++) {
                                netloc_explist_add(exp_real_edges, edge->real_edges[e]);
                            }
                        }
                        else {
                            netloc_explist_add(exp_real_edges, edge);
                        }
                    }
                    virtual_edge->num_real_edges = netloc_explist_get_size(exp_real_edges);
                    virtual_edge->real_edges = (netloc_edge_t **)
                        netloc_explist_get_array_and_destroy(exp_real_edges);

                    char *virtual_key;
                    asprintf(&virtual_key, "%d", virtual_edge->edge_uid);

                    netloc_lookup_table_append(topology->edges, virtual_key,
                            virtual_edge);

                    node->edges[idx] = virtual_edge;
                }
                else {
                    node->edges[idx] = node->edges[e-1];
                }
                firstIdx = e;
                old_neighbour = neighbour;

                idx++;
            }
        }

        node->num_edges = idx;
        node->num_edge_ids = idx;

        node->edges = realloc(node->edges, idx*sizeof(netloc_edge_t *));
        node->edge_ids = realloc(node->edge_ids, idx*sizeof(int));
        for (int e = 0; e < node->num_edges; e++)
        {
            node->edge_ids[e] = node->edges[e]->edge_uid;
        }
    }
    return 0;
}

int netloc_topology_merge_edges(netloc_topology_t topology)
{
    netloc_node_t *node;
    netloc_dt_lookup_table_t nodes;
    netloc_get_all_nodes(topology, &nodes);
    netloc_dt_lookup_table_iterator_t hti =
        netloc_dt_lookup_table_iterator_t_construct(nodes);
    while ((node =
            (netloc_node_t *)netloc_lookup_table_iterator_next_entry(hti))) {

        netloc_node_merge_edges(topology, node);

    }
    return 0;
}

static int node_are_equivalent(netloc_node_t *node1, netloc_node_t *node2)
{
    int num = node1->num_edges;
    if (node2->num_edges != num)
        return 0;

    for (int n = 0; n < num; n++) {
        if (node1->edges[n]->dest_node != node2->edges[n]->dest_node)
            return 0;
    }

    return 1;
}

int netloc_topology_merge_nodes(netloc_topology_t topology)
{
    netloc_explist_t *nodes_by_degree = netloc_explist_init(32);

    for (int n = 0; n < topology->num_nodes; n++) {
        netloc_node_t *node = topology->nodes[n];

        /* check if the current node as one leaf (node with one edge) as
         * neighbour if it does, it means it does not have a clone node */
        int leaf_as_neighbour = 0;
        for (int e = 0; e < node->num_edges; e++) {
            netloc_edge_t *edge = node->edges[e];
            if (edge->dest_node->num_edges == 1) {
                leaf_as_neighbour++;
                break;
            }
        }
        /* The node is still a candidate for having a clone */
        if (node->num_edges != 1 && !leaf_as_neighbour) {
            int idx = node->num_edges-1;

            /* we save the node in the array */
            netloc_explist_t *nodes = netloc_explist_get(nodes_by_degree, idx);
            if (!nodes) {
                nodes = netloc_explist_init(512);
                netloc_explist_set(nodes_by_degree, idx, nodes);
            }
            netloc_explist_add(nodes, (void *) (intptr_t)n);
        }
    }

    /* We read the list of candidates to find nodes to merge */
    netloc_explist_t *nodes_to_merge = netloc_explist_init(10);

    for (int idx = 0; idx < netloc_explist_get_size(nodes_by_degree); idx++) {
        netloc_explist_t *nodes = netloc_explist_get(nodes_by_degree, idx);

        if (!nodes)
            continue;

        for (int n1 = 0; n1 < netloc_explist_get_size(nodes); n1++) {
            int node_idx1 = (intptr_t)netloc_explist_get(nodes, n1);
            if (node_idx1 == -1) /* already in a group of same nodes */
                continue;
            netloc_node_t *node1 = topology->nodes[node_idx1];
            netloc_explist_t *same_nodes = NULL;

            for (int n2 = n1+1; n2 < netloc_explist_get_size(nodes); n2++) {
                int node_idx2 = (intptr_t)netloc_explist_get(nodes, n2);
                if (node_idx2 == -1) /* already in a group of same nodes */
                    continue;
                netloc_node_t *node2 = topology->nodes[node_idx2];

                if (node_are_equivalent(node1, node2)) {
                    netloc_explist_set(nodes, n2, (void *)(intptr_t)-1);
                    if (!same_nodes) {
                        same_nodes = netloc_explist_init(2);
                        netloc_explist_add(same_nodes, (void *)(intptr_t)node_idx1);
                    }
                    netloc_explist_add(same_nodes, (void *)(intptr_t)node_idx2);
                }
            }
            if (same_nodes)
                netloc_explist_add(nodes_to_merge, same_nodes);
        }
    }

    /* we merge the selected nodes */
    int virtual_count = 0;
    for (int n = 0; n < netloc_explist_get_size(nodes_to_merge); n++) {
        netloc_explist_t *same_nodes = netloc_explist_get(nodes_to_merge, n);

        /* We save the list of pointers to the clone nodes */
        int num_clones = netloc_explist_get_size(same_nodes);
        int num_edges = 0;
        netloc_explist_t *exp_real_nodes = netloc_explist_init(num_clones);
        netloc_node_t *virtual_node = netloc_dt_node_t_construct();

        for (int m = 0; m < num_clones; m++) {
            int idx = (intptr_t)netloc_explist_get(same_nodes, m);

            netloc_node_t *node = topology->nodes[idx];
            node->virtual_node = virtual_node;
            if (node->num_real_nodes) {
                for (int n = 0; n < node->num_real_nodes; n++) {
                    netloc_explist_add(exp_real_nodes, node->real_nodes[n]);
                }
            }
            else {
                netloc_explist_add(exp_real_nodes, node);
            }

            num_edges += node->num_edges;
        }

        /* we create the virtual node */
        // TODO TODO TODO
        // virtual_node->network_type = NETLOC_NETWORK_TYPE_INVALID;
        // virtual_node->node_type    = NETLOC_NODE_TYPE_INVALID;
        // virtual_node->subnet_id    = NULL;
        // virtual_node->num_phy_paths = 0;
        // virtual_node->physical_paths = calloc(1, sizeof(*node->physical_paths));
        // virtual_node->num_log_paths = 0;
        // virtual_node->logical_paths = calloc(1, sizeof(*node->logical_paths));
        // virtual_node->physical_id_int = 0;
        // TODO __uid__

        virtual_node->edges = (netloc_edge_t **)
            malloc(num_edges*sizeof(netloc_edge_t *));
        virtual_node->edge_ids = (int *) malloc(num_edges*sizeof(int));
        virtual_node->num_edges = num_edges;
        virtual_node->num_real_nodes = num_clones;
        asprintf(&virtual_node->physical_id, "virtual%d", virtual_count);
        asprintf(&virtual_node->logical_id, "virtual%d", virtual_count);
        asprintf(&virtual_node->description, "virtual%d", virtual_count);
        virtual_node->num_real_nodes = netloc_explist_get_size(exp_real_nodes);
        virtual_node->real_nodes = (netloc_node_t **)
            netloc_explist_get_array_and_destroy(exp_real_nodes);

        virtual_node->partitions = (int *)
            malloc(virtual_node->real_nodes[0]->num_partitions*sizeof(int));
        memcpy(virtual_node->partitions, virtual_node->real_nodes[0]->partitions,
                virtual_node->real_nodes[0]->num_partitions*sizeof(int));
        virtual_node->num_partitions =
            virtual_node->real_nodes[0]->num_partitions;
            /* works only if the same between clones TODO merge all */

        /* we copy all the edges */
        int edge_idx = 0;

        printf("Merged nodes: ");

        /* Copy all the edges from the merged nodes into the virtual node */
        for (int m = 0; m < num_clones; m++) {
            netloc_node_t *clone = virtual_node->real_nodes[m];
            printf("%s ", clone->physical_id);

            memcpy(&virtual_node->edge_ids[edge_idx], clone->edge_ids,
                    virtual_node->real_nodes[m]->num_edge_ids*sizeof(int)); 

            for (int e = 0; e < clone->num_edges; e++) {
                netloc_edge_t *new_edge = (netloc_edge_t *)
                    malloc(sizeof(netloc_edge_t));
                memcpy(new_edge, clone->edges[e], sizeof(netloc_edge_t)); 
                virtual_node->edges[edge_idx++] = clone->edges[e]; 
                clone->edges[e] = new_edge;
            }
        }
        printf("\n");

        /* fix the source of the edges to be the virtual one */
        for (int e = 0; e < num_edges; e++) {
            virtual_node->edges[e]->src_node = virtual_node;
            asprintf(&virtual_node->edges[e]->src_node_id, "%s",
                    virtual_node->physical_id);
        }

        /* fix the destination of the neighbours of the virtual node */
        for (int m = 0; m < num_clones; m++) {
            netloc_node_t *node = virtual_node->real_nodes[m];
            int num_edges = node->num_edges;
            for (int e = 0; e < num_edges; e++) {
                netloc_edge_t *edge = node->edges[e];
                netloc_node_t *neighbour = edge->dest_node;
                for (int f = 0; f < neighbour->num_edges; f++) {
                    if (neighbour->edges[f]->dest_node == node) {
                        neighbour->edges[f]->dest_node = virtual_node;
                        asprintf(&neighbour->edges[f]->dest_node_id, "%s",
                                virtual_node->physical_id);
                        break;
                    }
                }
            }
        }
        
        /* we need to remove clone edges */
        netloc_node_merge_edges(topology, virtual_node);

        /* We remove the merged nodes */
        topology->nodes[(intptr_t)netloc_explist_get(same_nodes, 0)] = virtual_node;
        int shift = 0;
        for (int m = 1; m < num_clones; m++) {
            int start = (intptr_t)netloc_explist_get(same_nodes, m)+1;
            int end;
            shift++;
            if (m != num_clones-1)
                end = (intptr_t)netloc_explist_get(same_nodes, m+1);
            else
                end = topology->num_nodes;
            for (int o = start; o < end; o++)
                topology->nodes[o-shift] = topology->nodes[o];
        }
        topology->num_nodes -= shift;

        virtual_count++;
    }

    return 0;
}

/* We suppose the description of nodes is like that: ([^ ]*).*
 * while \1 is the hostname
 */
static int node_set_hostname(netloc_node_t *node)
{
    char *name = node->description;
    int max_size = strlen(name);
    char *hostname = (char *)malloc(max_size*sizeof(char));

    /* Looking for the name of the hostname */
    int i = 0;
    if (name[0] == '\'')
        name++;
    while (i < max_size &&
            ((name[i] >= 'a' && name[i] <= 'z') ||
             (name[i] >= '0' && name[i] <= '9') ||
             (name[i] == '-'))) {
        hostname[i] = name[i];
        i++;
    }
    hostname[i++] = '\0';
    hostname = realloc(hostname, i*sizeof(char));
    node->hostname = hostname;

    return 0;
}

/* We suppose the description of nodes is like that: ([a-z]*).*
 * while \1 is the name of the partition
 */
static char *node_find_partition_name(netloc_node_t *node)
{
    char *name;
    int max_size;
    char *partition;

    if (!node->hostname)
        node_set_hostname(node);

    max_size = strlen(node->hostname);
    partition = (char *)malloc(max_size*sizeof(char));
    name = node->hostname;

    /* Looking for the name of the partition */
    int i = 0;
    while (i < max_size && (name[i] >= 'a' && name[i] <= 'z')) {
        partition[i] = name[i];
        i++;
    }
    partition[i++] = '\0';
    partition = realloc(partition, i*sizeof(char));
    return partition;
}

/* TODO use netloc_dt_lookup_table_iterator_t, but need to improve it first */
int netloc_topology_find_partitions(netloc_topology_t topology)
{
    int ret = 0;
    int num_nodes;
    char **partition_names;

    if( !topology->nodes_loaded ) {
        ret = support_load_json(topology);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to load the topology\n");
            return ret;
        }
    }

    num_nodes = topology->num_nodes;
    partition_names = (char **)malloc(num_nodes*sizeof(char *));

    /* Save all the partition names */
    for (int n = 0; n < num_nodes; n++) {
        netloc_node_t *node = topology->nodes[n];
        if (node->node_type == NETLOC_NODE_TYPE_HOST)
            partition_names[n] = node_find_partition_name(node);
        else
            partition_names[n] = NULL;
    }

    /* Associate the field partition in the nodes to the correct partition
     * index */
    int num_partitions = 0;
    for (int n1 = 0; n1 < num_nodes; n1++) {
        if (!partition_names[n1])
            continue;
        partition_names[num_partitions] = partition_names[n1];
        topology->nodes[n1]->partitions = (int *)malloc(sizeof(int));
        topology->nodes[n1]->partitions[0] = num_partitions;
        topology->nodes[n1]->num_partitions = 1;

        for (int n2 = n1+1; n2 < num_nodes; n2++) {
            if (!partition_names[n2])
                continue;

            if (!strcmp(partition_names[n1], partition_names[n2])) {
                free(partition_names[n2]);
                partition_names[n2] = NULL;
                topology->nodes[n2]->partitions = (int *)malloc(sizeof(int));
                topology->nodes[n2]->partitions[0] = num_partitions;
                topology->nodes[n2]->num_partitions = 1;
            }
        }
        num_partitions++;
    }

    printf("%d partitions found\n", num_partitions);
    for (int p = 0; p < num_partitions; p++) {
        printf("\t'%s'\n", partition_names[p]);
    }

    topology->partitions = (char **)
        realloc(partition_names, num_partitions*sizeof(char *));
    topology->num_partitions =  num_partitions;


    return ret;
}

static int remove_edge(netloc_topology_t topology, netloc_edge_t *edge)
{
    char *key;
    asprintf(&key, "%d", edge->edge_uid);
    netloc_lookup_table_remove(topology->edges, key);
    netloc_dt_edge_t_destruct(edge);
    return 0;
}

static netloc_node_t *find_node(netloc_topology_t topology, const char *id)
{
    for (int n = 0; n < topology->num_nodes; n++) {
        if (!strcmp(topology->nodes[n]->physical_id, id))
                return topology->nodes[n];
    }
    return NULL;

}

/* Returns: -1 if partition_name is NULL, -2 if partition not found, else
 * partition index */
int netloc_topology_find_partition_idx(netloc_topology_t topology, char *partition_name)
{
    if (!partition_name)
        return -1;

    if (!topology->num_partitions)
        netloc_topology_find_partitions(topology);

    /* Find the selected partition in the topology */
    int p = 0;
    int found = 0;
    while (p < topology->num_partitions) {
        if (!strcmp(topology->partitions[p], partition_name)) {
            found = 1;
            break;
        }
        p++;
    }

    if (!found)
        return -2;

    return p;
}

/* Partition is the pointer present in topology->partitions */
static int netloc_topology_set_partition(netloc_topology_t topology, int partition)
{
    netloc_dt_lookup_table_t hosts;
    netloc_get_all_host_nodes(topology, &hosts);

    netloc_dt_lookup_table_iterator_t host_iter =
        netloc_dt_lookup_table_iterator_t_construct(hosts);
    netloc_node_t *host;
    while ((host =
                (netloc_node_t *)netloc_lookup_table_iterator_next_entry(host_iter))) {
        if (host->partitions[0] != partition)
            continue;

        netloc_dt_lookup_table_iterator_t path_iter = 
            netloc_dt_lookup_table_iterator_t_construct(host->logical_paths);
        char *key;
        /* For each path from host */
        // TODO add a way to retrieve the key and the value (edges) at the same time
        while ((key = (char *)netloc_lookup_table_iterator_next_key(path_iter))) {
            netloc_node_t *dest_node = find_node(topology, key);
            /* Check that the destination node is in the partition */
            if (dest_node->partitions[0] != partition)
                continue;
            netloc_edge_t **edges = (netloc_edge_t **)
                netloc_lookup_table_access(host->logical_paths, key);

            /* Add all the edges in the path */
            int idx = 0;
            netloc_edge_t *edge;
            while ((edge = edges[idx++])) {
                /* Set the partition for the edge */
                if (!edge->partitions)
                    edge->partitions = (int *)malloc(topology->num_partitions*sizeof(int));
                int p = 0;
                for ( ; p < edge->num_partitions; p++) {
                    if (edge->partitions[p] == partition)
                        break;
                }
                if (p == edge->num_partitions) {
                    edge->num_partitions++;
                    edge->partitions[p] = partition;
                }

                /* Set the partition for the node */
                netloc_node_t *node = edge->dest_node;
                if (node == dest_node)
                    continue;

                if (!node->partitions)
                    node->partitions = (int*)malloc(topology->num_partitions*sizeof(int));
                for (; p < node->num_partitions; p++) {
                    if (node->partitions[p] == partition)
                        break;
                }
                if (p == node->num_partitions)
                    node->num_partitions++;
                    node->partitions[p] = partition;
            }
        }
    }
    return 0;
}

int netloc_topology_set_all_partitions(netloc_topology_t topology)
{
    /* Find the partitions */
    if (!topology->num_partitions)
        netloc_topology_find_partitions(topology);

    for (int p = 0; p < topology->num_partitions; p++)
    {
        netloc_topology_set_partition(topology, p);
    }
    return 0; // TODO
}

int netloc_topology_node_in_partition(netloc_node_t *node, int partition)
{
    for (int p = 0; p < node->num_partitions; p++) {
        if (node->partitions[p] == partition)
            return 1;
    }
    return 0;
}

int netloc_topology_edge_in_partition(netloc_edge_t *edge, int partition)
{
    for (int p = 0; p < edge->num_partitions; p++) {
        if (edge->partitions[p] == partition)
            return 1;
    }
    return 0;
}


int netloc_topology_keep_partition(netloc_topology_t topology, char *partition_name)
{
    int ret = 0;

    /* Find the selected partition in the topology */
    int partition = netloc_topology_find_partition_idx(topology, partition_name);
    if (partition < 0)
        return -1;

    /* Find all the partitions nodes and edges belong to */
    netloc_topology_set_partition(topology, partition);

    /* Remove nodes that are not in the given partition (or their link to edges
     * not in partition neither) */
    int num_removed_nodes = 0;
    for (int n = 0; n < topology->num_nodes; n++) {
        netloc_node_t *node = topology->nodes[n];
        if (netloc_topology_node_in_partition(node, partition)) {
            /* Need to removed unused edges from the node */
            int num_removed_edges = 0;
            for (int e = 0; e < node->num_edges; e++) {
                netloc_edge_t *edge = node->edges[e];
                if (netloc_topology_edge_in_partition(edge, partition)) {
                    node->edges[e-num_removed_edges] = node->edges[e];
                }
                else 
                    num_removed_edges++;
            }
            node->num_edges-= num_removed_edges;
            node->edges = realloc(node->edges, node->num_edges*sizeof(netloc_edge_t *));

            if (!node->num_edges) {
                int *truc = NULL; // XXX DEBUG
                *truc = 4;
            }
            topology->nodes[n-num_removed_nodes] = topology->nodes[n];
        }
        else {
            netloc_dt_node_t_destruct(node);
            num_removed_nodes++;
        }
    }
    topology->num_nodes -= num_removed_nodes;
    topology->nodes = realloc(topology->nodes, topology->num_nodes*sizeof(netloc_node_t *));

    /* Remove edges that are not in the given partition */
    netloc_dt_lookup_table_t edges = topology->edges;
    netloc_dt_lookup_table_iterator_t edges_iter;
    edges_iter= netloc_dt_lookup_table_iterator_t_construct(edges);
    netloc_edge_t *edge;
    netloc_explist_t *removed_edges = netloc_explist_init(100);
    while ((edge =
                (netloc_edge_t *)netloc_lookup_table_iterator_next_entry(edges_iter))) {
        /* TODO improve with function thar returns key and value */
        // XXX TODO is it okay to remove one element while browsing XXX XXX XXX FIXME
        if (!netloc_topology_edge_in_partition(edge, partition)) {
            /* mark them to be removed since we are browsing the table
             * TODO change with the table */
            netloc_explist_add(removed_edges, edge);
        }
    }
    /* Remove the marked edges */
    for (int e = 0; e < netloc_explist_get_size(removed_edges); e++) {
        netloc_edge_t *edge = (netloc_edge_t *)netloc_explist_get(removed_edges, e);
        remove_edge(topology, edge);
    }


    return ret;
}

int netloc_topology_simplify(netloc_topology_t topology)
{
    int ret = 0;

    netloc_topology_merge_edges(topology);
    netloc_topology_merge_nodes(topology);
    netloc_topology_merge_edges(topology);

    return ret;
}

static int netloc_read_hwloc(netloc_topology_t topology)
{
    int ret = 0;
    int err;
    netloc_node_t *node;

    char *hwloc_path;
    asprintf(&hwloc_path, "%s/../hwloc", topology->network->data_uri+7);

    netloc_explist_t *topos = netloc_explist_init(1);
    int num_topos = 0;

    DIR* dir = opendir(hwloc_path);
    /* Directory does not exist */
    if (!dir) {
        printf("Directory (%s) to hwloc does not exist\n", hwloc_path);
        return -1;
    }
    else {
        closedir(dir);
    }

    netloc_dt_lookup_table_t hosts;
    netloc_get_all_host_nodes(topology, &hosts);

    netloc_dt_lookup_table_iterator_t hti =
        netloc_dt_lookup_table_iterator_t_construct(hosts);
    int num_diffs = 0;
    while ((node =
            (netloc_node_t *)netloc_lookup_table_iterator_next_entry(hti))) {

        char *hwloc_file;
        char *refname;

        /* We try to find a diff file */
        asprintf(&hwloc_file, "%s/%s.diff.xml", hwloc_path, node->hostname);
        hwloc_topology_diff_t diff;
        if ((err = hwloc_topology_diff_load_xml(hwloc_file, &diff, &refname)) >= 0) {
            refname[strlen(refname)-4] = '\0';
            hwloc_topology_diff_destroy(diff);
            num_diffs++;
        }
        else {
            free(hwloc_file);
            /* We try to find a regular file */
            asprintf(&hwloc_file, "%s/%s.xml", hwloc_path, node->hostname);
            FILE *fxml;
            if (!(fxml = fopen(hwloc_file, "r"))) {
                printf("Hwloc file absent: %s\n", hwloc_file);
            }
            else
                fclose(fxml);
            asprintf(&refname, "%s", node->hostname);
        }
        free(hwloc_file);

        /* We add the hwloc topology */
        int t = 0;
        while (t < netloc_explist_get_size(topos) &&
                strcmp(netloc_explist_get(topos, t), refname)) {
            t++;
        }
        if (t == netloc_explist_get_size(topos)) {
            netloc_explist_add(topos, refname);
        }
        else {
            free(refname);
        }
        node->topoIdx = t;
    }

    if (!num_diffs) {
        printf("Warning: no hwloc diff file found!\n");
    }

    topology->num_topos = netloc_explist_get_size(topos);
    topology->topos = (char **)
        netloc_explist_get_array_and_destroy(topos);

    printf("%d hwloc topologies found:\n", topology->num_topos);
    // XXX XXX XXX XXX XXX XXX XXX faire pareil pour recherche partitions
    for (int p = 0; p < topology->num_topos; p++) {
        printf("\t'%s'\n", topology->topos[p]);
    }

    return ret;
}

int netloc_partition_analyse(netloc_topology_analysis *analysis,
        netloc_topology_t topology, int simplify, char *partition,
        int levels, int hwloc)
{
    int ret = 0;
    analysis->topology = topology;

    if (partition) {
        netloc_topology_keep_partition(topology, partition);
    }
    else {
        netloc_topology_set_all_partitions(topology);
    }
    if (hwloc)
        netloc_read_hwloc(topology);
    if (simplify)
        netloc_topology_simplify(topology);

    // if topology is a tree TODO check type
    analysis->type = NETLOC_TOPOLOGY_TYPE_TREE;
    netloc_tree_data *data = tree_data();
    analysis->data = (void *)data;

    data->num_levels = tree_set_levels(analysis);

    return ret;
}
