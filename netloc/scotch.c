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
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <scotch.h>

#include <netloc.h>
#include <netlocscotch.h>
#include <private/netloc.h>
#include <hwloc.h>
#include "support.h"

static void set_gbits(int *values, netloc_edge_t *edge, int num_levels)
{
    int gbits = (int)edge->total_gbits;
    int idx = num_levels-1-((netloc_analysis_data *)edge->node->userdata)->level;
    if (values[idx] != 0) {
        if (gbits < values[idx]) {
            printf("Warning: edge from %s to %s is only %dGbits\n",
                    edge->node->description, edge->dest->description,
                    gbits);
        }
        else if (gbits > values[idx]) {
            printf("Warning: at least on edge from %s has a limited speed\n",
                    edge->node->description);
            values[idx] = gbits;
        }
    }
    else
        values[idx] = gbits;
}


int netloc_node_is_in_partition(netloc_node_t *node, int partition)
{
    for (int i = 0; i < netloc_get_num_partitions(node); i++) {
        if (netloc_get_partition(node, i) == partition)
            return 1;
    }
    return NETLOC_SUCCESS;
}

int netloc_edge_is_in_partition(netloc_edge_t *edge, int partition)
{
    for (int i = 0; i < netloc_get_num_partitions(edge); i++) {
        if (netloc_get_partition(edge, i) == partition)
            return 1;
    }
    return NETLOC_SUCCESS;
}

netloc_tree_data *netloc_get_tree_data(netloc_topology_analysis *analysis)
{
    return (netloc_tree_data *)analysis->data;
}

static netloc_tree_data *tree_data()
{
    return (netloc_tree_data *)malloc(sizeof(netloc_tree_data));
}

