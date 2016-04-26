/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
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
#include <private/netloc.h>

#include "support.h"

/**
 * Export a netloc_edge_t in GraphML format
 */
static int netloc_topology_export_graphml_edge(const netloc_edge_t *edge, FILE *fh);

/**
 * Export a netloc_edge_t in GEXF format
 */
static int netloc_topology_export_gexf_edge(const netloc_edge_t *edge, FILE *fh);


/*****************************************************/

int netloc_topology_export_graphml(struct netloc_topology * topology, const char* filename) {
    int ret, exit_status = NETLOC_SUCCESS;

    struct netloc_dt_lookup_table *nodes = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *node = NULL;

    int i;
    int num_edges;
    netloc_edge_t **edges = NULL;

    FILE *fh = NULL;

    /*
     * Open the file
     */
    fh = fopen(filename, "w");
    if( NULL == fh ) {
        fprintf(stderr, "Error: Failed to open the file <%s> for writing of network %s\n",
                filename,
                netloc_pretty_print_network_t(topology->network) );
        return NETLOC_ERROR;
    }

    /*
     * Add the header information
     */
    fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fh, "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\"\n");
    fprintf(fh, "         xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
    fprintf(fh, "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(fh, "         xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns ");
    fprintf(fh, "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">\n");


    /*
     * Add some custom data keys
     */
    fprintf(fh, "\t<key id=\"network_type\" for=\"graph\" attr.name=\"network_type\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"subnet_id\"    for=\"graph\" attr.name=\"subnet_id\"    attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"description\"  for=\"graph\" attr.name=\"description\"  attr.type=\"string\"/>\n");

    fprintf(fh, "\t<key id=\"type\"         for=\"node\" attr.name=\"type\"         attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"network_type\" for=\"node\" attr.name=\"network_type\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"subnet_id\"    for=\"node\" attr.name=\"subnet_id\"    attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"logical_id\"   for=\"node\" attr.name=\"logical_id\"   attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"physical_id\"  for=\"node\" attr.name=\"physical_id\"  attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"description\"  for=\"node\" attr.name=\"description\"  attr.type=\"string\"/>\n");

    fprintf(fh, "\t<key id=\"source_type\" for=\"edge\" attr.name=\"source_type\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"target_type\" for=\"edge\" attr.name=\"target_type\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"source_port\" for=\"edge\" attr.name=\"source_port\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"target_port\" for=\"edge\" attr.name=\"target_port\" attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"speed\"       for=\"edge\" attr.name=\"speed\"       attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"width\"       for=\"edge\" attr.name=\"width\"       attr.type=\"string\"/>\n");
    fprintf(fh, "\t<key id=\"gbits\"       for=\"edge\" attr.name=\"gbits\"       attr.type=\"double\"/>\n");
    fprintf(fh, "\t<key id=\"description\" for=\"edge\" attr.name=\"description\" attr.type=\"string\"/>\n");


    /*
     * Basic Graph information
     */
    fprintf(fh, "\t<graph id=\"G\" edgedefault=\"undirected\">\n");
    fprintf(fh, "\t\t<data key=\"network_type\">%s</data>\n", netloc_decode_network_type_readable(topology->network->network_type));
    fprintf(fh, "\t\t<data key=\"subnet_id\">%s</data>\n", topology->network->subnet_id);
    fprintf(fh, "\t\t<data key=\"description\">%s</data>\n", topology->network->description);


    /*
     * Get hosts and switches
     */
    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Host information
     */
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == node ) {
            break;
        }

        // Add a node for this host
        fprintf(fh, "\t\t<node id=\"%s\">\n", node->physical_id);
        fprintf(fh, "\t\t\t<data key=\"type\">%s</data>\n", netloc_decode_node_type_readable(node->node_type) );
        fprintf(fh, "\t\t\t<data key=\"network_type\">%s</data>\n", netloc_decode_network_type_readable(node->network_type) );
        fprintf(fh, "\t\t\t<data key=\"subnet_id\">%s</data>\n", node->subnet_id);
        fprintf(fh, "\t\t\t<data key=\"logical_id\">%s</data>\n", node->logical_id);
        fprintf(fh, "\t\t\t<data key=\"physical_id\">%s</data>\n", node->physical_id);
        fprintf(fh, "\t\t\t<data key=\"description\">%s</data>\n", node->description);
        fprintf(fh, "\t\t</node>\n");

        ret = netloc_get_all_edges(topology, node, &num_edges, &edges);
        if( NETLOC_SUCCESS != ret ) {
            exit_status = ret;
            goto cleanup;
        }

        // Add each edge from this host
        for(i = 0; i < num_edges; ++i ) {
            netloc_topology_export_graphml_edge(edges[i], fh);
        }
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    hti = NULL;
    netloc_lookup_table_destroy(nodes);
    free(nodes);
    nodes = NULL;

    fprintf(fh, "\t</graph>\n");
    fprintf(fh, "</graphml>\n");

 cleanup:
    if( NULL != hti ) {
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;
    }

    if( NULL != nodes ) {
        netloc_lookup_table_destroy(nodes);
        free(nodes);
        nodes = NULL;
    }

    if( NULL != fh ) {
        fclose(fh);
        fh = NULL;
    }

    return exit_status;
}

int netloc_topology_export_gexf(struct netloc_topology * topology, const char * filename) {
    int ret, exit_status = NETLOC_SUCCESS;

    struct netloc_dt_lookup_table *nodes = NULL;
    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_node_t *node = NULL;

    int i;
    int num_edges;
    netloc_edge_t **edges = NULL;

    FILE *fh = NULL;

    /*
     * Open the file
     */
    fh = fopen(filename, "w");
    if( NULL == fh ) {
        fprintf(stderr, "Error: Failed to open the file <%s> for writing of network %s\n",
                filename,
                netloc_pretty_print_network_t(topology->network) );
        return NETLOC_ERROR;
    }

    /*
     * Add the header information
     */
    fprintf(fh, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fh, "<gexf    xmlns=\"http://www.gexf.net/1.2draft\"\n");
    fprintf(fh, "         xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(fh, "         xsi:schemaLocation=\"http://www.gexf.net/1.2draft http://www.gexf.net/1.2draft/gexf.xsd\"\n");
    fprintf(fh, "         version=\"1.2\">\n");


    /*
     * Basic Graph information
     */
    fprintf(fh, "\t<graph defaultedgetype=\"undirected\">\n");
    fprintf(fh, "\t\t<meta>\n");
    fprintf(fh, "\t\t\t<description>%s %s %s</description>\n",
            netloc_decode_network_type_readable(topology->network->network_type),
            topology->network->subnet_id,
            topology->network->description);
    fprintf(fh, "\t\t</meta>\n");

    fprintf(fh, "\t\t<attributes class=\"node\">\n");
    fprintf(fh, "\t\t\t<attribute id=\"type\"         title=\"type\"         type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"network_type\" title=\"network_type\" type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"subnet_id\"    title=\"subnet_id\"    type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"logical_id\"   title=\"logical_id\"   type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"physical_id\"  title=\"physical_id\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"description\"  title=\"description\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t</attributes>\n");

    fprintf(fh, "\t\t<attributes class=\"edge\">\n");
    fprintf(fh, "\t\t\t<attribute id=\"source_type\"  title=\"source_type\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"target_type\"  title=\"target_type\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"source_port\"  title=\"source_port\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"target_port\"  title=\"target_port\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"speed\"        title=\"speed\"        type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"width\"        title=\"width\"        type=\"string\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"gbits\"        title=\"gbits\"        type=\"float\"/>\n");
    fprintf(fh, "\t\t\t<attribute id=\"description\"  title=\"description\"  type=\"string\"/>\n");
    fprintf(fh, "\t\t</attributes>\n");


    /*
     * Get hosts and switches
     */
    ret = netloc_get_all_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Host information
     */
    fprintf(fh, "\t\t<nodes>\n");
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == node ) {
            break;
        }

        fprintf(fh, "\t\t\t<node id=\"%s\">\n", node->physical_id);
        fprintf(fh, "\t\t\t\t<attvalues>\n");
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"type\" value=\"%s\"/>\n", netloc_decode_node_type_readable(node->node_type));
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"network_type\" value=\"%s\"/>\n", netloc_decode_network_type_readable(node->network_type) );
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"subnet_id\" value=\"%s\"/>\n", node->subnet_id );
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"logical_id\" value=\"%s\"/>\n", node->logical_id );
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"physical_id\" value=\"%s\"/>\n", node->physical_id );
        fprintf(fh, "\t\t\t\t\t<attvalue for=\"description\" value=\"%s\"/>\n", node->description );
        fprintf(fh, "\t\t\t\t</attvalues>\n");
        fprintf(fh, "\t\t\t</node>\n");
    }
    netloc_dt_lookup_table_iterator_t_destruct(hti);

    fprintf(fh, "\t\t</nodes>\n");

    /*
     * All edges next
     */
    fprintf(fh, "\t\t<edges>\n");
    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == node ) {
            break;
        }

        ret = netloc_get_all_edges(topology, node, &num_edges, &edges);
        if( NETLOC_SUCCESS != ret ) {
            exit_status = ret;
            goto cleanup;
        }

        // Add each edge from this host
        for(i = 0; i < num_edges; ++i ) {
            netloc_topology_export_gexf_edge(edges[i], fh);
        }
    }

    /* Cleanup */
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    hti = NULL;

    fprintf(fh, "\t\t</edges>\n");

    fprintf(fh, "\t</graph>\n");
    fprintf(fh, "</gexf>\n");

 cleanup:
    if( NULL != hti ) {
        netloc_dt_lookup_table_iterator_t_destruct(hti);
        hti = NULL;
    }

    if( NULL != fh ) {
        fclose(fh);
        fh = NULL;
    }

    return exit_status;
}


