/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2015-2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <netloc/dc.h>
#include <private/netloc.h>

#include <limits.h>

#include "support.h"

/**
 * Priority Queue support
 */
struct pq_element_t {
    int priority;
    void * data;
};
typedef struct pq_element_t pq_element_t;

struct pq_queue_t {
    int size;
    int alloc;
    pq_element_t *data;
};
typedef struct pq_queue_t pq_queue_t;

static pq_queue_t * pq_queue_t_construct();
static int pq_queue_t_destruct(pq_queue_t *pq);
static int pq_push(pq_queue_t *pq, int priority, void *data);
static void * pq_pop(pq_queue_t *pq);
static void pq_reorder(pq_queue_t *pq, int priority, void *data);
//static void pq_dump(pq_queue_t *pq);

static inline bool pq_is_empty(pq_queue_t *pq) {
    return pq->size == 0;
}

/**
 * Use Dijkstra's shortest path algorithm to calculate the
 * path between the two nodes specified.
 */
static int compute_shortest_path_dijkstra(netloc_data_collection_handle_t *handle,
                                          netloc_node_t *src_node,
                                          netloc_node_t *dest_node,
                                          int *num_edges,
                                          netloc_edge_t ***edges);

/*************************************************************/

int netloc_dc_compute_path_between_nodes(netloc_data_collection_handle_t *handle,
                                         netloc_node_t *src_node,
                                         netloc_node_t *dest_node,
                                         int *num_edges,
                                         netloc_edge_t ***edges,
                                         bool is_logical)
{
    int ret, exit_status = NETLOC_SUCCESS;

    /*
     * Sanity check
     */
    if( NULL == src_node || NULL == dest_node ) {
        fprintf(stderr, "Error: Source or Destination node is NULL\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    if( is_logical ) {
        fprintf(stderr, "Error: Logical Pathfinding not supported\n");
        exit_status = NETLOC_ERROR_NOT_IMPL;
        goto cleanup;
    }

    /*
     * Calculate path between these two nodes
     */
    ret = compute_shortest_path_dijkstra(handle,
                                         src_node,
                                         dest_node,
                                         num_edges,
                                         edges);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    return exit_status;
}

/*************************************************************
 * Support Functionality
 *************************************************************/
static int compute_shortest_path_dijkstra(netloc_data_collection_handle_t *handle,
                                          netloc_node_t *src_node,
                                          netloc_node_t *dest_node,
                                          int *num_edges,
                                          netloc_edge_t ***edges)
{
    int exit_status = NETLOC_SUCCESS;
    int i;
    size_t j;
    pq_queue_t *queue = NULL;
    int *distance = NULL;
    bool *not_seen = NULL;
    netloc_node_t *node_u = NULL;
    netloc_node_t *node_v = NULL;
    netloc_node_t **prev_node = NULL;
    netloc_edge_t **prev_edge = NULL;
    int alt;
    int idx_u, idx_v;

    int num_rev_edges;
    netloc_edge_t **rev_edges = NULL;

    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *cur_node = NULL;

    unsigned long key_int;

    // Just in case things go poorly below
    (*num_edges) = 0;
    (*edges) = NULL;


    /*
     * Allocate some data structures
     */
    queue = pq_queue_t_construct();
    if( NULL == queue ) {
        fprintf(stderr, "Error: Failed to allocate the queue\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    distance = (int*)malloc(sizeof(int) * netloc_lookup_table_size(handle->node_list));
    if( NULL == distance ) {
        fprintf(stderr, "Error: Failed to allocate the distance array\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    not_seen = (bool*)malloc(sizeof(bool) * netloc_lookup_table_size(handle->node_list));
    if( NULL == not_seen ) {
        fprintf(stderr, "Error: Failed to allocate the 'not_seen' array\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    prev_node = (netloc_node_t**)malloc(sizeof(netloc_node_t*) * netloc_lookup_table_size(handle->node_list));
    if( NULL == prev_node ) {
        fprintf(stderr, "Error: Failed to allocate the 'prev_node' array\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    prev_edge = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*) * netloc_lookup_table_size(handle->node_list));
    if( NULL == prev_edge ) {
        fprintf(stderr, "Error: Failed to allocate the 'prev_edge' array\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    /*
     * Initialize the data structures
     */
    // Make sure to initialize the arrays
    for( j = 0; j < netloc_lookup_table_size(handle->node_list); ++j){
        distance[j] = INT_MAX;
        not_seen[j] = true;
        prev_node[j] = NULL;
        prev_edge[j] = NULL;
    }

    i = 0;
    hti = netloc_dt_lookup_table_iterator_t_construct(handle->node_list);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        cur_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == cur_node ) {
            break;
        }

        if( cur_node == src_node ) {
            pq_push(queue, 0, cur_node);
            distance[i] = 0;
        } else {
            pq_push(queue, INT_MAX, cur_node);
            distance[i] = INT_MAX;
        }

        not_seen[i] = true;

        prev_node[i] = NULL;
        prev_edge[i] = NULL;

        cur_node->__uid__ = i;
        ++i;
    }

    /*
     * Search
     */
    while( !pq_is_empty(queue) ) {
        //pq_dump(queue);

        // Grab the next hop
        node_u = pq_pop(queue);
        // Mark as seen
        idx_u = -1;
        i = 0;
        idx_u = node_u->__uid__;
        not_seen[idx_u] = false;

        // For all the edges from this node
        for(i = 0; i < node_u->num_edges; ++i ) {
            node_v = NULL;
            idx_v  = -1;

            // Lookup the "dest" node
            node_v = node_u->edges[i]->dest_node;
            idx_v = node_v->__uid__;

            // If the node has been seen, skip
            if( !not_seen[idx_v] ) {
                continue;
            }

            // Otherwise check to see if we found a shorter path
            // Future Work: Add a weight factor other than 1.
            //              Maybe calculated based on speed/width
            alt = distance[idx_u] + 1;
            if( alt < distance[idx_v] ) {
                distance[idx_v] = alt;
                prev_node[idx_v] = node_u;
                prev_edge[idx_v] = node_u->edges[i];

                // Adjust the priority queue as needed
                pq_reorder(queue, alt, node_v);
            }
        }
    }

    /*
     * Reconstruct the path by picking up the edges
     * The edges will be in reverse order (dest to source).
     */
    num_rev_edges = 0;
    rev_edges     = NULL;

    // Find last hop
    SUPPORT_CONVERT_ADDR_TO_INT(dest_node->physical_id,
                                handle->network->network_type,
                                key_int);
    node_u  = netloc_lookup_table_access_with_int( handle->node_list,
                                                   dest_node->physical_id,
                                                   key_int);
    idx_u = node_u->__uid__;

    node_v = NULL;
    idx_v  = -1;
    while( prev_node[idx_u] != NULL ) {
        // Find the linking edge
        if( node_u != dest_node) {
            for(i = 0; i < node_u->num_edges; ++i ) {
                if( node_v->physical_id_int == node_u->edges[i]->dest_node->physical_id_int ) {
                    ++num_rev_edges;
                    rev_edges = (netloc_edge_t**)realloc(rev_edges, sizeof(netloc_edge_t*) * num_rev_edges);
                    if( NULL == rev_edges ) {
                        fprintf(stderr, "Error: Failed to re-allocate the 'rev_edges' array with %d elements\n",
                                num_rev_edges);
                        exit_status = NETLOC_ERROR;
                        goto cleanup;
                    }
                    rev_edges[num_rev_edges-1] = node_u->edges[i];
                    break;
                }
            }
        }
        node_v = node_u;
        idx_v  = idx_u;

        // Find the next node
        SUPPORT_CONVERT_ADDR_TO_INT(prev_node[idx_u]->physical_id,
                                    handle->network->network_type,
                                    key_int);
        node_u = netloc_lookup_table_access_with_int( handle->node_list,
                                                      prev_node[idx_u]->physical_id,
                                                      key_int);
        idx_u = node_u->__uid__;
    }

    for(i = 0; i < src_node->num_edges; ++i ) {
        if( NULL == node_v ) {
            fprintf(stderr, "Error: This should never happen, but node_v is NULL at line %d in file %s\n",
                    __LINE__, __FILE__);
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }
        if( node_v->physical_id_int == src_node->edges[i]->dest_node->physical_id_int ) {
            ++num_rev_edges;
            rev_edges = (netloc_edge_t**)realloc(rev_edges, sizeof(netloc_edge_t*) * num_rev_edges);
            if( NULL == rev_edges ) {
                fprintf(stderr, "Error: Failed to re-allocate the 'rev_edges' array with %d elements\n",
                        num_rev_edges);
                exit_status = NETLOC_ERROR;
                goto cleanup;
            }
            rev_edges[num_rev_edges-1] = node_u->edges[i];

            break;
        }
    }


    /*
     * Copy the edges back in correct order
     */
    (*num_edges) = num_rev_edges;
    (*edges) = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*) * (*num_edges));
    if( NULL == (*edges) ) {
        fprintf(stderr, "Error: Failed to allocate the edges array\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    for( i = 0; i < num_rev_edges; ++i ) {
        (*edges)[i] = rev_edges[num_rev_edges-1-i];
        //printf("DEBUG: \t Edge: %s\n", netloc_pretty_print_edge_t( (*edges)[i] ) );
    }


    /*
     * Cleanup
     */
 cleanup:
    if( NULL != queue ) {
        pq_queue_t_destruct(queue);
        queue = NULL;
    }

    free(rev_edges);
    free(distance);
    free(not_seen);
    free(prev_node);
    free(prev_edge);

    netloc_dt_lookup_table_iterator_t_destruct(hti);

    return exit_status;
}

static pq_queue_t * pq_queue_t_construct()
{
    pq_queue_t *pq = NULL;

    pq = (pq_queue_t*)malloc(sizeof(pq_queue_t));
    if( NULL == pq ) {
        return NULL;
    }

    pq->size  = 0;
    pq->alloc = 4; // Increment by 4
    pq->data = (pq_element_t*)malloc(sizeof(pq_element_t) * pq->alloc);
    if( NULL == pq->data ) {
        free(pq);
        return NULL;
    }

    return pq;
}

static int pq_queue_t_destruct(pq_queue_t *pq)
{
    if( NULL == pq ) {
        return NETLOC_SUCCESS;
    }

    free(pq->data);
    free(pq);

    return NETLOC_SUCCESS;
}

static int pq_push(pq_queue_t *pq, int priority, void *data)
{
    int i;
    pq_element_t *elem = NULL;

    if( NULL == pq ) {
        return NETLOC_ERROR;
    }

    pq->size++;

    if( pq->size >= pq->alloc ) {
        pq->alloc += 4;
        pq->data = (pq_element_t*)realloc(pq->data, sizeof(pq_element_t) * pq->alloc);
        if( NULL == pq->data ) {
            return NETLOC_ERROR;
        }
    }

    if( 1 == pq->size ) {
        elem = &pq->data[0];
    }
    else {
        elem = &pq->data[pq->size-1];
        // Make a hole in the array to place the new item
        for( i = pq->size-2; i >= 0; --i ) {
            if( pq->data[i].priority > priority ) {
                // Move item backwards
                pq->data[i+1].priority = pq->data[i].priority;
                pq->data[i+1].data     = pq->data[i].data;

                // Save this 'hole'
                elem = &pq->data[i];
                elem->priority = INT_MAX;
                elem->data     = NULL;
            }
        }
    }

    // Insert into the hole
    elem->priority = priority;
    elem->data     = data;

    return NETLOC_SUCCESS;
}

static void * pq_pop(pq_queue_t *pq)
{
    int i;
    void *data = NULL;

    if( NULL == pq ) {
        return NULL;
    }

    data = pq->data[0].data;

    if( 0 == pq->size ) {
        return NULL;
    }

    // Remove top item, shift everyone up
    for(i = 1; i < pq->size; ++i) {
        pq->data[i-1].priority = pq->data[i].priority;
        pq->data[i-1].data     = pq->data[i].data;
    }
    pq->data[pq->size-1].priority = INT_MAX;
    pq->data[pq->size-1].data     = NULL;

    pq->size--;

    return data;
}

static void pq_reorder(pq_queue_t *pq, int priority, void *data)
{
    int i, elem_idx;
    pq_element_t *elem = NULL;

    // Find this element in this array
    elem_idx = -1;
    for(i = 0; i < pq->size; ++i) {
        if( data == pq->data[i].data ) {
            elem_idx = i;
            break;
        }
    }

    if( 0 > elem_idx ) {
        fprintf(stderr, "Error: Could not find item!\n");
        return;
    }

    // Use the elem_idx as a pivot since we have the item information
    // provided as a parameter
    elem = &pq->data[elem_idx];
    elem->priority = INT_MAX;
    elem->data     = NULL;

    // Trivial case - do nothing
    if( 1 == pq->size ) {
        ;
    }
    // Move this item 'up'
    else if( priority < elem->priority ) {
        for(i = elem_idx-1; i >= 0; --i ) {
            if( pq->data[i].priority > priority ) {
                pq->data[i+1].priority = pq->data[i].priority;
                pq->data[i+1].data     = pq->data[i].data;

                elem = &pq->data[i];
                elem->priority = INT_MAX;
                elem->data     = NULL;
            }
        }
    }
    // Move this item 'down'
    else {
        for(i = elem_idx+1; i < pq->size; ++i ) {
            if( pq->data[i].priority < priority ) {
                pq->data[i-1].priority = pq->data[i].priority;
                pq->data[i-1].data     = pq->data[i].data;

                elem = &pq->data[i];
                elem->priority = INT_MAX;
                elem->data     = NULL;
            }
        }
    }

    // Insert into the hole
    elem->priority = priority;
    elem->data     = data;
}

#if 0
static void pq_dump(pq_queue_t *pq)
{
    int i;
    for(i = 0; i < pq->size; ++i) {
        printf("Dump: %3d) ", i);
        if( NULL == pq->data[i].data ) {
            printf("NULL\n");
        } else {
            printf("%3d | %s\n",
                   pq->data[i].priority,
                   netloc_pretty_print_node_t((netloc_node_t*)pq->data[i].data));
        }
    }
    printf("---------------------------------------\n");
}
#endif
