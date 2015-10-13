/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2015-2016 Inria.  All rights reserved.
 *
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgen.h> // for dirname

#include <sys/time.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <jansson.h>

#include "netloc/dc.h"
#include "private/netloc.h"

#include "perl_json_support.h"

const char * ARG_OUTDIR         = "--outdir";
const char * ARG_SHORT_OUTDIR   = "-o";
const char * ARG_SUBNET         = "--subnet";
const char * ARG_SHORT_SUBNET   = "-s";
const char * ARG_FILE           = "--file";
const char * ARG_SHORT_FILE     = "-f";
const char * ARG_ROUTEDIR       = "--routedir";
const char * ARG_SHORT_ROUTEDIR = "-r";
const char * ARG_PROGRESS       = "--progress";
const char * ARG_SHORT_PROGRESS = "-p";
const char * ARG_HELP           = "--help";
const char * ARG_SHORT_HELP     = "-h";

/*
 * Parse command line arguments
 */
static int parse_args(int argc, char ** argv);

/*
 * Run the parser for general data
 */
static int run_parser();

/*
 * Convert the temporary node file to the proper netloc format
 */
static int convert_nodes_file(netloc_data_collection_handle_t *dc_handle);
static int load_json_from_file(char * fname, json_t **json);
static int extract_network_info_from_json_file(netloc_network_t *network, char *dat_file);
static int process_nodes_dat(netloc_data_collection_handle_t *dc_handle, char *dat_file);

/*
 * Find physical paths
 */
static int compute_physical_paths(netloc_data_collection_handle_t *dc_handle);

/*
 * Run the parser for routing data
 */
static int run_routes_parser();
static int process_logical_paths(netloc_data_collection_handle_t *dc_handle);
static int process_logical_paths_between_nodes(netloc_data_collection_handle_t *handle,
                                               netloc_dt_lookup_table_t all_routes,
                                               netloc_node_t *src_node,
                                               netloc_node_t *dest_node,
                                               int *num_edges,
                                               netloc_edge_t ***edges);

/*
 * Check the resulting .dat files
 */
static int check_dat_files();


/*
 * Output directory
 */
static char * outdir = NULL;
static char * out_file_nodes = NULL;
static char * out_file_log_prep = NULL;

/*
 * Subnet
 */
static char * subnet = NULL;

/*
 * ibnetdiscover files
 */
static char * file_ibnetdiscover = NULL;

/*
 * ibroutes directory of files
 */
static char * dir_ibroutes = NULL;

/*
 * Show progress during pathfinding
 */
static int progress = 0;

int main(int argc, char ** argv) {
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_network_t *network = NULL;
    netloc_data_collection_handle_t *dc_handle = NULL;

    /*
     * Parse Args
     */
    if( 0 != parse_args(argc, argv) ) {
        printf("Usage: %s %s|%s <input file> [%s|%s <path to routing files>] [%s|%s <subnet id>] [%s|%s <output directory>] [%s|%s] [--help|-h]\n",
               argv[0],
               ARG_FILE, ARG_SHORT_FILE,
               ARG_ROUTEDIR, ARG_SHORT_ROUTEDIR,
               ARG_SUBNET, ARG_SHORT_SUBNET,
               ARG_OUTDIR, ARG_SHORT_OUTDIR,
               ARG_PROGRESS, ARG_SHORT_PROGRESS);
        printf("       Default %-10s = none\n", ARG_ROUTEDIR);
        printf("       Default %-10s = \"unknown\"\n", ARG_SUBNET);
        printf("       Default %-10s = current working directory\n", ARG_OUTDIR);
        return NETLOC_ERROR;
    }


    /*
     * Run the parser for the nodes/edges
     */
    if( 0 != (ret = run_parser() ) ) {
        return ret;
    }

    /*
     * Setup network information
     */
    network = netloc_dt_network_t_construct();
    ret = extract_network_info_from_json_file(network, out_file_nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to extract network information from the file: %s\n", out_file_nodes);
        return ret;
    }

    dc_handle = netloc_dc_create(network, outdir);
    if( NULL == dc_handle ) {
        fprintf(stderr, "Error: Failed to create a new data file\n");
        return ret;
    }
    netloc_dt_network_t_destruct(network);
    network = NULL;

    /*
     * Convert the temporary node file to the proper format
     */
    if( 0 != (ret = convert_nodes_file(dc_handle)) ) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Find all physical paths
     */
    if( 0 != (ret = compute_physical_paths(dc_handle)) ) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Run the parser for the logical paths
     */
    if( NULL != dir_ibroutes && strlen(dir_ibroutes) > 0 ) {
        if( 0 != (ret = run_routes_parser() ) ) {
            exit_status = ret;
            goto cleanup;
        }

        /*
         * Convert the temporary log_prep file to the proper format
         */
        if( 0 != (ret = process_logical_paths(dc_handle)) ) {
            exit_status = ret;
            goto cleanup;
        }
    }

 cleanup:
    /*
     * Close the handle
     */
    ret = netloc_dc_close(dc_handle);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to close the data connection!\n");
        return ret;
    }

    netloc_dt_data_collection_handle_t_destruct(dc_handle);
    dc_handle = NULL;

    /*
     * Validate the resulting .dat files
     */
    if( 0 == exit_status ) {
        if( 0 != (ret = check_dat_files() ) ) {
            return ret;
        }
    }

    free(outdir);
    free(out_file_nodes);
    free(out_file_log_prep);
    free(subnet);
    free(file_ibnetdiscover);
    free(dir_ibroutes);
    return exit_status;
}

