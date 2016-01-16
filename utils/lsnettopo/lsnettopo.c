/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
 *
 * Copyright © 2016 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "netloc.h"


const char * ARG_EXPORT         = "--export";
const char * ARG_SHORT_EXPORT   = "-e";
const char * ARG_FULL           = "--full";
const char * ARG_SHORT_FULL     = "-f";
const char * ARG_VERBOSE        = "--verbose";
const char * ARG_SHORT_VERBOSE  = "-v";
const char * ARG_HELP           = "--help";
const char * ARG_SHORT_HELP     = "-h";

/*
 * Parse command line arguments
 */
static int parse_args(int argc, char ** argv);

/*
 * Display the topology (Export = Screen)
 */
static int display_topo_screen(netloc_topology_t topology, netloc_network_t *network);

/*
 * Display a single edge connection in the topology (Export = Screen)
 */
static int display_netloc_edge_screen(const netloc_edge_t* edge);

/*
 * Output directory
 */
static char * indir = NULL;

/*
 * Verbose output
 */
static bool verbose = false;

/*
 * Detailed output
 */
static bool full_output = false;

/*
 * Valid Export types for graph data
 */
static int num_valid_export_types = 3;
const char * valid_export_types[3] = {"screen", "graphml", "gexf"};

/*
 * Selected export type
 */
static char * export_type = NULL;

int main(int argc, char ** argv) {
    int ret, i;
    netloc_topology_t topology;
    char ** search_uris = NULL;
    int num_search_uris = 1;
    int num_all_networks = 0;
    netloc_network_t **all_networks = NULL;
    char * filename = NULL;

    /*
     * Parse Args
     */
    if( 0 != (ret = parse_args(argc, argv)) ) {
        printf("Usage: %s [<input directory>] [%s|%s] [%s|%s <export type>] [%s|%s] [%s|%s]\n",
               argv[0],
               ARG_FULL, ARG_SHORT_FULL,
               ARG_EXPORT, ARG_SHORT_EXPORT,
               ARG_VERBOSE, ARG_SHORT_VERBOSE,
               ARG_HELP, ARG_SHORT_HELP);
        printf("       Default <input directory> = current working directory\n");
        printf("       Valid Options for %s:\n", ARG_EXPORT );
        for(i = 0; i < num_valid_export_types; ++i ) {
            printf("\t\t%s\n", valid_export_types[i]);
        }
        return NETLOC_ERROR;
    }


    /*
     * Find all networks
     */
    search_uris = (char **)malloc(sizeof(char*) * num_search_uris);
    asprintf(&search_uris[0], "file://%s", indir);
    ret = netloc_foreach_network((const char * const *)search_uris,
                                 1,
                                 NULL, NULL,
                                 &num_all_networks,
                                 &all_networks);

    if( verbose ) {
        printf("  Found %d Networks.\n", num_all_networks);
        printf("---------------------------------------------------\n");
    }

    for(i = 0; i < num_all_networks; ++i) {
        /*
         * Attach to the topology
         */
        ret = netloc_attach(&topology, *all_networks[i]);
        if( NETLOC_SUCCESS != ret ) {
            return ret;
        }

        /*
         * Display the topology
         */
        if( 0 == strncmp("screen", export_type, strlen("screen") )) {
            ret = display_topo_screen(topology, all_networks[i] );
        }
        else if( 0 == strncmp("graphml", export_type, strlen("graphml")) ) {
            asprintf(&filename, "%s-%s.graphml",
                     netloc_decode_network_type(all_networks[i]->network_type),
                     all_networks[i]->subnet_id);

            printf("Network: %s\n", netloc_pretty_print_network_t(all_networks[i]) );
            printf("\tFilename: %s\n", filename);

            ret = netloc_topology_export_graphml(topology, filename);
        }
        else if( 0 == strncmp("gexf", export_type, strlen("gexf")) ) {
            asprintf(&filename, "%s-%s.gexf",
                     netloc_decode_network_type(all_networks[i]->network_type),
                     all_networks[i]->subnet_id);

            printf("Network: %s\n", netloc_pretty_print_network_t(all_networks[i]) );
            printf("\tFilename: %s\n", filename);

            ret = netloc_topology_export_gexf(topology, filename);
        }

        if( NETLOC_SUCCESS != ret ) {
            return ret;
        }

        /*
         * Detach from the topology
         */
        ret = netloc_detach(topology);
        if( NETLOC_SUCCESS != ret ) {
            return ret;
        }
    }

    free(filename);

    if( NULL != search_uris ) {
        for(i = 0; i < num_search_uris; ++i ) {
            free(search_uris[i]);
        }
        free(search_uris);
    }

    return NETLOC_SUCCESS;
}