static int partition_topology_to_tleaf(netloc_topology_t topology, int partition,
        netloc_arch_tree_t *tree)
{
    int ret = 0;
    UT_array *nodes;
    utarray_new(nodes, &ut_ptr_icd);

    /* we build nodes from host list in the given partition
     * and we init all the analysis data */
    netloc_node_t *node, *node_tmp;
    netloc_topology_iter_nodes(topology, node, node_tmp) {
        if (!netloc_node_is_in_partition(node, partition))
            continue;
        void *userdata = node->userdata;
        node->userdata = (void *)malloc(sizeof(netloc_analysis_data));
        netloc_analysis_data *analysis_data = (netloc_analysis_data *)node->userdata;
        analysis_data->level = -1;
        analysis_data->userdata = userdata; 

        netloc_edge_t *edge, *edge_tmp;
        netloc_node_iter_edges(node, edge, edge_tmp) {
            void *userdata = edge->userdata;
            edge->userdata = (void *)malloc(sizeof(netloc_analysis_data));
            netloc_analysis_data *analysis_data = (netloc_analysis_data *)edge->userdata;
            analysis_data->level = -1;
            analysis_data->userdata = userdata; 
        }

        if (netloc_node_is_host(node)) {
            utarray_push_back(nodes, &node);
        }
    }

    /* We set the levels in the analysis data */
    /* Upward edges will have the level of the source node and downward edges
     * will have -1 as level */
    int num_levels = 0;
    UT_array *edges_by_level;
    utarray_new( edges_by_level, &ut_ptr_icd);
    netloc_node_t *current_node = /* pointer to one host node */
        *(void **)utarray_eltptr(nodes, 0);
    while (utarray_len(nodes)) {
        UT_array *new_nodes;
        utarray_new(new_nodes, &ut_ptr_icd);

        UT_array *edges;
        utarray_new(edges, &ut_ptr_icd);
        utarray_push_back(edges_by_level, &edges);

        for (int n = 0; n < utarray_len(nodes); n++) {
            netloc_node_t *node = *(void **)utarray_eltptr(nodes, n);
            netloc_analysis_data *node_data = (netloc_analysis_data *)node->userdata;
            /* There is a problem, this is not a tree */
            if (node_data->level != -1 && node_data->level != num_levels) {
                utarray_free(new_nodes);
                ret = -1;
                goto end;
            }
            else {
                node_data->level = num_levels;
                netloc_edge_t *edge, *edge_tmp;
                netloc_node_iter_edges(node, edge, edge_tmp) {
                    if (!netloc_edge_is_in_partition(edge, partition))
                        continue;
                    netloc_analysis_data *edge_data = (netloc_analysis_data *)edge->userdata;

                    netloc_node_t *dest = edge->dest;
                    netloc_analysis_data *dest_data = (netloc_analysis_data *)dest->userdata;
                    /* If we are going back */
                    if (dest_data->level != -1 && dest_data->level < num_levels) {
                        continue;
                    }
                    else {
                        if (dest_data->level != num_levels) {
                            edge_data->level = num_levels;
                            utarray_push_back(new_nodes, &dest);
                            utarray_push_back(edges, &edge);
                        }
                    }
                }
            }
        }
        num_levels++;
        utarray_free(nodes);
        nodes = new_nodes;
    }
    utarray_free(nodes);

    /* We go though the tree to order the leaves  and find the tree
     * structure */
    UT_array *ordered_hosts;
    UT_array **down_degrees_by_level;
    int *max_down_degrees_by_level;
    int *gbits_by_level;

    utarray_new(ordered_hosts, &ut_ptr_icd);

    down_degrees_by_level = (UT_array **)malloc(num_levels*sizeof(UT_array *));
    for (int l = 0; l < num_levels; l++) {
        utarray_new(down_degrees_by_level[l], &ut_int_icd);
    }
    max_down_degrees_by_level = (int *)calloc(num_levels-1, sizeof(int));
    gbits_by_level = (int *)calloc(num_levels-1, sizeof(int));

    UT_array *down_edges;
    utarray_new(down_edges, &ut_ptr_icd);
    netloc_edge_t *up_edge = current_node->edges;
    utarray_push_back(ordered_hosts, &current_node);
    while (1) {
        if (utarray_len(down_edges)) {
            netloc_edge_t *down_edge = *(void **)utarray_back(down_edges);
            utarray_pop_back(down_edges);
            netloc_node_t *dest_node = down_edge->dest;
            if (netloc_node_is_host(dest_node)) {
                set_gbits(gbits_by_level, down_edge, num_levels);
                utarray_push_back(ordered_hosts, &dest_node);
            }
            else {
                netloc_edge_t *edge, *edge_tmp;
                int num_edges = 0;
                netloc_node_iter_edges(dest_node, edge, edge_tmp) {
                    if (!netloc_edge_is_in_partition(edge, partition))
                        continue;
                    netloc_analysis_data *edge_data = (netloc_analysis_data *)edge->userdata;
                    int edge_level = edge_data->level;
                    if (edge_level == -1) {
                        set_gbits(gbits_by_level, edge, num_levels);
                        utarray_push_back(down_edges, &edge);
                        num_edges++;
                    }
                }
                int level = ((netloc_analysis_data *)dest_node->userdata)->level;
                utarray_push_back(down_degrees_by_level[level], &num_edges);
                max_down_degrees_by_level[num_levels-1-level] =
                    max_down_degrees_by_level[num_levels-1-level] > num_edges ?
                    max_down_degrees_by_level[num_levels-1-level]: num_edges;
            }
        }
        else {
            netloc_edge_t *new_up_edge = NULL;
            if (!up_edge)
                break;

            netloc_node_t *up_node = up_edge->dest;
            netloc_edge_t *edge, *edge_tmp;
            int num_edges = 0;
            netloc_node_iter_edges(up_node, edge, edge_tmp) {
                if (!netloc_edge_is_in_partition(edge, partition))
                    continue;
                netloc_analysis_data *edge_data = (netloc_analysis_data *)edge->userdata;
                int edge_level = edge_data->level;
                
                netloc_node_t *dest_node = edge->dest;

                /* If the is the node where we are from */
                if (dest_node == up_edge->node) {
                    num_edges++;
                    set_gbits(gbits_by_level, edge, num_levels);
                    continue;
                }

                /* Downward edge */
                if (edge_level == -1) {
                    set_gbits(gbits_by_level, edge, num_levels);
                    utarray_push_back(down_edges, &edge);
                    num_edges++;
                }
                /* Upward edge */
                else {
                    new_up_edge = edge;
                }

            }
            int level = ((netloc_analysis_data *)up_node->userdata)->level;
            utarray_push_back(down_degrees_by_level[level], &num_edges);
            max_down_degrees_by_level[num_levels-1-level] =
                max_down_degrees_by_level[num_levels-1-level] > num_edges ?
                max_down_degrees_by_level[num_levels-1-level]: num_edges;
            up_edge = new_up_edge;
        }
    }

    /* Now we have the degree of each node, so we can complete the topology to
     * have a complete balanced tree as requested by the tleaf structure */
    /* Warning: the level for nodes and edges is not expressed as usual: the leaves have
     * 0 and the root has number_of_levels-1 */
    for (int l = num_levels-1; l > 1; l--) { // from the root to the leaves
        UT_array *degrees = down_degrees_by_level[l];
        int max_degree = max_down_degrees_by_level[l];

        int down_level_idx = 0;
        UT_array *down_level_degrees = down_degrees_by_level[l-1];
        int down_level_max_degree = max_down_degrees_by_level[l-1];
        printf("level: %d\n", l);
        for (int d = 0; d < utarray_len(degrees); d++) {
            int degree = *(int *)utarray_eltptr(degrees, d);
            printf("\tdegree: %d\n", degree);

            if (degree > 0) {
                down_level_idx += degree;
                if (degree < max_degree) {
                    int missing_degree = (degree-max_degree)*down_level_max_degree;
                    utarray_insert(down_level_degrees, &missing_degree, down_level_idx);
                    down_level_idx++;
                }
            } else {
                int missing_degree = degree*down_level_max_degree;
                utarray_insert(down_level_degrees, &missing_degree, down_level_idx);
                down_level_idx++;
            }
        }
    }

    /* Complete the ordered list of hosts */
    UT_array *degrees = down_degrees_by_level[1];
    int max_degree = max_down_degrees_by_level[1];
    int idx = 0;
    int host_idx = 0;
    netloc_arch_host_t *numbered_hosts = NULL;
    for (int d = 0; d < utarray_len(degrees); d++) {
        int degree = *(int *)utarray_eltptr(degrees, d);
        int diff;

        if (degree > 0) {
            diff = max_degree-degree;
        } else {
            diff = -degree;
        }

        /* We go through the ordered_hosts */
        for (int i = 0; i < degree; i++) {
            netloc_node_t *node = *(netloc_node_t **)
                utarray_eltptr(ordered_hosts, idx);

            netloc_arch_host_t *host = (netloc_arch_host_t *)
                malloc(sizeof(netloc_arch_host_t));
            strcpy(host->id, node->hostname);
            host->idx = host_idx++;
            HASH_ADD_STR(numbered_hosts, id, host);

            idx++;
        }

        /* we add the missing nodes for the tree to be complete */
        for (int i = 0; i < diff; i++) {
            void *new = NULL;
            netloc_arch_host_t *host = (netloc_arch_host_t *)
                malloc(sizeof(netloc_arch_host_t));
            strcpy(host->id, "");
            host->idx = host_idx++;
            HASH_ADD_STR(numbered_hosts, id, host);
        }
    }
    // TODO

    // scotch->arh
    tree->num_levels = num_levels-1;
    tree->degrees = max_down_degrees_by_level;
    tree->throughput = gbits_by_level;
    tree->hosts = numbered_hosts;

end:
    /* We copy back all userdata */
    netloc_topology_iter_nodes(topology, node, node_tmp) {
        if (!netloc_node_is_in_partition(node, partition))
            continue;
        netloc_analysis_data *analysis_data = (netloc_analysis_data *)node->userdata;
        if (analysis_data->level == -1 && ret != -1) {
            ret = -1;
            printf("The node %s was not browsed\n", node->description);
        }

        node->userdata = analysis_data->userdata;
        free(analysis_data);

        netloc_edge_t *edge, *edge_tmp;
        netloc_node_iter_edges(node, edge, edge_tmp) {
            netloc_analysis_data *analysis_data = (netloc_analysis_data *)edge->userdata;
            node->userdata = analysis_data->userdata;
            free(analysis_data);
        }
    }

    return ret;
}