static int parse_args(int argc, char ** argv) {
    int ret = NETLOC_SUCCESS;
    int i;

    for(i = 1; i < argc; ++i ) {
        /*
         * --outdir
         */
        if( 0 == strncmp(ARG_OUTDIR,       argv[i], strlen(ARG_OUTDIR)) ||
            0 == strncmp(ARG_SHORT_OUTDIR, argv[i], strlen(ARG_SHORT_OUTDIR)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_OUTDIR );
                return NETLOC_ERROR;
            }
            outdir = strdup(argv[i]);
        }
        /*
         * --subnet
         */
        else if( 0 == strncmp(ARG_SUBNET,       argv[i], strlen(ARG_SUBNET)) ||
                 0 == strncmp(ARG_SHORT_SUBNET, argv[i], strlen(ARG_SHORT_SUBNET)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_SUBNET );
                return NETLOC_ERROR;
            }
            subnet = strdup(argv[i]);
        }
        /*
         * --file (output from ibnetdiscover)
         */
        else if( 0 == strncmp(ARG_FILE,       argv[i], strlen(ARG_FILE)) ||
                 0 == strncmp(ARG_SHORT_FILE, argv[i], strlen(ARG_SHORT_FILE)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s (ibnetdiscover data)\n", ARG_FILE );
                return NETLOC_ERROR;
            }
            file_ibnetdiscover = strdup(argv[i]);
        }
        /*
         * --routedir (output from ibroutes)
         */
        else if( 0 == strncmp(ARG_ROUTEDIR,       argv[i], strlen(ARG_ROUTEDIR)) ||
                 0 == strncmp(ARG_SHORT_ROUTEDIR, argv[i], strlen(ARG_SHORT_ROUTEDIR)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s (ibroutes data)\n", ARG_ROUTEDIR );
                return NETLOC_ERROR;
            }
            dir_ibroutes = strdup(argv[i]);
        }
        /*
         * Progress during pathfinder
         */
        else if( 0 == strncmp(ARG_PROGRESS,       argv[i], strlen(ARG_PROGRESS)) ||
                 0 == strncmp(ARG_SHORT_PROGRESS, argv[i], strlen(ARG_SHORT_PROGRESS)) ) {
            progress = 1;
        }
        /*
         * Help
         */
        else if( 0 == strncmp(ARG_HELP,       argv[i], strlen(ARG_HELP)) ||
                 0 == strncmp(ARG_SHORT_HELP, argv[i], strlen(ARG_SHORT_HELP)) ) {
            return NETLOC_ERROR;
        }
        /*
         * Unknown options throw warnings
         */
        else {
            fprintf(stderr, "Warning: Unknown argument of <%s>\n", argv[i]);
            return NETLOC_ERROR;
        }
    }


    /*
     * Check Output Directory Parameter
     */
    if( NULL == outdir || strlen(outdir) <= 0 ) {
        free(outdir);
        // Default: current working directory
        outdir = strdup(".");
    }
    if( '/' != outdir[strlen(outdir)-1] ) {
        outdir = (char *)realloc(outdir, sizeof(char) * (strlen(outdir)+1));
        outdir[strlen(outdir)+1] = '\0';
        outdir[strlen(outdir)]   = '/';
    }

    /*
     * Check Subnet Parameter
     */
    if( NULL == subnet || strlen(subnet) <= 0 ) {
        free(subnet);
        //fprintf(stderr, "Warning: Subnet was not specified. Using default value.\n");
        // Default: 'unknown'
        subnet = "unknown";
    }

    /*
     * Check the ibnetdiscover data path
     */
    if( NULL == file_ibnetdiscover || strlen(file_ibnetdiscover) <= 0 ) {
        fprintf(stderr, "Error: Must supply an argument to %s (ibnetdiscover data)\n", ARG_FILE );
        return NETLOC_ERROR;
    }

    asprintf(&out_file_nodes,    "%sIB-%s-nodes.dat",    outdir, subnet);
    asprintf(&out_file_log_prep, "%sIB-%s-log_prep.dat", outdir, subnet);

    /*
     * Display Parsed Arguments
     */
    printf("  Output Directory   : %s\n", outdir);
    printf("  Subnet             : %s\n", subnet);
    printf("  ibnetdiscover File : %s\n", file_ibnetdiscover);
    printf("  ibroutes Directory : %s\n", (NULL == dir_ibroutes || strlen(dir_ibroutes) <= 0 ? "None Specified" : dir_ibroutes) );

    return ret;
}

