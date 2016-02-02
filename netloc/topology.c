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

#include <netloc.h>
#include "support.h"

int netloc_attach(struct netloc_topology ** topology_ptr, netloc_network_t network)
{
    struct netloc_topology *topology = NULL;

    /*
     * Allocate Memory
     */
    topology = (struct netloc_topology *)malloc(sizeof(struct netloc_topology) * 1);
    if( NULL == topology ) {
        return NETLOC_ERROR;
    }

    /*
     * Initialize the structure
     */
    topology->network    = netloc_dt_network_t_dup(&network);

    topology->nodes_loaded   = false;
    topology->num_nodes      = 0;
    topology->nodes          = NULL;
    topology->edges          = NULL;
    topology->partitions     = NULL;
    topology->num_partitions = 0;
    topology->topos          = NULL;
    topology->num_topos      = 0;

    /*
     * Make the pointer live
     */
    (*topology_ptr) = topology;

    return NETLOC_SUCCESS;
}

int netloc_detach(struct netloc_topology * topology)
{
    int i;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_edge_t *cur_edge = NULL;

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
    netloc_dt_network_t_destruct(topology->network);

    if( NULL != topology->edges ) {
        hti = netloc_dt_lookup_table_iterator_t_construct(topology->edges);
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            cur_edge = (netloc_edge_t*)netloc_lookup_table_iterator_next_entry(hti);
            if( NULL == cur_edge ) {
                break;
            }
            netloc_dt_edge_t_destruct(cur_edge);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        netloc_lookup_table_destroy(topology->edges);
        free(topology->edges);
        topology->edges = NULL;
    }

    if( NULL != topology->nodes ) {
        for(i = 0; i < topology->num_nodes; ++i ) {
            if( NULL != topology->nodes[i] ) {
                netloc_dt_node_t_destruct(topology->nodes[i]);
                topology->nodes[i] = NULL;
            }
        }
        free(topology->nodes);
        topology->nodes = NULL;
    }

    free(topology);

    return NETLOC_SUCCESS;
}

int netloc_refresh(struct netloc_topology *topology)
{
    return NETLOC_ERROR_NOT_IMPL;
}

int netloc_topology_copy(struct netloc_topology *src, struct netloc_topology **dup)
{
   struct netloc_topology *new = (struct netloc_topology *)
       malloc(sizeof(struct netloc_topology));

   new->network = src->network;
   new->network->refcount++;

   new->nodes_loaded = src->nodes_loaded;

   /* Copy nodes */
   new->num_nodes = src->num_nodes;
   new->nodes = (netloc_node_t **)
       malloc(new->num_nodes*sizeof(netloc_node_t *));
   memcpy(new->nodes, src->nodes, new->num_nodes*sizeof(netloc_node_t *));
   for (int n = 0; n < new->num_nodes; n++) {
       new->nodes[n]->refcount++;
   }

   /* Copy edges */
   {
       new->edges = calloc(1, sizeof(struct netloc_dt_lookup_table));
       netloc_dt_lookup_table_t_copy(src->edges, new->edges);

       netloc_dt_lookup_table_t edges = new->edges;
       netloc_dt_lookup_table_iterator_t edges_iter =
           netloc_dt_lookup_table_iterator_t_construct(edges);
       netloc_edge_t *edge;
       while ((edge =
                   (netloc_edge_t *)netloc_lookup_table_iterator_next_entry(edges_iter))) {
           edge->refcount++;
       }
   }

   /* Copy partitions */
   new->num_partitions = src->num_partitions;
   new->partitions = (char **)
       malloc(new->num_partitions*sizeof(char *));
   memcpy(new->partitions, src->nodes, new->num_partitions*sizeof(char *));
   for (int p = 0; p < new->num_partitions; p++) {
       asprintf(&new->partitions[p], "%s", src->partitions[p]);
   }

   /* Copy topologies */
   new->num_topos = src->num_topos;
   new->topos = (char **)
       malloc(new->num_topos*sizeof(char *));
   memcpy(new->topos, src->nodes, new->num_topos*sizeof(char *));
   for (int p = 0; p < new->num_topos; p++) {
       asprintf(&new->topos[p], "%s", src->topos[p]);
   }

   new->type = src->type;

   *dup = new;

    return 0;
}
