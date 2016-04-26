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
    static UT_icd partitions_icd = {sizeof(char *), NULL, NULL, NULL };
    static UT_icd topos_icd = {sizeof(char *), NULL, NULL, NULL };

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
    topology->nodes          = NULL;
    topology->physical_links = NULL;
    utarray_new(topology->partitions, &partitions_icd );
    utarray_new(topology->topos, &topos_icd );
    topology->type           = NETLOC_TOPOLOGY_TYPE_INVALID ;

    /*
     * Make the pointer live
     */
    (*topology_ptr) = topology;

    return NETLOC_SUCCESS;
}

int netloc_detach(struct netloc_topology * topology)
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
    netloc_dt_network_t_destruct(topology->network);

    // TODO

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

   // TODO
    return 0;
}