static int run_parser() {
    char * command = NULL;
    int ret;

    printf("Status: Querying the ibnetdiscover data for subnet %s...\n", subnet);

    asprintf(&command, "netloc_reader_ib_backend_general -i %s -s %s -o %s",
             file_ibnetdiscover, subnet, out_file_nodes);

    ret = system(command);
    if( ret != 0 ) {
        fprintf(stderr, "Error: Failed to process the ibnetdiscover data at %s!\n",
                file_ibnetdiscover);
        fprintf(stderr, "Error: See error message above for more details\n");
        fprintf(stderr, "Error: Command: %s\n", command);
        free(command);
        return ret;
    }

    free(command);
    return NETLOC_SUCCESS;
}

static int run_routes_parser() {
    char * command = NULL;
    int ret;

    printf("Status: Querying the ibroutes data for subnet %s...\n", subnet);

    asprintf(&command, "netloc_reader_ib_backend_log_prep -d %s -o %s",
             dir_ibroutes, out_file_log_prep);

    ret = system(command);
    if( ret != 0 ) {
        fprintf(stderr, "Error: Failed to process the ibroutes data at %s! See error message above for more details\n",
                dir_ibroutes);
        fprintf(stderr, "Error: Command: %s\n", command);
        free(command);
        return ret;
    }

    free(command);
    return NETLOC_SUCCESS;
}


static int convert_nodes_file(netloc_data_collection_handle_t *dc_handle)
{
    int ret;

    printf("Status: Processing Node Information\n");

    /*
     * Process the Nodes temp file
     */
    ret = process_nodes_dat(dc_handle, out_file_nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed while processing the Nodes file: %s\n", out_file_nodes);
        return ret;
    }

    /*
     * Remove the temporary nodes file
     */
    ret = remove(out_file_nodes);
    if( 0 != ret ) {
        fprintf(stderr, "Error: Failed to remove the temporary file %s\n", out_file_nodes);
        return ret;
    }

    return 0;
}

