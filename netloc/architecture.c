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


static int partition_topology_to_tleaf(netloc_topology_t topology,
        int partition, int num_cores, netloc_arch_t *arch);
static void set_gbits(int *values, netloc_edge_t *edge, int num_levels);

/* Complete the topology to have a complete balanced tree  */
void netloc_complete_tree(netloc_arch_tree_t *tree, UT_array **down_degrees_by_level,
        int num_hosts, int **parch_idx)
{
    int num_levels = tree->num_levels;
    int *max_degrees = tree->degrees;

    /* Complete the tree by inserting nodes */
    for (int l = 0; l < num_levels-1; l++) { // from the root to the leaves
        int num_degrees = utarray_len(down_degrees_by_level[l]);
        int *degrees = (int *)down_degrees_by_level[l]->d;
        int max_degree = max_degrees[l];

        int down_level_idx = 0;
        UT_array *down_level_degrees = down_degrees_by_level[l+1];
        int down_level_max_degree = max_degrees[l+1];
        for (int d = 0; d < num_degrees; d++) {
            int degree = degrees[d];
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

    /* Indices for the list of hosts, in the complete architecture */
    int num_degrees = utarray_len(down_degrees_by_level[num_levels-1]);
    int *degrees = (int *)down_degrees_by_level[num_levels-1]->d;
    int max_degree = max_degrees[num_levels-1];
    int ghost_idx = 0;
    int idx = 0;
    netloc_arch_node_t *named_nodes = NULL;
    int *arch_idx = (int *)malloc(sizeof(int[num_hosts]));
    for (int d = 0; d < num_degrees; d++) {
        int degree = degrees[d];
        int diff;

        if (degree > 0) {
            diff = max_degree-degree;
        } else {
            diff = -degree;
        }

        for (int i = 0; i < degree; i++) {
            arch_idx[idx++] = ghost_idx++;
        }
        ghost_idx += diff;
    }
    *parch_idx = arch_idx;
}

int netloc_tree_num_leaves(netloc_arch_tree_t *tree)
{
    int num_leaves = 1;
    for (int l = 0; l < tree->num_levels; l++) {
        num_leaves *= tree->degrees[l];
    }
    return num_leaves;
}

static int get_current_resources(int *pnum_nodes, char ***pnodes, int **pslot_idx,
        int **pslot_list, int **prank_list)
{
    char *filename = getenv("NETLOC_CURRENTSLOTS");
    char word[1024];

    if (!filename) {
        return NETLOC_ERROR;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return NETLOC_ERROR;
    }

    fscanf(file, " %1023s", word);
    int num_nodes;
    num_nodes = atoi(word);

    char **nodes = (char **)malloc(sizeof(char *[num_nodes]));
    for (int n = 0; n < num_nodes; n++) {
        fscanf(file, " %1023s", word);
        nodes[n] = strdup(word);
    }

    int *slot_idx = (int *)malloc(sizeof(int[num_nodes+1]));
    slot_idx[0] = 0;
    for (int n = 0; n < num_nodes; n++) {
        fscanf(file, " %1023s", word);
        slot_idx[n+1] = slot_idx[n]+atoi(word);
    }

    int *slot_list = (int *)malloc(sizeof(int[slot_idx[num_nodes]]));
    int *rank_list = (int *)malloc(sizeof(int[slot_idx[num_nodes]]));
    for (int s = 0; s < slot_idx[num_nodes]; s++) {
        fscanf(file, " %1023s", word);
        slot_list[s] = atoi(word);
        fscanf(file, " %1023s", word);
        rank_list[s] = atoi(word);
    }

    *pnum_nodes = num_nodes;
    *pnodes = nodes;
    *pslot_idx = slot_idx;
    *pslot_list = slot_list;
    *prank_list = rank_list;

    return NETLOC_SUCCESS;
}

int netloc_set_current_resources(netloc_arch_t *arch)
{
    int ret;
    int num_nodes;
    char **nodes;
    int *slot_idx;
    int *slot_list;
    int *rank_list;

    ret = get_current_resources(&num_nodes, &nodes, &slot_idx, &slot_list,
            &rank_list);

    if (ret != NETLOC_SUCCESS)
        assert(0); // XXX

    arch->num_current_nodes = num_nodes;
    arch->current_nodes = (int *)
        malloc(sizeof(int[num_nodes]));

    int constant_num_slots = 0;
    for (int n = 0; n < num_nodes; n++) {
        netloc_arch_node_t *node;
        HASH_FIND_STR(arch->nodes_by_name, nodes[n], node);
        if (!node) {
            return NETLOC_ERROR;
        }

        ret = hwloc_to_netloc_arch(node);
        if (ret != NETLOC_SUCCESS)
            return ret;

        arch->current_nodes[n] = node->idx_in_topo;

        int num_slots = slot_idx[n+1]-slot_idx[n];
        node->num_current_slots = num_slots;

        /* FIXME Nodes with different number of slots is not handled yet, because we
         * build the scotch architecture without taking account of the
         * available cores inside nodes, and Scotch is not able to wieght the
         * nodes */
        if (constant_num_slots) {
            if (constant_num_slots != num_slots) {
                fprintf(stderr, "Oups: the same number of cores by node is needed!\n");
                assert(constant_num_slots == num_slots);
            }
        } else {
            constant_num_slots = num_slots;
        }

        node->current_slots = (int *)
            malloc(sizeof(int[num_slots]));
        int num_leaves = netloc_tree_num_leaves(node->slot_tree);
        node->slot_ranks = (int *)
            malloc(sizeof(int[num_leaves]));

        for (int s = slot_idx[n]; s < slot_idx[n+1]; s++) {
            int slot = slot_list[s];
            node->current_slots[s-slot_idx[n]] = node->slot_idx[slot];
            node->slot_ranks[node->slot_idx[slot]] = rank_list[s];
        }
    }

    return NETLOC_SUCCESS;
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
                utarray_push_back(down_degrees_by_level[num_levels-1-level], &num_edges);
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
            utarray_push_back(down_degrees_by_level[num_levels-1-level], &num_edges);
            max_down_degrees_by_level[num_levels-1-level] =
                max_down_degrees_by_level[num_levels-1-level] > num_edges ?
                max_down_degrees_by_level[num_levels-1-level]: num_edges;
            up_edge = new_up_edge;
        }
    }

    tree->num_levels = num_levels-1;
    tree->degrees = max_down_degrees_by_level;
    tree->throughput = gbits_by_level;

    /* Now we have the degree of each node, so we can complete the topology to
     * have a complete balanced tree as requested by the tleaf structure */
    int *arch_idx;
    int num_nodes = utarray_len(ordered_name_array);
    netloc_complete_tree(tree, down_degrees_by_level, num_nodes, &arch_idx);

    netloc_node_t **ordered_nodes = (netloc_node_t **)ordered_name_array->d;
    netloc_arch_node_t *named_nodes = NULL;
    int max_idx_in_topo = -1;
    for (int i = 0; i < num_nodes; i++) {
        netloc_arch_node_t *node = (netloc_arch_node_t *)
            malloc(sizeof(netloc_arch_node_t));
        node->node = ordered_nodes[i];
        node->name = ordered_nodes[i]->hostname;
        node->idx_in_topo = arch_idx[i];
        max_idx_in_topo  = (max_idx_in_topo > arch_idx[i]) ? max_idx_in_topo : arch_idx[i];
        node->num_slots = -1;
        node->slot_idx = NULL;
        HASH_ADD_KEYPTR( hh, named_nodes, node->name, strlen(node->name), node);
    }

    /* Build nodes_by_idx */
    netloc_arch_node_t **nodes_by_idx = (netloc_arch_node_t **)
        malloc(sizeof(netloc_arch_node_t *[max_idx_in_topo+1]));
    netloc_arch_node_t *named_node = named_nodes;
    for (int i = 0; i < num_nodes; i++) {
        nodes_by_idx[named_node->idx_in_topo] = named_node;
        named_node = named_node->hh.next;
    }

    arch->nodes_by_name = named_nodes;
    arch->nodes_by_idx = nodes_by_idx;

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

/* Given a list of node names, we get a list of hosts picked from the complete
 * architecture arch
 */
int netloc_arch_find_current_nodes(netloc_arch_t *arch, char **nodelist,
        int num_nodes, netloc_arch_node_t ***pnode_list)
{
    netloc_arch_node_t **node_list = (netloc_arch_node_t **)
        malloc(num_nodes*sizeof(netloc_arch_node_t *));

    char *last_nodename = "";
    netloc_arch_node_t *all_nodes;
    netloc_arch_node_t *node;
    for (int n = 0; n < num_nodes; n++) {
        char *nodename = nodelist[n];

        /* Another processor from the previous node */
        if (strcmp(nodename, last_nodename)) {
            HASH_FIND_STR(all_nodes, nodename, node);
            last_nodename = nodename;
            if (!node) {
                fprintf(stderr, "Error: node %s not present in the all architecture\n", nodename);
                return NETLOC_ERROR;
            }
        }
        node_list[n] = node;
    }

    *pnode_list = node_list;
    return NETLOC_SUCCESS;
}

int netloc_arch_build(netloc_arch_t *arch)
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

        partition_topology_to_tleaf(topology, partition, 1, arch);
    }

    return NETLOC_SUCCESS;
}
