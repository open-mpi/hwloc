/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <private/netloc.h>

static char *line_get_next_field(char **string);
static void read_partition_list(char *list, UT_array *array);
static int edges_sort_by_dest(netloc_edge_t *a, netloc_edge_t *b);
static int find_reverse_edges(netloc_topology_t *topology);
static void find_similar_nodes(netloc_topology_t *topology);
static int netloc_node_get_virtual_id(char *id);
static int edge_merge_into(netloc_edge_t *dest, netloc_edge_t *src, int keep);

netloc_topology_t *netloc_topology_construct(netloc_network_t *network)
{
    netloc_topology_t *topology = NULL;
    static UT_icd partitions_icd = {sizeof(char *), NULL, NULL, NULL };
    static UT_icd topos_icd = {sizeof(char *), NULL, NULL, NULL };

    /*
     * Allocate Memory
     */
    topology = (netloc_topology_t *)malloc(sizeof(netloc_topology_t) * 1);
    if( NULL == topology ) {
        return NULL;
    }

    /*
     * Initialize the structure
     */
    topology->network    = netloc_network_dup(network);

    topology->nodes_loaded   = false;
    topology->nodes          = NULL;
    topology->physical_links = NULL;
    utarray_new(topology->partitions, &partitions_icd );
    utarray_new(topology->topos, &topos_icd );
    topology->type           = NETLOC_TOPOLOGY_TYPE_INVALID ;

    return topology;
}

int netloc_topology_destruct(netloc_topology_t *topology)
{
    /*
     * Sanity Check
     */
    if( NULL == topology ) {
        fprintf(stderr, "Error: Detaching from a NULL pointer\n");
        return NETLOC_ERROR;
    }

    /*
     * Free Memory
     */
    netloc_network_destruct(topology->network);

    // TODO

    free(topology);

    return NETLOC_SUCCESS;
}

int netloc_topology_load(netloc_topology_t *topology)
{
    int exit_status = NETLOC_SUCCESS;

    if( topology->nodes_loaded ) {
        return NETLOC_SUCCESS;
    }

    topology->nodes = NULL;
    topology->nodesByHostname = NULL;
    topology->physical_links = NULL;
    topology->hwloc_topos = NULL;

    char *path = topology->network->node_uri;

    FILE *input = fopen(path, "r");

    if (!input ) {
        perror("fopen");
        exit(-1);
    }

    int num_nodes;
    fscanf(input , "%d\n,", &num_nodes);

    char *line = NULL;
    size_t linesize = 0;

    /* Read nodes from file */
    for (int n = 0; n < num_nodes; n++) {
        netloc_node_t *node = netloc_node_construct();
        netloc_line_get(&line, &linesize, input);
        char *remain_line = line;

        strcpy(node->physical_id, line_get_next_field(&remain_line));
        node->logical_id = atoi(line_get_next_field(&remain_line));
        node->type = atoi(line_get_next_field(&remain_line));
        read_partition_list(line_get_next_field(&remain_line), node->partitions);
        node->description = strdup(line_get_next_field(&remain_line));
        node->hostname = strdup(line_get_next_field(&remain_line));

        HASH_ADD_STR(topology->nodes, physical_id, node);
        if (strlen(node->hostname) > 0) {
            HASH_ADD_KEYPTR(hh2, topology->nodesByHostname, node->hostname,
                    strlen(node->hostname), node);
        }
    }

    /* Read edges from file */
    for (int n = 0; n < num_nodes; n++) {
        char *field;
        netloc_node_t *node;

        netloc_line_get(&line, &linesize, input);
        char *remain_line = line;

        field = line_get_next_field(&remain_line);
        HASH_FIND_STR(topology->nodes, field, node);

        while ((field = line_get_next_field(&remain_line))) {
            /* There is an edge */
            netloc_edge_t *edge = netloc_edge_construct();

            HASH_FIND_STR(topology->nodes, field, edge->dest);
            edge->total_gbits = strtof(line_get_next_field(&remain_line), NULL);
            read_partition_list(line_get_next_field(&remain_line), edge->partitions);

            edge->node = node;
            HASH_ADD_PTR(node->edges, dest, edge);

            /* Read links */
            int num_links = atoi(line_get_next_field(&remain_line));
            utarray_reserve(edge->physical_links, num_links);
            utarray_reserve(node->physical_links, num_links);
            for (int i = 0; i < num_links; i++) {
                netloc_physical_link_t *link;
                link =  netloc_physical_link_construct();

                link->id = atoi(line_get_next_field(&remain_line));

                link->src = node;
                link->dest = edge->dest;

                link->ports[0] = atoi(line_get_next_field(&remain_line));
                link->ports[1] = atoi(line_get_next_field(&remain_line));

                link->width = strdup(line_get_next_field(&remain_line));
                link->speed = strdup(line_get_next_field(&remain_line));
                link->gbits = strtof(line_get_next_field(&remain_line), NULL);
                link->description = strdup(line_get_next_field(&remain_line));
                link->other_way_id = atoi(line_get_next_field(&remain_line));

                read_partition_list(line_get_next_field(&remain_line),
                        link->partitions);

                HASH_ADD_INT(topology->physical_links, id, link);

                utarray_push_back(node->physical_links, &link);
                utarray_push_back(edge->physical_links, &link);
            }

        }
        HASH_SRT(hh, node->edges, edges_sort_by_dest);
    }

    /* Read partitions from file */
    {
        netloc_line_get(&line, &linesize, input);
        char *remain_line = line;
        char *field;

        while ((field = line_get_next_field(&remain_line))) {
            char *name = strdup(field);
            utarray_push_back(topology->partitions, &name);
        }
    }

    /* Read paths */
    while (netloc_line_get(&line, &linesize, input) != -1) {
        netloc_node_t *node;
        netloc_path_t *path;
        char *field;

        char *remain_line = line;
        char *src_id = line_get_next_field(&remain_line);
        char *dest_id = line_get_next_field(&remain_line);

        HASH_FIND_STR(topology->nodes, src_id, node);

        path = (netloc_path_t *)malloc(sizeof(netloc_path_t));
        strcpy(path->dest_id, dest_id);
        utarray_new(path->links, &ut_ptr_icd);

        while ((field = line_get_next_field(&remain_line))) {
            int link_id = atoi(field);
            netloc_physical_link_t *link;

            HASH_FIND_INT(topology->physical_links, &link_id, link);
            utarray_push_back(path->links, &link);
        }

        HASH_ADD_STR(node->paths, dest_id, path);
    }

    find_reverse_edges( topology);

    fclose(input);

    find_similar_nodes(topology);

    return exit_status;
}