static int compute_physical_paths(netloc_data_collection_handle_t *dc_handle)
{
    int ret;
    int src_idx, dst_idx;

    int num_edges = 0;
    netloc_edge_t **edges = NULL;

    netloc_dt_lookup_table_iterator_t hti_src = NULL;
    netloc_dt_lookup_table_iterator_t hti_dst = NULL;
    netloc_node_t *cur_src_node = NULL;
    netloc_node_t *cur_dst_node = NULL;

    int total_nodes;
    double last_perc = 0;

    printf("Status: Computing Physical Paths\n");

    /*
     * Calculate the path from all sources to all destinations
     */

    total_nodes = netloc_lookup_table_size(dc_handle->node_list);
    src_idx = 0;
    hti_src = netloc_dt_lookup_table_iterator_t_construct(dc_handle->node_list);
    hti_dst = netloc_dt_lookup_table_iterator_t_construct(dc_handle->node_list);

    netloc_lookup_table_iterator_reset(hti_src);
    while( !netloc_lookup_table_iterator_at_end(hti_src) ) {
        cur_src_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti_src);
        if( NULL == cur_src_node ) {
            break;
        }

        // JJH: For now limit to just the "host" nodes
        if( NETLOC_NODE_TYPE_HOST != cur_src_node->node_type ) {
            ++src_idx;
            continue;
        }

        if( progress > 0 ) {
            if( last_perc + 0.05 < src_idx / (double)total_nodes ) {
                last_perc = src_idx / (double)total_nodes;
                printf("\tProgress: %6.2f%% -- %4d of %4d\n", last_perc * 100, src_idx, total_nodes);
            }
        }

#if 0
        printf("\tSource:      %s\n", netloc_pretty_print_node_t(cur_src_node));
#endif

        dst_idx = 0;
        netloc_lookup_table_iterator_reset(hti_dst);
        while( !netloc_lookup_table_iterator_at_end(hti_dst) ) {
            cur_dst_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti_dst);
            if( NULL == cur_dst_node ) {
                break;
            }

            // Skip path to self
            if( src_idx == dst_idx ) {
                ++dst_idx;
                continue;
            }

            // JJH: For now limit to just the "host" nodes
            if( NETLOC_NODE_TYPE_HOST != cur_dst_node->node_type ) {
                ++dst_idx;
                continue;
            }

#if 0
            printf("Computing a path between the following two nodes\n");
            printf("\tSource:      %s\n", netloc_pretty_print_node_t(cur_src_node));
            printf("\tDestination: %s\n", netloc_pretty_print_node_t(cur_dst_node));
#endif
            /*
             * Calculate the path between these nodes
             */
            ret = netloc_dc_compute_path_between_nodes(dc_handle,
                                                       cur_src_node,
                                                       cur_dst_node,
                                                       &num_edges,
                                                       &edges,
                                                       false);
            if( NETLOC_SUCCESS != ret ) {
                fprintf(stderr, "Error: Failed to compute a path between the following two nodes\n");
                fprintf(stderr, "Error: Source:      %s\n", netloc_pretty_print_node_t(cur_src_node));
                fprintf(stderr, "Error: Destination: %s\n", netloc_pretty_print_node_t(cur_dst_node));
                return ret;
            }


            /*
             * Store that path in the data collection
             */
            ret = netloc_dc_append_path(dc_handle,
                                        cur_src_node->physical_id,
                                        cur_dst_node->physical_id,
                                        num_edges,
                                        edges,
                                        false);
            if( NETLOC_SUCCESS != ret ) {
                fprintf(stderr, "Error: Could not append the physical path between the following two nodes\n");
                fprintf(stderr, "Error: Source:      %s\n", netloc_pretty_print_node_t(cur_src_node));
                fprintf(stderr, "Error: Destination: %s\n", netloc_pretty_print_node_t(cur_dst_node));
                return ret;
            }

            free(edges);

            ++dst_idx;
        }

        ++src_idx;
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti_src);
    netloc_dt_lookup_table_iterator_t_destruct(hti_dst);

    return NETLOC_SUCCESS;
}

