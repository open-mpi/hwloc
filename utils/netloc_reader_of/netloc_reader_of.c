/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
 *
 * Copyright © 2015-2016 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libgen.h> // for dirname

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <jansson.h>

#include "netloc/dc.h"
#include "private/netloc.h"

#include "perl_json_support.h"


const char * ARG_OUTDIR           = "--outdir";
const char * ARG_SHORT_OUTDIR     = "-o";
const char * ARG_SUBNET           = "--subnet";
const char * ARG_SHORT_SUBNET     = "-s";
const char * ARG_CONTROLLER       = "--controller";
const char * ARG_SHORT_CONTROLLER = "-c";
const char * ARG_ADDRESS          = "--addr";
const char * ARG_SHORT_ADDRESS    = "-a";
const char * ARG_AUTH_USER        = "--username";
const char * ARG_SHORT_AUTH_USER  = "-u";
const char * ARG_AUTH_PASS        = "--password";
const char * ARG_SHORT_AUTH_PASS  = "-p";
const char * ARG_HELP             = "--help";
const char * ARG_SHORT_HELP       = "-h";

/*
 * Parse command line arguments
 */
static int parse_args(int argc, char ** argv);

/*
 * Run the parser
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
 * Check the resulting .dat files
 */
static int check_dat_files();


/*
 * Output directory
 */
static char * outdir = NULL;
static char * out_file_nodes = NULL;

/*
 * Subnet
 */
static char * subnet = NULL;

/*
 * Connection information
 */
static char * uri_address = NULL;
static char * auth_username = NULL;
static char * auth_password = NULL;

/*
 * Valid controllers
 * The short names must match the corresponding perl script
 */
static int num_valid_controllers = 4;
static int cur_controller_idx = -1;
const char * valid_controllers[4] = {"noop", // Must be index 0
                                     "floodlight",
                                     "opendaylight",
                                     "xnc"};
const char * valid_controllers_cmds[4] = {"true",
                                          "netloc_reader_of_floodlight",
                                          "netloc_reader_of_opendaylight",
                                          "netloc_reader_of_opendaylight"};

/*
 * Controller
 */
static char * controller = NULL;


int main(int argc, char ** argv) {
    int ret, exit_status = NETLOC_SUCCESS;
    int i;
    netloc_network_t *network = NULL;
    netloc_data_collection_handle_t *dc_handle = NULL;

    /*
     * Parse Args
     */
    if( 0 != parse_args(argc, argv) ) {
        printf("Usage: %s %s|%s <controller> [%s|%s <subnet id>] [%s|%s <output directory>] [%s|%s <URL Address:Port>] [%s|%s <username>] [%s|%s <password>] [%s|%s]\n",
               argv[0],
               ARG_CONTROLLER, ARG_SHORT_CONTROLLER,
               ARG_SUBNET, ARG_SHORT_SUBNET,
               ARG_OUTDIR, ARG_SHORT_OUTDIR,
               ARG_ADDRESS, ARG_SHORT_ADDRESS,
               ARG_AUTH_USER, ARG_SHORT_AUTH_USER,
               ARG_AUTH_PASS, ARG_SHORT_AUTH_PASS,
               ARG_HELP, ARG_SHORT_HELP);
        printf("       Default %-10s = \"unknown\"\n", ARG_SUBNET);
        printf("       Default %-10s = \"127.0.0.1:8080\"\n", ARG_ADDRESS);
        printf("       Default %-10s = current working directory\n", ARG_OUTDIR);
        printf("       Valid Options for %s:\n", ARG_CONTROLLER );
        // Note: Hide 'noop' since it is only meant for debugging, and not for normal use
        for(i = 1; i < num_valid_controllers; ++i) {
            printf("\t\t%s\n", valid_controllers[i] );
        }

        return NETLOC_ERROR;
    }


    /*
     * Run the parser requested
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
    free(subnet);
    free(uri_address);
    free(auth_username);
    free(auth_password);
    return exit_status;
}

static int parse_args(int argc, char ** argv) {
    int ret = NETLOC_SUCCESS;
    int i;
    bool found;

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
         * --controller
         */
        else if( 0 == strncmp(ARG_CONTROLLER,       argv[i], strlen(ARG_CONTROLLER)) ||
                 0 == strncmp(ARG_SHORT_CONTROLLER, argv[i], strlen(ARG_SHORT_CONTROLLER)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_CONTROLLER );
                return NETLOC_ERROR;
            }
            controller = strdup(argv[i]);
        }
        /*
         * --addr
         */
        else if( 0 == strncmp(ARG_ADDRESS,       argv[i], strlen(ARG_ADDRESS)) ||
                 0 == strncmp(ARG_SHORT_ADDRESS, argv[i], strlen(ARG_SHORT_ADDRESS)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_ADDRESS );
                return NETLOC_ERROR;
            }
            uri_address = strdup(argv[i]);
        }
        /*
         * --username
         */
        else if( 0 == strncmp(ARG_AUTH_USER,       argv[i], strlen(ARG_AUTH_USER)) ||
                 0 == strncmp(ARG_SHORT_AUTH_USER, argv[i], strlen(ARG_SHORT_AUTH_USER)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_AUTH_USER );
                return NETLOC_ERROR;
            }
            auth_username = strdup(argv[i]);
        }
        /*
         * --password
         */
        else if( 0 == strncmp(ARG_AUTH_PASS,       argv[i], strlen(ARG_AUTH_PASS)) ||
                 0 == strncmp(ARG_SHORT_AUTH_PASS, argv[i], strlen(ARG_SHORT_AUTH_PASS)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_AUTH_PASS );
                return NETLOC_ERROR;
            }
            auth_password = strdup(argv[i]);
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
        subnet = strdup("unknown");
    }

    /*
     * Check URI Address:Port Parameter
     */
    if( NULL == uri_address || strlen(uri_address) <= 0 ) {
        free(uri_address);
        uri_address = strdup("127.0.0.1:8080");
    }

    /*
     * Check Controller Parameter
     */
    if( NULL == controller || strlen(controller) <= 0 ) {
        fprintf(stderr, "Error: A valid controller must be specified with the %s option\n", ARG_CONTROLLER);
        found = false;
    }
    else {
        found = false;
        for(i = 0; i < num_valid_controllers; ++i) {
            if( 0 == strncmp(valid_controllers[i], controller, strlen(valid_controllers[i])) ) {
                cur_controller_idx = i;
                found = true;
                break;
            }
        }
        if( !found ) {
            fprintf(stderr, "Error: Invalid controller specified: %s\n", controller);
        }
    }
    if( !found ) {
        return NETLOC_ERROR;
    }

    asprintf(&out_file_nodes,    "%sETH-%s-nodes.dat",    outdir, subnet);

    /*
     * Display Parsed Arguments
     */
    printf("  Output Directory : %s\n", outdir);
    printf("  Subnet           : %s\n", subnet);
    printf("  Controller       : %s\n", controller);
    printf("  Address:Port     : %s\n", uri_address);
    printf("  Username         : %s\n", (NULL == auth_username ? "<none>" : auth_username) );

    return ret;
}

