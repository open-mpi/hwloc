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
#include <private/netloc.h>
#include <netloc.h>

typedef struct netloc_analysis_data_t {
    int level;
    void *userdata;
} netloc_analysis_data;


static int netloc_arch_add_hwloc(netloc_arch_t *arch);
static int partition_topology_to_tleaf(netloc_topology_t topology,
        int partition, int num_cores, netloc_arch_t *arch);
static void set_gbits(int *values, netloc_edge_t *edge, int num_levels);

/* Complete the topology to have a complete balanced tree  */
/* Warning: the level for nodes and edges is not expressed as usual: the leaves
 * have 0 and the root has number_of_levels-1
 * */
void netloc_complete_tree(netloc_arch_tree_t *tree, UT_array **down_degrees_by_level,
        netloc_arch_host_t **phosts_by_idx)
{
    int num_levels = tree->num_levels;
    int *max_degrees = tree->degrees;

    /* Complete the tree by inserting nodes */
    for (int l = num_levels; l > 1; l--) { // from the root to the leaves
        UT_array *degrees = down_degrees_by_level[l];
        int max_degree = max_degrees[num_levels-l];

        int down_level_idx = 0;
        UT_array *down_level_degrees = down_degrees_by_level[l-1];
        int down_level_max_degree = max_degrees[l-1];
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
    int max_degree = max_degrees[1];
    int idx = 0;
    int ghost_idx = 0;
    netloc_arch_host_t *hosts_by_idx = NULL;
    for (int d = 0; d < utarray_len(degrees); d++) {
        int degree = *(int *)utarray_eltptr(degrees, d);
        int diff;

        if (degree > 0) {
            diff = max_degree-degree;
        } else {
            diff = -degree;
        }

        /* We go through the host_names */
        for (int i = 0; i < degree; i++) {
            netloc_arch_host_t *host = (netloc_arch_host_t *)
                malloc(sizeof(netloc_arch_host_t));
            host->host_idx = idx;
            host->idx = ghost_idx++;
            HASH_ADD_INT(hosts_by_idx, idx, host);

            idx++;
        }

        /* we add the missing nodes for the tree to be complete */
        for (int i = 0; i < diff; i++) {
            void *new = NULL;
            netloc_arch_host_t *host = (netloc_arch_host_t *)
                malloc(sizeof(netloc_arch_host_t));
            host->host_idx = -1;
            host->idx = ghost_idx++;
            HASH_ADD_INT(hosts_by_idx, idx, host);
        }
    }
    *phosts_by_idx = hosts_by_idx;
}


int partition_topology_to_tleaf(netloc_topology_t topology,
        int partition, int num_cores, netloc_arch_t *arch)
{
    int ret = 0;
    UT_array *nodes;
    utarray_new(nodes, &ut_ptr_icd);

    netloc_arch_tree_t *tree = (netloc_arch_tree_t *)
        malloc(sizeof(netloc_arch_tree_t));
    arch->arch.tree = tree;
    arch->type = NETLOC_ARCH_TREE;

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
    UT_array *ordered_name_array;
    UT_array **down_degrees_by_level;
    int *max_down_degrees_by_level;
    int *gbits_by_level;

    utarray_new(ordered_name_array, &ut_ptr_icd);

    down_degrees_by_level = (UT_array **)malloc(num_levels*sizeof(UT_array *));
    for (int l = 0; l < num_levels; l++) {
        utarray_new(down_degrees_by_level[l], &ut_int_icd);
    }
    max_down_degrees_by_level = (int *)calloc(num_levels-1, sizeof(int));
    gbits_by_level = (int *)calloc(num_levels-1, sizeof(int));

    UT_array *down_edges;
    utarray_new(down_edges, &ut_ptr_icd);
    netloc_edge_t *up_edge = current_node->edges;
    utarray_push_back(ordered_name_array, &current_node);
    while (1) {
        if (utarray_len(down_edges)) {
            netloc_edge_t *down_edge = *(void **)utarray_back(down_edges);
            utarray_pop_back(down_edges);
            netloc_node_t *dest_node = down_edge->dest;
            if (netloc_node_is_host(dest_node)) {
                set_gbits(gbits_by_level, down_edge, num_levels);
                utarray_push_back(ordered_name_array, &dest_node);
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

    tree->num_levels = num_levels-1;
    tree->degrees = max_down_degrees_by_level;
    tree->throughput = gbits_by_level;
    tree->num_cores = num_cores;

    /* Now we have the degree of each node, so we can complete the topology to
     * have a complete balanced tree as requested by the tleaf structure */
    netloc_arch_host_t *numbered_hosts;

    netloc_complete_tree(tree, down_degrees_by_level, &numbered_hosts);

    arch->num_hosts = utarray_len(ordered_name_array);
    arch->idx_hosts = numbered_hosts;
    arch->hosts = (netloc_node_t **)ordered_name_array->d;

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

void set_gbits(int *values, netloc_edge_t *edge, int num_levels)
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

typedef struct {
    UT_hash_handle hh;
    char name[15];
    netloc_arch_host_t *host;
} node_by_name_t;

/* Given a list of node names, we get a list of hosts picked from the complete
 * architecture arch
 */
int netloc_arch_find_current_hosts(netloc_arch_t *arch, char **nodelist,
        int num_nodes, netloc_arch_host_t ***phost_list)
{
    /* Build nodes_by_name */
    netloc_arch_host_t *current_host = arch->idx_hosts;
    node_by_name_t *nodes_by_name = NULL;
    while (current_host) {
        if (current_host->host_idx != -1) {
            char *hostname =
                ((netloc_node_t **)arch->hosts)[current_host->host_idx]->hostname;
            node_by_name_t *node_by_name = (node_by_name_t *)
                malloc(sizeof(node_by_name_t));
            strcpy(node_by_name->name, hostname);
            node_by_name->host = current_host;
            HASH_ADD_STR(nodes_by_name, name, node_by_name);
        }
        current_host = current_host->hh.next;
    }


    netloc_arch_host_t **host_list = (netloc_arch_host_t **)
        malloc(num_nodes*sizeof(netloc_arch_host_t *));

    char *last_nodename = "";
    netloc_arch_host_t *host;
    for (int n = 0; n < num_nodes; n++) {
        char *nodename = nodelist[n];

        /* Another processor from the previous node */
        if (strcmp(nodename, last_nodename)) {
            node_by_name_t *node_by_name;
            HASH_FIND_STR(nodes_by_name, nodename, node_by_name);
            last_nodename = nodename;
            if (!node_by_name) {
                fprintf(stderr, "Error: node %s not present in the all architecture\n", nodename);
                return NETLOC_ERROR;
            }
            host = node_by_name->host;
        }
        host_list[n] = host;
    }

    *phost_list = host_list;
    return NETLOC_SUCCESS;
}

int netloc_arch_add_hwloc(netloc_arch_t *arch)
{
    int ret;
    /* Add hwloc information to go beyond nodes */
    /* TODO improve by using the tree from hwloc */
    netloc_arch_tree_t *tree = arch->arch.tree;
    tree->num_levels++;
    tree->degrees = (int *)realloc(tree->degrees, tree->num_levels*sizeof(int));
    tree->throughput = (int *)realloc(tree->throughput, tree->num_levels*sizeof(int));

    int num_cores;
    ret = hwloc_get_core_number(&num_cores);

    if (ret != NETLOC_SUCCESS) {
        return ret;
    }

    tree->degrees[tree->num_levels-1] = num_cores;
    tree->throughput[tree->num_levels-1] = 1000; // TODO
    tree->num_cores = num_cores;

    return NETLOC_SUCCESS;
}


int netloc_arch_build(netloc_arch_t *arch, int add_hwloc)
{
    char *arch_file = getenv("NETLOC_ARCHFILE");
    char *partition_name = getenv("NETLOC_PARTITION");
    char *path = getenv("NETLOC_TOPODIR");

    if (arch_file) { // TODO XXX
    } else {
        char *netloc_dir;

        if (!path) {
            fprintf(stderr, "Error: you need to set NETLOC_TOPODIR in your environment.\n");
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
        if (NETLOC_SUCCESS != ret) {
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
        arch->topology = topology;

        if (!partition_name) {
            fprintf(stderr, "Error: you need to set NETLOC_PARTITION in your environment.\n");
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

        if (add_hwloc) {
            int *a = NULL; *a = 1;
            // TODO
            // partition_topology_to_tleaf(topology, partition, XXX, arch);
        } else {
            partition_topology_to_tleaf(topology, partition, 1, arch);
        }

        if (add_hwloc) {
            ret = netloc_arch_add_hwloc(arch);
            if (NETLOC_SUCCESS != ret) {
                return NETLOC_ERROR;
            }
        }
    }

    return NETLOC_SUCCESS;
}