static int process_logical_paths(netloc_data_collection_handle_t *dc_handle)
{
    int ret;
    json_t *json = NULL;

    const char * key = NULL;
    const char * key2 = NULL;
    json_t * value = NULL;
    json_t * value2 = NULL;

    netloc_dt_lookup_table_t all_routes = NULL;
    netloc_dt_lookup_table_t tmp_routes = NULL;

    netloc_dt_lookup_table_iterator_t hti = NULL;
    netloc_dt_lookup_table_iterator_t hti2 = NULL;

    netloc_dt_lookup_table_iterator_t hti_src = NULL;
    netloc_dt_lookup_table_iterator_t hti_dst = NULL;
    netloc_node_t *cur_src_node = NULL;
    netloc_node_t *cur_dst_node = NULL;

    char *out_port = NULL;
    int src_idx, dst_idx;
    int num_edges = 0;
    netloc_edge_t **edges = NULL;

    printf("Status: Processing Logical Paths\n");

    /*
     * Open the file
     */
    ret = load_json_from_file(out_file_log_prep, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to mmap the file.\n");
        return ret;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        return NETLOC_ERROR;
    }

    /*
     * Pull in the logical path 'hops' into memory
     * Lookup Table[Key   = GUID of switch,
     *              Value = Lookup Table[Key   = LID,
     *                                   Value = Output Port
     *                                   ]
     *             ]
     */
    all_routes = calloc(1, sizeof(*all_routes));
    netloc_lookup_table_init(all_routes, 1, 0);

    json_object_foreach(json, key, value) {
        //printf("Switch %s\n", key);
        tmp_routes = calloc(1, sizeof(*tmp_routes));
        netloc_lookup_table_init(tmp_routes, 1, 0);

        json_object_foreach(value, key2, value2) {
            //printf("\t%s \t %s\n", key2, json_string_value(value2));
            netloc_lookup_table_append(tmp_routes, key2, strdup(json_string_value(value2)));
        }

        netloc_lookup_table_append(all_routes, key, tmp_routes);
    }

    /*
     * Compute logical path between all 'host' node pairs
     */
    src_idx = 0;
    hti_src = netloc_dt_lookup_table_iterator_t_construct(dc_handle->node_list);
    hti_dst = netloc_dt_lookup_table_iterator_t_construct(dc_handle->node_list);

    netloc_lookup_table_iterator_reset(hti_src);
    while( !netloc_lookup_table_iterator_at_end(hti_src) ) {
        cur_src_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti_src);
        if( NULL == cur_src_node ) {
            break;
        }

        // JJH: For now limit to just the "host" nodes
        if( NETLOC_NODE_TYPE_HOST != cur_src_node->node_type ) {
            ++src_idx;
            continue;
        }

#if 0
        printf("\tSource:      %s\n", netloc_pretty_print_node_t( cur_src_node ));
#endif
        dst_idx = 0;

        netloc_lookup_table_iterator_reset(hti_dst);
        while( !netloc_lookup_table_iterator_at_end(hti_dst) ) {
            cur_dst_node = (netloc_node_t*)netloc_lookup_table_iterator_next_entry(hti_dst);
            if( NULL == cur_dst_node ) {
                break;
            }

            // Skip path to self
            if( src_idx == dst_idx ) {
                dst_idx++;
                continue;
            }

            // JJH: For now limit to just the "host" nodes
            if( NETLOC_NODE_TYPE_HOST != cur_dst_node->node_type ) {
                dst_idx++;
                continue;
            }

#if 0
            printf("Computing a path between the following two nodes\n");
            printf("\tSource:      %s\n", netloc_pretty_print_node_t( cur_src_node ));
            printf("\tDestination: %s\n", netloc_pretty_print_node_t( cur_dst_node ));
#endif
            /*
             * Calculate the path between these nodes
             */
            ret = process_logical_paths_between_nodes(dc_handle,
                                                      all_routes,
                                                      cur_src_node,
                                                      cur_dst_node,
                                                      &num_edges,
                                                      &edges);
            if( NETLOC_SUCCESS != ret ) {
                netloc_dt_lookup_table_iterator_t_destruct(hti_src);
                netloc_dt_lookup_table_iterator_t_destruct(hti_dst);
                free(all_routes);
                fprintf(stderr, "Error: Failed to compute a path between the following two nodes\n");
                fprintf(stderr, "Error: Source:      %s\n", netloc_pretty_print_node_t( cur_src_node ));
                fprintf(stderr, "Error: Destination: %s\n", netloc_pretty_print_node_t( cur_dst_node ));
                return ret;
            }


            /*
             * Store that path in the data collection
             */
            ret = netloc_dc_append_path(dc_handle,
                                        cur_src_node->physical_id,
                                        cur_dst_node->physical_id,
                                        num_edges,
                                        edges,
                                        true);
            if( NETLOC_SUCCESS != ret ) {
                netloc_dt_lookup_table_iterator_t_destruct(hti_src);
                netloc_dt_lookup_table_iterator_t_destruct(hti_dst);
                free(all_routes);
                free(edges);
                fprintf(stderr, "Error: Could not append the logical path between the following two nodes\n");
                fprintf(stderr, "Error: Source:      %s\n", netloc_pretty_print_node_t( cur_src_node ));
                fprintf(stderr, "Error: Destination: %s\n", netloc_pretty_print_node_t( cur_dst_node ));
                return ret;
            }

            free(edges);

            dst_idx++;
        }

        src_idx++;
    }

    /*
     * Cleanup
     */
    hti = netloc_dt_lookup_table_iterator_t_construct( all_routes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }
        tmp_routes = (netloc_dt_lookup_table_t)netloc_lookup_table_access(all_routes, key);

        hti2 = netloc_dt_lookup_table_iterator_t_construct( tmp_routes );
        while( !netloc_lookup_table_iterator_at_end(hti2) ) {
            key2 = netloc_lookup_table_iterator_next_key(hti2);
            if( NULL == key2 ) {
                break;
            }
            out_port = (char *)netloc_lookup_table_access(tmp_routes, key2);
            free(out_port);
        }
        netloc_dt_lookup_table_iterator_t_destruct(hti2);
        netloc_lookup_table_destroy(tmp_routes);
        free(tmp_routes);
    }
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(all_routes);
    free(all_routes);

    netloc_dt_lookup_table_iterator_t_destruct(hti_src);
    netloc_dt_lookup_table_iterator_t_destruct(hti_dst);

    /*
     * Remove the temporary prep file
     */
    ret = remove(out_file_log_prep);
    if( 0 != ret ) {
        fprintf(stderr, "Error: Failed to remove the temporary file %s\n", out_file_log_prep);
        return ret;
    }

    if(NULL != json) {
        json_decref(json);
        json = NULL;
    }

    return 0;
}