static int parse_args(int argc, char ** argv) {
    int num_indir = 0;
    int i;
    bool found = false;

    for(i = 1; i < argc; ++i ) {
        /*
         * Export type
         */
        if( 0 == strncmp(ARG_EXPORT,       argv[i], strlen(ARG_EXPORT))  ||
            0 == strncmp(ARG_SHORT_EXPORT, argv[i], strlen(ARG_SHORT_EXPORT)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_EXPORT );
                return NETLOC_ERROR;
            }
            export_type = strdup(argv[i]);
        }
        /*
         * Full output
         */
        else if( 0 == strncmp(ARG_FULL,       argv[i], strlen(ARG_FULL))  ||
                 0 == strncmp(ARG_SHORT_FULL, argv[i], strlen(ARG_SHORT_FULL)) ) {
            full_output = true;
        }
        /*
         * Verbose
         */
        else if( 0 == strncmp(ARG_VERBOSE,       argv[i], strlen(ARG_VERBOSE))  ||
                 0 == strncmp(ARG_SHORT_VERBOSE, argv[i], strlen(ARG_SHORT_VERBOSE)) ) {
            verbose = true;
        }
        /*
         * Help
         */
        else if( 0 == strncmp(ARG_HELP,       argv[i], strlen(ARG_HELP))  ||
                 0 == strncmp(ARG_SHORT_HELP, argv[i], strlen(ARG_SHORT_HELP)) ) {
            return NETLOC_ERROR;
        }
        /*
         * Input directory
         */
        else {
            if( num_indir > 0 ) {
                fprintf(stderr, "Error: More than one input directory specified\n");
                fprintf(stderr, "  Last directory value: %s\n", indir);
                fprintf(stderr, "  Current value       : %s\n", argv[i]);
                return NETLOC_ERROR;
            }
            indir = strdup(argv[i]);
            ++num_indir;
        }
    }


    /*
     * Check Output Directory Parameter
     */
    if( NULL == indir || strlen(indir) <= 0 ) {
        // Default: current working directory
        indir = strdup(".");
    }
    if( '/' != indir[strlen(indir)-1] ) {
        indir = (char *)realloc(indir, sizeof(char) * (strlen(indir)+1));
        indir[strlen(indir)+1] = '\0';
        indir[strlen(indir)]   = '/';
    }

    /*
     * Check export parameter
     */
    if( NULL == export_type || strlen(export_type) <= 0 ) {
        export_type = strdup("screen");
    }
    else {
        found = false;
        for( i = 0; i < num_valid_export_types; ++i ) {
            if( 0 == strncmp(valid_export_types[i], export_type, strlen(valid_export_types[i])) ) {
                found = true;
                break;
            }
        }

        if( !found ) {
            fprintf(stderr, "Error: Invalid export type specified: %s\n", export_type);
            return NETLOC_ERROR;
        }
    }

    /*
     * Display Parsed Arguments
     */
    if( verbose ) {
        printf("---------------------------------------------------\n");
        printf("  Input Directory  : %s\n", indir);
        printf("  Export type      : %s\n", export_type);
        printf("---------------------------------------------------\n");
    }

    return NETLOC_SUCCESS;
}

static int display_topo_screen(netloc_topology_t topology, netloc_network_t *network) {
    int ret, exit_status = NETLOC_SUCCESS;
    int i;
    netloc_dt_lookup_table_t hosts_nodes = NULL;
    netloc_dt_lookup_table_t switches_nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;
    int num_edges;
    netloc_edge_t **edges = NULL;

    printf("Network: %s\n", netloc_pretty_print_network_t(network) );
    printf("  Type    : %s\n", netloc_decode_network_type_readable(network->network_type) );
    printf("  Subnet  : %s\n", network->subnet_id);

    /*
     * Get hosts and switches
     */
    ret = netloc_get_all_host_nodes(topology, &hosts_nodes);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("  Hosts   :   %d\n", netloc_lookup_table_size(hosts_nodes));;

	ret = netloc_get_all_switch_nodes(topology, &switches_nodes);
    if( NETLOC_SUCCESS != ret ) {
        exit_status = ret;
        goto cleanup;
    }
    printf("  Switches:   %d\n", netloc_lookup_table_size(switches_nodes) );
    printf("---------------------------------------------------\n");

    if( full_output ) {
        /*
         * Print out a list of hosts and their connections
         */
        printf("\n");
        printf("Information by Host\n");
        printf("---------------------\n");

        hti = netloc_dt_lookup_table_iterator_t_construct( hosts_nodes );
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            key = netloc_lookup_table_iterator_next_key(hti);
            if( NULL == key ) {
                break;
            }

            node = (netloc_node_t*)netloc_lookup_table_access(hosts_nodes, key);

            ret = netloc_get_all_edges(topology, node, &num_edges, &edges);
            if( NETLOC_SUCCESS != ret ) {
                exit_status = ret;
                goto cleanup;
            }

            for(i = 0; i < num_edges; ++i ) {
                display_netloc_edge_screen(edges[i]);
            }
        }


        /*
         * Print out a list of switches and their connections
         */
        printf("\n");
        printf("Information by Switch\n");
        printf("---------------------\n");

        hti = netloc_dt_lookup_table_iterator_t_construct( switches_nodes );
        while( !netloc_lookup_table_iterator_at_end(hti) ) {
            key = netloc_lookup_table_iterator_next_key(hti);
            if( NULL == key ) {
                break;
            }

            node = (netloc_node_t*)netloc_lookup_table_access(switches_nodes, key);

            ret = netloc_get_all_edges(topology, node, &num_edges, &edges);
            if( NETLOC_SUCCESS != ret ) {
                exit_status = ret;
                goto cleanup;
            }

            for(i = 0; i < num_edges; ++i ) {
                display_netloc_edge_screen(edges[i]);
            }
        }

        printf("------------------------------------------------------------------------------\n");
    }

    printf("\n");

 cleanup:

    return exit_status;
}

static int display_netloc_edge_screen(const netloc_edge_t *edge) {

    printf("%s (%6s) on port %3s",
           edge->src_node_id,
           netloc_decode_node_type_readable(edge->src_node_type),
           edge->src_port_id);

    printf("  [-> %s/%s <-]  ",
           edge->speed,
           edge->width);

    printf("%s (%6s) on port %3s",
           edge->dest_node_id,
           netloc_decode_node_type_readable(edge->dest_node_type),
           edge->dest_port_id);

    printf("\n");
    //printf("\t%s\n", edge->get_description());

    return NETLOC_SUCCESS;
}