static int build_subarch(netloc_arch_t *arch, char **nodelist,
        int num_nodes, SCOTCH_Arch *subarch)
{
    int ret;
    if (arch->type == NETLOC_ARCH_TREE) {
        netloc_arch_tree_t *tree = arch->arch.tree;
        int num_cores = tree->num_cores;
        int *idx_list = (int *)malloc(num_nodes*sizeof(int));
        netloc_arch_host_t *all_hosts = tree->hosts;

        char *last_nodename = "";
        netloc_arch_host_t *host;
        int core_idx;
        for (int n = 0; n < num_nodes; n++) {
            char *nodename = nodelist[n];

            /* Another processor from the previous node */
            if (!strcmp(nodename, last_nodename)) {
                if (core_idx >= num_cores) {
                    fprintf(stderr, "Error: node %s is present more than"
                            "%d times in the node list file\n", nodename, core_idx);
                    return NETLOC_ERROR;
                }
            } else {
                core_idx = 0;
                HASH_FIND_STR(all_hosts, nodename, host);
                if (host) {
                    idx_list[n] = host->idx;
                }
                else {
                    fprintf(stderr, "Error: node %s not present in the all architecture\n", nodename);
                    return NETLOC_ERROR;
                }
            }
            idx_list[n] = num_cores*host->idx+core_idx;
            core_idx++;
        }

        ret = SCOTCH_archSub(subarch, arch->scotch, num_nodes, idx_list);
        if (ret != 0) {
            fprintf(stderr, "Error: SCOTCH_archSub failed\n");
            return NETLOC_ERROR;
        }

    } else {
        // TODO
    }

    return NETLOC_SUCCESS;
}