static int process_logical_paths_between_nodes(netloc_data_collection_handle_t *handle,
                                               netloc_dt_lookup_table_t all_routes,
                                               netloc_node_t *src_node,
                                               netloc_node_t *dest_node,
                                               int *num_edges,
                                               netloc_edge_t ***edges)
{
    int exit_status = NETLOC_SUCCESS;
    netloc_node_t *cur_node = NULL;
    netloc_edge_t *cur_edge = NULL;
    netloc_dt_lookup_table_t cur_routes = NULL;
    char * output_port = NULL;
    int i, len;

    if( src_node == dest_node ) {
        fprintf(stderr, "Error: Source and Destination node are the same\n");
        (*num_edges) = 0;
        (*edges) = NULL;
        return NETLOC_ERROR;
    }

    if( src_node->num_edges <= 0 ) {
        fprintf(stderr, "Error: Source node has no edges\n");
        (*num_edges) = 0;
        (*edges) = NULL;
        return NETLOC_ERROR;
    }

    if( dest_node->num_edges <= 0 ) {
        fprintf(stderr, "Error: Destination node has no edges\n");
        (*num_edges) = 0;
        (*edges) = NULL;
        return NETLOC_ERROR;
    }

    /*
     * Access first edge from the src_node
     * Since we assume that the src_node is a 'host' then it should only have one edge
     */
    (*num_edges) = 1;
    (*edges) = (netloc_edge_t**)malloc(sizeof(netloc_edge_t*) * (*num_edges));
    if( NULL == (*edges) ) {
        fprintf(stderr, "Error: Failed to allocate memory for edge list (%d items)\n", (*num_edges));
        return NETLOC_ERROR;
    }

    cur_edge = src_node->edges[0];
    // Append to the edge list
    (*edges)[(*num_edges)-1] = cur_edge;
    cur_node = netloc_dc_get_node_by_physical_id(handle, cur_edge->dest_node_id);

    /*
     * While we have not reached the dest_node
     */
    while( cur_node != dest_node ) {
        /*
         * Get output port from this switch for the LID we are tracing
         */
        cur_routes = (netloc_dt_lookup_table_t)netloc_lookup_table_access(all_routes, cur_node->physical_id);
        if( NULL == cur_routes ) {
            fprintf(stderr, "Error: No routing information for node %s\n", netloc_pretty_print_node_t(cur_node));
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }

        // Look for the destination LID at this hop, to get output port
        output_port = (char*)netloc_lookup_table_access(cur_routes, dest_node->logical_id);
        if( NULL == output_port ) {
            fprintf(stderr, "Error: No output port associated with LID %s on node %s\n",
                    dest_node->logical_id, netloc_pretty_print_node_t(cur_node));
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }


        /*
         * Find the edge on this switch that matches the output port
         */
        cur_edge = NULL;
        for(i = 0; i < cur_node->num_edges; ++i) {
            if( strlen(output_port) > strlen(cur_node->edges[i]->src_port_id) ) {
                len = strlen(output_port);
            }
            else {
                len = strlen(cur_node->edges[i]->src_port_id);
            }
            if( 0 == strncmp(cur_node->edges[i]->src_port_id, output_port, len) ) {
                cur_edge = cur_node->edges[i];
                break;
            }
        }

        if( NULL == cur_edge ) {
            fprintf(stderr, "Error: Could not find output port %s associated with LID %s on node %s\n",
                    output_port, dest_node->logical_id, netloc_pretty_print_node_t(cur_node));
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }

        /*
         * Save this edge in our edge list
         */
        (*num_edges) += 1;
        (*edges) = (netloc_edge_t**)realloc((*edges), sizeof(netloc_edge_t*) * (*num_edges));
        if( NULL == (*edges) ) {
            fprintf(stderr, "Error: Failed to allocate memory for edge list (%d items)\n", (*num_edges));
            return NETLOC_ERROR;
        }
        (*edges)[(*num_edges)-1] = cur_edge;

        /*
         * Hop along this edge to the next node
         */
        cur_node = netloc_dc_get_node_by_physical_id(handle, cur_edge->dest_node_id);
    }

 cleanup:

    return exit_status;
}

