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
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <scotch.h>

#include <netloc.h>
#include <netlocscotch.h>
#include <private/netloc.h>
#include <hwloc.h>
#include "support.h"

static int arch_to_scotch_arch(netloc_arch_t *arch, SCOTCH_Arch *scotch);
static int host_get_idx(netloc_arch_t *arch, int num_hosts,
        netloc_arch_host_t **host_list, int **pidx);
static int comm_matrix_to_scotch_graph(double **matrix, int n, SCOTCH_Graph *graph);
int build_scotch_graph(int n, SCOTCH_Graph *graph);

int host_get_idx(netloc_arch_t *arch, int num_hosts,
        netloc_arch_host_t **host_list, int **pidx)
{
    int core_idx = 0;
    netloc_arch_host_t *host;
    netloc_arch_host_t *last_host = NULL;

    int *idx = (int *)malloc(num_hosts*sizeof(int));

    int num_cores = arch->arch.tree->num_cores;

    for (int n = 0; n < num_hosts; n++) {
        netloc_arch_host_t *host = host_list[n];
        if (host == last_host) {
            core_idx++;
        } else {
            core_idx = 0;
            last_host = host;
        }
        idx[n] = host->idx*num_cores+core_idx;
    }

    *pidx = idx;

    return NETLOC_SUCCESS;
}

static int compareint(void const *a, void const *b)
{
   int const *pa = a;
   int const *pb = b;
   return *pa - *pb;
}

static int build_subarch(netloc_arch_t *arch, char **nodelist,
        int num_nodes, SCOTCH_Arch *scotch, SCOTCH_Arch *subarch,
        int **pnode_idx)
{
    int ret;
    if (arch->type == NETLOC_ARCH_TREE) {
        netloc_arch_host_t **host_list;
        ret = netloc_arch_find_current_hosts(arch, nodelist, num_nodes, &host_list);
        if (ret != 0) {
            fprintf(stderr, "Error: netloc_arch_find_current_hosts\n");
            return NETLOC_ERROR;
        }

        int *idx_list;
        host_get_idx(arch, num_nodes, host_list, &idx_list);

        /* Hack to avoid problem with unsorted node list in the subarch and scotch */
        qsort(idx_list, num_nodes, sizeof(int), compareint); // FIXME XXX
        *pnode_idx = idx_list;

        ret = SCOTCH_archSub(subarch, scotch, num_nodes, idx_list);
        if (ret != 0) {
            fprintf(stderr, "Error: SCOTCH_archSub failed\n");
            return NETLOC_ERROR;
        }

    } else {
        // TODO
    }

    return NETLOC_SUCCESS;
}

int arch_to_scotch_arch(netloc_arch_t *arch, SCOTCH_Arch *scotch)
{
    netloc_arch_tree_t *tree = arch->arch.tree;
    int ret;

    ret = SCOTCH_archTleaf(scotch, tree->num_levels, tree->degrees, tree->throughput);
    if (ret != 0) {
        fprintf(stderr, "Error: SCOTCH_archTleaf failed\n");
        return NETLOC_ERROR;
    }

    return NETLOC_SUCCESS;
}