/*******************************************************************
 * Support Functionality
 *******************************************************************/
static int netloc_topology_export_graphml_edge(const netloc_edge_t *edge, FILE *fh) {
    const char * src_type = NULL;
    const char * target_type = NULL;

    src_type    = netloc_decode_node_type_readable( edge->src_node_type );
    target_type = netloc_decode_node_type_readable( edge->dest_node_type );

    fprintf(fh, "\t\t<edge source=\"%s\" target=\"%s\">\n",
            edge->src_node_id,
            edge->dest_node_id);

    fprintf(fh, "\t\t\t<data key=\"source_type\">%s</data>\n", src_type);
    fprintf(fh, "\t\t\t<data key=\"target_type\">%s</data>\n", target_type);

    fprintf(fh, "\t\t\t<data key=\"source_port\">%s</data>\n", edge->src_port_id);
    fprintf(fh, "\t\t\t<data key=\"target_port\">%s</data>\n", edge->dest_port_id);

    fprintf(fh, "\t\t\t<data key=\"speed\">%s</data>\n", edge->speed);
    fprintf(fh, "\t\t\t<data key=\"width\">%s</data>\n", edge->width);
    fprintf(fh, "\t\t\t<data key=\"gbits\">%f</data>\n", edge->gbits);

    fprintf(fh, "\t\t\t<data key=\"description\">%s</data>\n", edge->description);

    fprintf(fh, "\t\t</edge>\n");

    return NETLOC_SUCCESS;
}