static int check_dat_files() {
    int ret, exit_status = NETLOC_SUCCESS;
    char * spec = NULL;
    char * hosts_filename = NULL;
    char * switches_filename = NULL;

    char *search_uri = NULL;
    netloc_topology_t topology;

    netloc_dt_lookup_table_t nodes = NULL;
    netloc_dt_lookup_table_iterator_t hti = NULL;
    const char * key = NULL;
    netloc_node_t *node = NULL;

    int num_bad = 0;
    netloc_network_t *network = NULL;

    int total_num_edges = 0;

    printf("Status: Validating the output...\n");

    network = netloc_dt_network_t_construct();
    network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    network->subnet_id    = strdup(subnet);
    asprintf(&search_uri, "file://%s/", outdir);

    if( NETLOC_SUCCESS != (ret = netloc_find_network(search_uri, network)) ) {
        fprintf(stderr, "Error: Cannot find the network data. (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Attach to Netloc
     */
    if( NETLOC_SUCCESS != (ret = netloc_attach(&topology, *network)) ) {
        fprintf(stderr, "Error: Cannot attach the topology\n");
        exit_status = ret;
        goto cleanup;
    }


    /*
     * Check 'hosts'
     */
    ret = netloc_get_all_host_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Cannot access the list of hosts!\n");
        exit_status = ret;
        goto cleanup;
    }

    printf("\tNumber of hosts   : %4d\n", netloc_lookup_table_size(nodes) );

    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(nodes, key);

        if( NETLOC_NODE_TYPE_INVALID == node->node_type ) {
            printf("Host Node: %s is invalid\n", netloc_pretty_print_node_t(node) );
            num_bad++;
        }
        else {
            total_num_edges += node->num_edges;
        }
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    /*
     * Check the 'switches'
     */
    ret = netloc_get_all_switch_nodes(topology, &nodes);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Cannot access the list of switches!\n");
        exit_status = ret;
        goto cleanup;
    }

    printf("\tNumber of switches: %4d\n", netloc_lookup_table_size(nodes) );

    hti = netloc_dt_lookup_table_iterator_t_construct( nodes );
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        key = netloc_lookup_table_iterator_next_key(hti);
        if( NULL == key ) {
            break;
        }

        node = (netloc_node_t*)netloc_lookup_table_access(nodes, key);

        if( NETLOC_NODE_TYPE_INVALID == node->node_type ) {
            printf("Switch Node: %s is invalid\n", netloc_pretty_print_node_t(node) );
            num_bad++;
        }
        else {
            total_num_edges += node->num_edges;
        }
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti);
    netloc_lookup_table_destroy(nodes);
    free(nodes);

    if( num_bad > 0 ) {
        fprintf(stderr, "Error: Found %2d malformed nodes in the .dat files\n", num_bad);
        exit_status = NETLOC_ERROR;
    }

    printf("\tNumber of edges   : %4d\n", total_num_edges );

    /*
     * Cleanup
     */
    if( NETLOC_SUCCESS != (ret = netloc_detach(topology)) ) {
        fprintf(stderr, "Error: Failed to detach the topology\n");
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    netloc_dt_network_t_destruct(network);
    free(search_uri);
    free(spec);
    free(hosts_filename);
    free(switches_filename);
    return exit_status;
}

/*********************************************************/
static int load_json_from_file(char * fname, json_t **json) {
    const char *memblock = NULL;
    int fd, filesize, pagesize, res;
    struct stat sb;


    // Open file and get the needed file info
    res = fd = open(fname, O_RDONLY);
    if( 0 > res ) {
        fprintf(stderr, "Error: Cannot open the file %s\n", fname);
        goto CLEANUP;
    }
    res = fstat(fd, &sb);
    if( 0 != res ) {
        fprintf(stderr, "Error: Cannot stat the file %s\n", fname);
        goto CLEANUP;
    }

    // Set some useful values
    filesize = sb.st_size;
    pagesize = getpagesize();
    filesize = (filesize/pagesize)*pagesize + pagesize;

    // mmap the file
    memblock = (const char *) mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == memblock) {
        fprintf(stderr, "Error: mmap failed\n");
        res = NETLOC_ERROR;
        goto CLEANUP;
    }

    // load the JSON from the file
    (*json) = json_loads(memblock, 0, NULL);
    if(NULL == (*json)) {
        fprintf(stderr, "Error: json_loads on mmaped file failed\n");
        res = NETLOC_ERROR;
        goto CLEANUP;
    }

    // munmap the file and close it
 CLEANUP:
    if(NULL != memblock) {
        res = munmap((char *)memblock, filesize);
        if(0 != res) {
            fprintf(stderr, "Error: munmap failed!\n");
        }
        memblock = NULL;
    }
    if(0 <= fd) {
        close(fd);
    }

    return res;
}

static int extract_network_info_from_json_file(netloc_network_t *network, char *dat_file)
{
    int ret;
    json_t *json = NULL;

    const char * key = NULL;
    json_t * value = NULL;
    const char * str_val = NULL;
    char * tmp_dat_file = NULL;

    tmp_dat_file = strdup(dat_file);

    /*
     * Open the file
     */
    ret = load_json_from_file(tmp_dat_file, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to mmap the file.\n");
        return ret;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        return NETLOC_ERROR;
    }

    /*
     * Process all of the node information in the file
     */
    json_object_foreach(json, key, value) {
        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_NETWORK_TYPE ));
        network->network_type = netloc_encode_network_type(str_val);

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_SUBNET_ID ));
        if( NULL != str_val ) {
            network->subnet_id = strdup(str_val);
        } else {
            network->subnet_id = NULL;
        }

        asprintf(&network->data_uri, "file://%s", dirname(tmp_dat_file));

        str_val = json_string_value( json_object_get( value, PERL_JSON_EDGE_DESCRIPTION ));
        if( NULL != str_val ) {
            network->description = strdup(str_val);
        } else {
            network->description = NULL;
        }

        break;
    }

    /*
     * Cleanup
     */
    free(tmp_dat_file);
    json_decref(json);
    return NETLOC_SUCCESS;
}