int netlocscotch_build_current_arch(SCOTCH_Arch *scotch_subarch)
{
    int ret;
    /* First we need to get the topology of the whole machine */
    netloc_arch_t arch;
    ret = netloc_arch_build(&arch, 1);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    SCOTCH_Arch *scotch_arch;
    scotch_arch = (SCOTCH_Arch *)malloc(sizeof(SCOTCH_Arch));
    ret = arch_to_scotch_arch(&arch, scotch_arch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Then we retrieve the list of cores given by the resource manager */
    int num_nodes;
    char **nodes;
    ret = netloc_get_current_cores(&num_nodes, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Now we can build the sub architecture */
    int *node_idx;
    ret = build_subarch(&arch, nodes, num_nodes, scotch_arch, scotch_subarch, &node_idx);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    return NETLOC_SUCCESS;
}

static int build_subgraph(SCOTCH_Graph *graph, int *vertices, int num_vertices,
        SCOTCH_Graph *nodegraph)
{
    int ret;

    SCOTCH_Num base;       /* Base value               */
    SCOTCH_Num vert;       /* Number of vertices       */
    SCOTCH_Num *verttab;   /* Vertex array [vertnbr+1] */
    SCOTCH_Num *vendtab;   /* Vertex array [vertnbr]   */
    SCOTCH_Num *velotab;   /* Vertex load array        */
    SCOTCH_Num *vlbltab;   /* Vertex label array       */
    SCOTCH_Num edge;       /* Number of edges (arcs)   */
    SCOTCH_Num *edgetab;   /* Edge array [edgenbr]     */
    SCOTCH_Num *edlotab;   /* Edge load array          */

    SCOTCH_graphData(graph, &base, &vert, &verttab, &vendtab, &velotab,
            &vlbltab, &edge, &edgetab, &edlotab);

    int *vertex_is_present = (int *)malloc(vert*sizeof(int));
    for (int v = 0; v < vert; v++) {
        vertex_is_present[v] = -1;
    }
    for (int v = 0; v < num_vertices; v++) {
        vertex_is_present[vertices[v]] = v;
    }

    // TODO handle other cases 
    if (vendtab) {
        for (int i = 0; i < vert; i++) {
            assert(vendtab[i] == verttab[i+1]);
        }
    }

    SCOTCH_Num *new_verttab;   /* Vertex array [vertnbr+1] */
    SCOTCH_Num *new_vendtab;   /* Vertex array [vertnbr]   */
    SCOTCH_Num *new_velotab;   /* Vertex load array        */
    SCOTCH_Num *new_vlbltab;   /* Vertex label array       */
    SCOTCH_Num new_edge;       /* Number of edges (arcs)   */
    SCOTCH_Num *new_edgetab;   /* Edge array [edgenbr]     */
    SCOTCH_Num *new_edlotab;   /* Edge load array          */

    new_verttab = (SCOTCH_Num *)malloc((num_vertices+1)*sizeof(SCOTCH_Num));
    new_vendtab = NULL;
    if (velotab)
        new_velotab = (SCOTCH_Num *)malloc(num_vertices*sizeof(SCOTCH_Num));
    else
        new_velotab = NULL;
    if (vlbltab)
        new_vlbltab = (SCOTCH_Num *)malloc(num_vertices*sizeof(SCOTCH_Num));
    else
        new_vlbltab = NULL;

    new_edgetab = (SCOTCH_Num *)malloc(edge*sizeof(SCOTCH_Num));
    new_edlotab = (SCOTCH_Num *)malloc(edge*sizeof(SCOTCH_Num));

    int edge_idx = 0;
    new_verttab[0] = 0;
    for (int v = 0; v < num_vertices; v++) {
        if (velotab)
            new_velotab[v] = velotab[vertices[v]];
        if (vlbltab)
            new_vlbltab[v] = vlbltab[vertices[v]];

        for (int e = verttab[vertices[v]]; e < verttab[vertices[v]+1]; e++) {
            int dest_vertex = edgetab[e];
            int new_dest = vertex_is_present[dest_vertex];
            if (new_dest != -1) {
                new_edgetab[edge_idx] = new_dest;
                new_edlotab[edge_idx] = edlotab[e];
                edge_idx++;
            }
        }
        new_verttab[v+1] = edge_idx;
    }

    new_edge = edge_idx;
    new_edgetab = (SCOTCH_Num *)
        realloc(new_edgetab, new_edge*sizeof(SCOTCH_Num));
    new_edlotab = (SCOTCH_Num *)
        realloc(new_edlotab, new_edge*sizeof(SCOTCH_Num));

    ret = SCOTCH_graphBuild (nodegraph, base, num_vertices,
                new_verttab, new_vendtab, new_velotab, new_vlbltab,
                new_edge, new_edgetab, new_edlotab);

    return NETLOC_SUCCESS;
}

int netlocscotch_get_mapping_from_graph(SCOTCH_Graph *graph,
        netlocscotch_core_t **pcores)
{
    int ret;
    /* First we need to get the topology of the whole machine */
    netloc_arch_t arch;
    ret = netloc_arch_build(&arch, 0);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    netloc_topology_t topology = arch.topology;

    SCOTCH_Arch scotch_arch;
    ret = arch_to_scotch_arch(&arch, &scotch_arch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    /* Then we retrieve the list of nodes given by the resource manager */
    int num_nodes;
    char **nodes;
    ret = netloc_get_current_nodes(&num_nodes, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }
    /* TODO another way for retrieving the list of nodes, cores and corresponding
     * ranks: use MPI, hostname and hwloc (see mpi.c)
     */

    /* Now we can build the sub architecture */
    SCOTCH_Arch scotch_subarch;
    int *node_idx;
    ret = build_subarch(&arch, nodes, num_nodes, &scotch_arch, &scotch_subarch, &node_idx);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }

    int graph_size;
    SCOTCH_graphSize(graph, &graph_size, NULL);

    SCOTCH_Strat strategy;
    SCOTCH_stratInit(&strategy);

    netlocscotch_core_t *cores = (netlocscotch_core_t *)
        malloc(graph_size*sizeof(netlocscotch_core_t));

    /* The ranks are the indices of the nodes in the complete graph */
    int *ranks = (int *)malloc(graph_size*sizeof(int));
    ret = SCOTCH_graphMap(graph, &scotch_subarch, &strategy, ranks);

    if (ret != 0) {
        fprintf(stderr, "Error: SCOTCH_graphMap failed\n");
        return NETLOC_ERROR;
    }

    /* We have the mapping but only for the nodes, not inside the nodes */

    UT_array *process_by_node[num_nodes];
    for (int n = 0; n < num_nodes; n++) {
        utarray_new(process_by_node[n], &ut_int_icd);
    }

    /* Find the processes mapped to the nodes */
    for (int p = 0; p < graph_size; p++) {
        int rank = ranks[p];
        utarray_push_back(process_by_node[rank], &p);
    }

    /* Get intranode topology from hwloc */
    if (!topology->hwloc_topos) {
        ret = netloc_read_hwloc(topology);
        if (ret != NETLOC_SUCCESS) {
            fprintf(stderr, "Error: netloc_read_hwloc failed\n");
            return NETLOC_ERROR;
        }
    }

    /* Use the intranode topology */
    for (int n = 0; n < num_nodes; n++) {
        int *process_list = (int *)process_by_node[n]->d;
        int num_processes = utarray_len(process_by_node[n]);
        char *nodename = ((netloc_node_t **)arch.hosts)[node_idx[n]]->hostname;
        int node_ranks[num_processes];

        /* We need to extract the subgraph with only the vertices mapped to the
         * current node */
        SCOTCH_Graph nodegraph; /* graph with only elements for node n */
        build_subgraph(graph, process_list, num_processes, &nodegraph);

        /* Get the architecture from hwloc */
        netloc_node_t *node;
        HASH_FIND_STR(topology->nodesByHostname, nodename, node);
        HASH_FIND(hh2, topology->nodesByHostname, nodename, strlen(nodename),
                node);
        if (!node) {
            return NETLOC_ERROR;
        }

        hwloc_topology_t hwloc_topology =
            topology->hwloc_topos[node->hwlocTopoIdx];
        netloc_arch_t nodearch;
        ret = hwloc_to_netloc_arch(hwloc_topology, &nodearch);

        SCOTCH_Arch scotch_nodearch;
        ret = arch_to_scotch_arch(&nodearch, &scotch_nodearch);

        /* Find the mapping to the cores */
        ret = SCOTCH_graphMap(&nodegraph, &scotch_nodearch, &strategy, node_ranks);
        if (ret != 0) {
            fprintf(stderr, "Error: SCOTCH_graphMap failed\n");
            return NETLOC_ERROR;
        }

        /* Report the node ranks in the global rank array */
        for (int p = 0; p < num_processes; p++) {
            int process = process_list[p];
            cores[process].core = node_ranks[p];
            cores[process].node = node;
        }
    }

    *pcores = cores;

    return NETLOC_SUCCESS;
}

int netlocscotch_get_mapping_from_comm_matrix(double **comm, int num_vertices,
        netlocscotch_core_t **pcores)
{
    int ret;

    SCOTCH_Graph graph;
    ret = comm_matrix_to_scotch_graph(comm, num_vertices, &graph);

    ret = netlocscotch_get_mapping_from_graph(&graph, pcores);

    return ret;
}

/* NULL terminated array */
int netlocscotch_get_mapping_from_comm_file(char *filename, int *pnum_processes,
        netlocscotch_core_t **pcores)
{
    int n;
    double **mat;
    double *sum_row;

    int ret;
    ret = netloc_build_comm_mat(filename, &n, &mat, &sum_row);

    if (ret != NETLOC_SUCCESS) {
        return ret;
    }

    *pnum_processes = n;

    return netlocscotch_get_mapping_from_comm_matrix(mat, n, pcores);
}

static int comm_matrix_to_scotch_graph(double **matrix, int n, SCOTCH_Graph *graph)
{
    int ret;

    SCOTCH_Num base;       /* Base value               */
    SCOTCH_Num vert;       /* Number of vertices       */
    SCOTCH_Num *verttab;   /* Vertex array [vertnbr+1] */
    SCOTCH_Num *vendtab;   /* Vertex array [vertnbr]   */
    SCOTCH_Num *velotab;   /* Vertex load array        */
    SCOTCH_Num *vlbltab;   /* Vertex label array       */
    SCOTCH_Num edge;       /* Number of edges (arcs)   */
    SCOTCH_Num *edgetab;   /* Edge array [edgenbr]     */
    SCOTCH_Num *edlotab;   /* Edge load array          */

    base = 0;
    vert = n;

    verttab = (SCOTCH_Num *)malloc((vert+1)*sizeof(SCOTCH_Num));
    for (int v = 0; v < vert+1; v++) {
        verttab[v] = v*(n-1);
    }

    vendtab = NULL;
    velotab = NULL;
    vlbltab = NULL;

    edge = n*(n-1);

    /* Compute the lowest load to reduce of the values of the load to avoid overflow */
    double min_load = -1;
    for (int v1 = 0; v1 < vert; v1++) {
        for (int v2 = 0; v2 < vert; v2++) {
            double load = matrix[v1][v2];
            if (load >= 0.01 && (load < min_load || min_load < 0)) /* TODO set an epsilon */
                min_load = load;
        }
    }


    edgetab = (SCOTCH_Num *)malloc(n*(n-1)*sizeof(SCOTCH_Num));
    edlotab = (SCOTCH_Num *)malloc(n*(n-1)*sizeof(SCOTCH_Num));
    for (int v1 = 0; v1 < vert; v1++) {
        for (int v2 = 0; v2 < vert; v2++) {
            if (v2 == v1)
                continue;
            int idx = v1*(n-1)+((v2 < v1) ? v2: v2-1);
            edgetab[idx] = v2;
            edlotab[idx] = (int)(matrix[v1][v2]/min_load);
        }
    }

    ret = SCOTCH_graphBuild (graph, base, vert,
                verttab, vendtab, velotab, vlbltab,
                edge, edgetab, edlotab);

    return NETLOC_SUCCESS;
}

int build_scotch_graph(int n, SCOTCH_Graph *graph)
{
    int ret;

    SCOTCH_Num base;       /* Base value               */
    SCOTCH_Num vert;       /* Number of vertices       */
    SCOTCH_Num *verttab;   /* Vertex array [vertnbr+1] */
    SCOTCH_Num *vendtab;   /* Vertex array [vertnbr]   */
    SCOTCH_Num *velotab;   /* Vertex load array        */
    SCOTCH_Num *vlbltab;   /* Vertex label array       */
    SCOTCH_Num edge;       /* Number of edges (arcs)   */
    SCOTCH_Num *edgetab;   /* Edge array [edgenbr]     */
    SCOTCH_Num *edlotab;   /* Edge load array          */

    base = 0;
    vert = n;

    verttab = (SCOTCH_Num *)malloc((vert+1)*sizeof(SCOTCH_Num));
    for (int v = 0; v < vert+1; v++) {
        verttab[v] = v*(n-1);
    }

    vendtab = NULL;
    velotab = NULL;
    vlbltab = NULL;

    edge = n*(n-1);

    edgetab = (SCOTCH_Num *)malloc(n*(n-1)*sizeof(SCOTCH_Num));
    edlotab = (SCOTCH_Num *)malloc(n*(n-1)*sizeof(SCOTCH_Num));
    for (int v1 = 0; v1 < vert; v1++) {
        for (int v2 = 0; v2 < vert; v2++) {
            if (v2 == v1)
                continue;
            int idx = v1*(n-1)+((v2 < v1) ? v2: v2-1);
            edgetab[idx] = v2;
            edlotab[idx] = 1;
        }
    }

    ret = SCOTCH_graphBuild (graph, base, vert,
                verttab, vendtab, velotab, vlbltab,
                edge, edgetab, edlotab);

    return 0;
}