// TODO move in another file
int netloc_topology_find_partition_idx(netloc_topology_t topology, char *partition_name)
{
    if (!partition_name)
        return -1;

    /* Find the selected partition in the topology */
    int p = 0;
    int found = 0;
    while (p < utarray_len(topology->partitions)) {
        char *current_name = *(char **)utarray_eltptr(topology->partitions, p);

        if (!strcmp(current_name, partition_name)) {
            found = 1;
            break;
        }
        p++;
    }

    if (!found)
        return -2;

    return p;
}

static int get_core_number()
{
    char *pbs_num_ppn = getenv("PBS_NUM_PPN");
    if (strlen(pbs_num_ppn)) {
        int num_ppn = atoi(pbs_num_ppn);
        return num_ppn;
    } else {
        printf("Error: you do not use PBS resource manager\n");
        return -1;
    }
}

static int tree_to_scotch_arch(netloc_arch_tree_t *tree, SCOTCH_Arch *scotch)
{
    int ret;
    /* Add hwloc information to go beyond nodes */
    /* TODO improve by using the tree from hwloc */
    tree->num_levels++;
    tree->degrees = (int *)realloc(tree->degrees, tree->num_levels*sizeof(int));
    tree->throughput = (int *)realloc(tree->throughput, tree->num_levels*sizeof(int));

    int num_cores = get_core_number();

    if (num_cores == -1) {
        return NETLOC_ERROR;
    }

    tree->degrees[tree->num_levels-1] = num_cores;
    tree->throughput[tree->num_levels-1] = 1000; // TODO
    tree->num_cores = num_cores;

    ret = SCOTCH_archTleaf(scotch, tree->num_levels, tree->degrees, tree->throughput);
    if (ret != 0) {
        fprintf(stderr, "Error: SCOTCH_archTleaf failed\n");
        return NETLOC_ERROR;
    }

    return NETLOC_SUCCESS;
}