static int process_nodes_dat(netloc_data_collection_handle_t *dc_handle, char *dat_file)
{
    int ret;
    json_t *json = NULL;

    const char * key = NULL;
    json_t * value = NULL;

    const char * str_val = NULL;

    json_t * connections = NULL;
    const char * ckey = NULL;
    json_t * cvalue = NULL;

    netloc_node_t *node = NULL;
    netloc_edge_t *edge = NULL;

    size_t i;
    json_t * json_edge = NULL;

    /*
     * Open the file
     */
    ret = load_json_from_file(dat_file, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to mmap the file.\n");
        return ret;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        return NETLOC_ERROR;
    }

    /*
     * Process all of the node information in the file
     */
    json_object_foreach(json, key, value) {
        /*
         * Setup the node information
         */
        node = netloc_dt_node_t_construct();

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_NETWORK_TYPE ));
        node->network_type = netloc_encode_network_type(str_val);

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_TYPE ));
        node->node_type    = netloc_encode_node_type(str_val);

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_PHYSICAL_ID ));
        node->physical_id  = (NULL == str_val ? NULL : strdup(str_val));

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_LOGICAL_ID ));
        node->logical_id   = (NULL == str_val ? NULL : strdup(str_val));

        str_val = json_string_value( json_object_get( value, PERL_JSON_NODE_SUBNET_ID ));
        node->subnet_id    = (NULL == str_val ? NULL : strdup(str_val));

        str_val = json_string_value( json_object_get( value, PERL_JSON_EDGE_DESCRIPTION ));
        node->description  = (NULL == str_val ? NULL : strdup(str_val));

        //printf("Debug: Node %s\n", netloc_pretty_print_node_t(node));

        /*
         * Process the connections
         */
        connections = json_object_get(value, PERL_JSON_NODE_CONNECTIONS);

        // Create a new edge for each connection
        json_object_foreach(connections, ckey, cvalue) {
            if( !json_is_array(cvalue) ) {
                break;
            }

            for(i = 0; i < json_array_size(cvalue); ++i) {
                json_edge = json_array_get(cvalue, i);

                edge = netloc_dt_edge_t_construct();

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_FROM ));
                edge->src_node_id = (NULL == str_val ? NULL : strdup(str_val));

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_TYPE_FROM ));
                edge->src_node_type = netloc_encode_node_type(str_val);

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_ID_FROM ));
                edge->src_port_id = (NULL == str_val ? NULL : strdup(str_val));


                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_TO ));
                edge->dest_node_id = (NULL == str_val ? NULL : strdup(str_val));

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_TYPE_TO ));
                edge->dest_node_type = netloc_encode_node_type(str_val);

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_PORT_ID_TO ));
                edge->dest_port_id = (NULL == str_val ? NULL : strdup(str_val));


                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_SPEED ));
                edge->speed = (NULL == str_val ? NULL : strdup(str_val));

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_WIDTH ));
                edge->width = (NULL == str_val ? NULL : strdup(str_val));

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_GBITS ));
                edge->gbits = (NULL == str_val ? 1.0 : atof(str_val));

                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_DESCRIPTION ));
                edge->description = (NULL == str_val ? NULL : strdup(str_val));

                // Append to the edges list
                ret = netloc_dc_append_edge_to_node(dc_handle, node, edge);
                if( NETLOC_SUCCESS != ret ) {
                    fprintf(stderr, "Error: Failed to append the edge to the node to the data collection\n");
                    return ret;
                }
                // The append_edge duplicates the edge, so we should free it
                netloc_dt_edge_t_destruct(edge);
                edge = NULL;

                //printf("\t\t%s\n", netloc_pretty_print_edge_t(edge));
            }
        }
        //printf("Debug: Node %s\n", netloc_pretty_print_node_t(node));

        /*
         * Add the node to the data store
         */
        ret = netloc_dc_append_node(dc_handle, node);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Failed to append the node to the data collection\n");
            return ret;
        }

        netloc_dt_node_t_destruct(node);
    }

    /*
     * Cleanup
     */
    if(NULL != json) {
        json_decref(json);
        json = NULL;
    }

    return NETLOC_SUCCESS;
}