static int run_parser() {
    char * command = NULL;
    int ret;
    char * auth_u_str = NULL;
    char * auth_p_str = NULL;

    printf("Querying the %s network controller...\n", valid_controllers[cur_controller_idx] );

    if( NULL != auth_username ) {
        asprintf(&auth_u_str, "-u %s", auth_username);
    }
    else {
        auth_u_str = strdup("");
    }

    if( NULL != auth_password ) {
        asprintf(&auth_p_str, "-p %s", auth_password);
    }
    else {
        auth_p_str = strdup("");
    }

    asprintf(&command, "%s -s %s -o %s -a %s %s %s",
             valid_controllers_cmds[cur_controller_idx], subnet, out_file_nodes, uri_address, auth_u_str, auth_p_str);

    ret = system(command);
    if( ret != 0 ) {
        fprintf(stderr, "Error: Failed to query the %s controller! See error message above for more details\n", valid_controllers[cur_controller_idx]);
        fprintf(stderr, "Error: Command: %s\n", command);
        free(auth_u_str);
        free(auth_p_str);
        free(command);
        return ret;
    }

    free(auth_u_str);
    free(auth_p_str);
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

    printf("Status: Computing Physical Paths\n");

    /*
     * Calculate the path from all sources to all destinations
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
            src_idx++;
            continue;
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

            dst_idx++;
        }

        src_idx++;
    }

    netloc_dt_lookup_table_iterator_t_destruct(hti_src);
    netloc_dt_lookup_table_iterator_t_destruct(hti_dst);

    return NETLOC_SUCCESS;
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
    network->network_type = NETLOC_NETWORK_TYPE_ETHERNET;
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
     * Check the 'hosts'
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
     * Check 'switches'
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

                edge->gbits = json_real_value( json_object_get( json_edge, PERL_JSON_EDGE_WIDTH ));


                str_val = json_string_value( json_object_get( json_edge, PERL_JSON_EDGE_DESCRIPTION ));
                edge->description = (NULL == str_val ? NULL : strdup(str_val));

                // Append to the edges list
                ret = netloc_dc_append_edge_to_node(dc_handle, node, edge);
                if( NETLOC_SUCCESS != ret ) {
                    fprintf(stderr, "Error: Failed to append the edge to the node to the data collection\n");
                    return ret;
                }

                //printf("\t\t%s\n", netloc_pretty_print_edge_t(edge));

                // The append_edge duplicates the edge, so we should free it
                netloc_dt_edge_t_destruct(edge);
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