static int netloc_topology_export_gexf_edge(const netloc_edge_t *edge, FILE *fh) {
    static int counter = 0;
    const char * src_type = NULL;
    const char * target_type = NULL;

    src_type    = netloc_decode_node_type_readable( edge->src_node_type );
    target_type = netloc_decode_node_type_readable( edge->dest_node_type );

    fprintf(fh, "\t\t\t<edge id=\"%d\" source=\"%s\" target=\"%s\">\n",
            counter,
            edge->src_node_id,
            edge->dest_node_id );

    fprintf(fh, "\t\t\t\t<attvalues>\n");
    fprintf(fh, "\t\t\t\t\t<attvalue for=\"source_type\" value=\"%s\"/>\n", src_type);
    fprintf(fh, "\t\t\t\t\t<attvalue for=\"target_type\" value=\"%s\"/>\n", target_type);

    fprintf(fh, "\t\t\t\t\t<attvalue for=\"source_port\" value=\"%s\"/>\n", edge->src_port_id );
    fprintf(fh, "\t\t\t\t\t<attvalue for=\"target_port\" value=\"%s\"/>\n", edge->dest_port_id );

    fprintf(fh, "\t\t\t\t\t<attvalue for=\"speed\"       value=\"%s\"/>\n", edge->speed );
    fprintf(fh, "\t\t\t\t\t<attvalue for=\"width\"       value=\"%s\"/>\n", edge->width );
    fprintf(fh, "\t\t\t\t\t<attvalue for=\"gbits\"       value=\"%f\"/>\n", edge->gbits );

    fprintf(fh, "\t\t\t\t\t<attvalue for=\"description\" value=\"%s\"/>\n", edge->description );
    fprintf(fh, "\t\t\t\t</attvalues>\n");

    fprintf(fh, "\t\t\t</edge>\n");

    return NETLOC_SUCCESS;
}