int netloc_topology_copy(netloc_topology_t *src, netloc_topology_t **dup)
{
   netloc_topology_t *new = (netloc_topology_t *)
       malloc(sizeof(netloc_topology_t));

   new->network = src->network;
   new->network->refcount++;

   new->nodes_loaded = src->nodes_loaded;

   // TODO
    return 0;
}

int netloc_topology_find_partition_idx(netloc_topology_t *topology, char *partition_name)
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

static char *line_get_next_field(char **string)
{
    return netloc_line_get_next_token(string, ',');
}

static void read_partition_list(char *list, UT_array *array)
{
    char *partition;
    if (!strlen(list))
        return;
    while ((partition = netloc_line_get_next_token(&list, ':'))) {
        int partition_num = atoi(partition);
        utarray_push_back(array, &partition_num);
    }
}

static int edges_sort_by_dest(netloc_edge_t *a, netloc_edge_t *b)
{
    if (a->dest == b->dest)
        return 0;
    return (a->dest < b->dest) ? -1 : 1;
}

static int find_reverse_edges(netloc_topology_t *topology)
{
    netloc_node_t *node, *node_tmp;
    HASH_ITER(hh, topology->nodes, node, node_tmp) {
        netloc_edge_t *edge, *edge_tmp;
        netloc_node_iter_edges(node, edge, edge_tmp) {
            netloc_node_t *dest = edge->dest;
            if (dest > node) {
                netloc_edge_t *reverse_edge;
                HASH_FIND_PTR(dest->edges, &node, reverse_edge);
                edge->other_way = reverse_edge;
                reverse_edge->other_way = edge;
            }
        }
    }
    return 0;
}