static int build_arch(netloc_arch_t *arch)
{
    char *arch_file = getenv("NETLOCSCOTCH_ARCHFILE");
    char *partition_name = getenv("NETLOCSCOTCH_PARTITION");
    char *path = getenv("NETLOCSCOTCH_TOPODIR");

    if (arch_file) { // TODO XXX
    } else {
        char *netloc_dir;

        if (!strlen(path)) {
            fprintf(stderr, "Error: you need to set NETLOCSCOTCH_TOPODIR in your environment.\n");
            return NETLOC_ERROR;
        }
        asprintf(&netloc_dir, "file://%s/%s", path, "netloc");

        netloc_network_t *network = NULL;
        netloc_topology_t topology;

        // Find a specific InfiniBand network
        network = netloc_dt_network_t_construct();
        network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;

        // Search for the specific network
        int ret = netloc_find_network(netloc_dir, network);
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

        support_load_datafile(topology);

        if (!strlen(partition_name)) {
            fprintf(stderr, "Error: you need to set NETLOCSCOTCH_PARTITION in your environment.\n");
            fprintf(stderr, "\tIt can be: ");
            int num_partitions = utarray_len(topology->partitions);
            for (int p = 0; p < num_partitions; p++) {
                char *partition = *(char **)utarray_eltptr(topology->partitions, p);
                fprintf(stderr, "%s%s", partition, p != num_partitions-1 ? ", ": "\n");
            }
            return NETLOC_ERROR;
        }

        int partition =
            netloc_topology_find_partition_idx(topology, partition_name);

        netloc_arch_tree_t *tree = (netloc_arch_tree_t *)
            malloc(sizeof(netloc_arch_tree_t));
        partition_topology_to_tleaf(topology, partition, tree);
        arch->scotch = (SCOTCH_Arch *)malloc(sizeof(SCOTCH_Arch));
        ret = tree_to_scotch_arch(tree, arch->scotch);
        if( NETLOC_SUCCESS != ret ) {
            return ret;
        }
    }

    return NETLOC_SUCCESS;
}

static int get_current_nodes(int *pnum_nodes, char ***pnodes)
{
    char *pbs_file = getenv("PBS_NODEFILE");
    char *domainname = getenv("NETLOCSCOTCH_DOMAINNAME");
    char *line = NULL;
    size_t size = 0;

    if (strlen(pbs_file)) {
        int num_nodes = atoi(getenv("PBS_NUM_NODES"));
        int by_node = atoi(getenv("PBS_NUM_PPN"));
        int total_num_nodes = num_nodes*by_node;
        FILE *node_file = fopen(pbs_file, "r");
        int found_nodes = 0;
        char **nodes = (char **)malloc(total_num_nodes*sizeof(char *));
        int remove_length = strlen(domainname);
        while (getline(&line, &size, node_file) > 0) {
            if (found_nodes > total_num_nodes) {
                printf("Error: variable PBS_NUM_NODES does not correspond to "
                        "value from PBS_NODEFILE\n");
                return NETLOC_ERROR;
            }
            line[strlen(line)-remove_length-1] = '\0';
            asprintf(nodes+found_nodes, "%s", line);
            found_nodes++;
        }
        *pnodes = nodes;
        *pnum_nodes = total_num_nodes;
    } else { // TODO
        printf("Error: you do not use PBS resource manager\n");
        return NETLOC_ERROR;
    }

    return NETLOC_SUCCESS;
}

/* Need to define some variables in your environment:
 *   - NETLOCSCOTCH_TOPODIR: the path to the netloc topology directory
 *   - NETLOCSCOTCH_PARTITION: name of the current partition (TODO auto find it)
 *   - NETLOCSCOTCH_DOMAINNAME: trailing domain name in the node list
 */
int netlocscotch_build_current_arch(SCOTCH_Arch *subarch)
{
    int ret;
    /* First we need to get the topology of the whole machine */
    netloc_arch_t arch;
    ret = build_arch(&arch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Then we retrieve the list of nodes given by the resource manager */
    int num_nodes;
    char **nodes;
    ret = get_current_nodes(&num_nodes, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Now we can build the sub architecture */
    ret = build_subarch(&arch, nodes, num_nodes, subarch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    return NETLOC_SUCCESS;
}