static void find_similar_nodes(netloc_topology_t * topology)
{
    /* Build edge lists by node */
    int num_nodes = HASH_COUNT(topology->nodes);
    netloc_node_t **nodes = (netloc_node_t **)malloc(num_nodes*sizeof(netloc_node_t *));
    netloc_node_t ***edgedest_by_node = (netloc_node_t ***)malloc(num_nodes*sizeof(netloc_node_t **));
    int *num_edges_by_node = (int *)malloc(num_nodes*sizeof(int));
    netloc_node_t *node, *node_tmp;
    int idx = -1;
    netloc_topology_iter_nodes(topology, node, node_tmp) {
        idx++;
        if (netloc_node_is_host(node)) {
            nodes[idx] = NULL;
            continue;
        }
        int num_edges = HASH_COUNT(node->edges);
        nodes[idx] = node;
        num_edges_by_node[idx] = num_edges;
        edgedest_by_node[idx] = (netloc_node_t **)malloc(num_edges*sizeof(netloc_node_t *));

        netloc_edge_t *edge, *edge_tmp;
        int edge_idx = 0;
        netloc_node_iter_edges(node, edge, edge_tmp) {
            edgedest_by_node[idx][edge_idx] = edge->dest;
            edge_idx++;
        }
    }

    /* We compare the edge lits to find similar nodes */
    UT_array *similar_nodes;
    utarray_new(similar_nodes, &ut_ptr_icd);
    for (int idx1 = 0; idx1 < num_nodes; idx1++) {
        netloc_node_t *node1 = nodes[idx1];
        netloc_node_t *virtual_node = NULL;
        netloc_edge_t *first_virtual_edge = NULL;
        if (!node1)
            continue;
        for (int idx2 = idx1+1; idx2 < num_nodes; idx2++) {
            netloc_node_t *node2 = nodes[idx2];
            if (!node2)
                continue;
            if (num_edges_by_node[idx2] != num_edges_by_node[idx1])
                continue;
            if (idx2 == idx1)
                continue;

            int equal = 1;
            for (int i = 0; i < num_edges_by_node[idx1]; i++) {
                if (edgedest_by_node[idx2][i] != edgedest_by_node[idx1][i]) {
                    equal = 0;
                    break;
                }
            }

            /* If we have similar nodes */
            if (equal) {
                /* We create a new virtual node to contain all of them */
                if (!virtual_node) {
                    virtual_node = netloc_node_construct();
                    netloc_node_get_virtual_id(virtual_node->physical_id);

                    virtual_node->type = node1->type;
                    utarray_concat(virtual_node->physical_links, node1->physical_links);
                    virtual_node->description = strdup(virtual_node->physical_id);

                    utarray_push_back(virtual_node->subnodes, &node1);
                    utarray_concat(virtual_node->partitions, node1->partitions);

                    // TODO paths

                    /* Set edges */
                    netloc_edge_t *edge1, *edge_tmp1;
                    netloc_node_iter_edges(node1, edge1, edge_tmp1) {
                        netloc_edge_t *virtual_edge = netloc_edge_construct();
                        if (!first_virtual_edge)
                            first_virtual_edge = virtual_edge;
                        virtual_edge->node = virtual_node;
                        virtual_edge->dest = edge1->dest;
                        edge_merge_into(virtual_edge, edge1, 0);
                        HASH_ADD_PTR(virtual_node->edges, dest, virtual_edge);

                        /* Change the reverse edge of the neighbours (reverse nodes) */
                        netloc_node_t *reverse_node = edge1->dest;
                        netloc_edge_t *reverse_edge = edge1->other_way;

                        netloc_edge_t *reverse_virtual_edge =
                            netloc_edge_construct();
                        reverse_virtual_edge->dest = virtual_node;
                        reverse_virtual_edge->node = reverse_node;
                        reverse_virtual_edge->other_way = virtual_edge;
                        virtual_edge->other_way = reverse_virtual_edge;
                        HASH_ADD_PTR(reverse_node->edges, dest, reverse_virtual_edge);
                        edge_merge_into(reverse_virtual_edge, reverse_edge, 1);
                        HASH_DEL(reverse_node->edges, reverse_edge);
                    }

                    /* We remove the node from the list of nodes */
                    HASH_DEL(topology->nodes, node1);
                    HASH_ADD_STR(topology->nodes, physical_id, virtual_node);
                    printf("First node found: %s (%s)\n", node1->description, node1->physical_id);
                }

                utarray_concat(virtual_node->physical_links, node2->physical_links);
                utarray_push_back(virtual_node->subnodes, &node2);
                utarray_concat(virtual_node->partitions, node2->partitions);

                /* Set edges */
                netloc_edge_t *edge2, *edge_tmp2;
                netloc_edge_t *virtual_edge = first_virtual_edge;
                netloc_node_iter_edges(node2, edge2, edge_tmp2) {
                    /* Merge the edges from the physical node into the virtual node */
                    edge_merge_into(virtual_edge, edge2, 0);

                    /* Change the reverse edge of the neighbours (reverse nodes) */
                    netloc_node_t *reverse_node = edge2->dest;
                    netloc_edge_t *reverse_edge = edge2->other_way;

                    netloc_edge_t *reverse_virtual_edge;
                    HASH_FIND_PTR(reverse_node->edges, &virtual_node,
                            reverse_virtual_edge);
                    edge_merge_into(reverse_virtual_edge, reverse_edge, 1);
                    HASH_DEL(reverse_node->edges, reverse_edge);

                    /* Get the next edge */
                    virtual_edge = virtual_edge->hh.next;
                }

                /* We remove the node from the list of nodes */
                HASH_DEL(topology->nodes, node2);
                printf("\t node found: %s (%s)\n", node2->description, node2->physical_id);

                nodes[idx2] = NULL;
            }
        }
        utarray_clear(similar_nodes);
    }
}

static int netloc_node_get_virtual_id(char *id)
{
    static int virtual_id = 0;
    sprintf(id, "virtual%d", virtual_id++);
    return 0;
}

static int edge_merge_into(netloc_edge_t *dest, netloc_edge_t *src, int keep)
{
    utarray_concat(dest->physical_links, src->physical_links);
    dest->total_gbits += src->total_gbits;
    utarray_concat(dest->partitions, src->partitions);
    /* TODO XXX modify to avoid duplicates */
    if (keep)
        utarray_push_back(dest->subnode_edges, &src);

    return 0;
}

