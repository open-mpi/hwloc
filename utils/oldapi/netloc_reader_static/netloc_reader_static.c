/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2016 Inria.  All rights reserved.
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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <jansson.h>

#include "netloc/dc.h"
#include "private/netloc.h"

#define CMP_JSON_KEYS(key, def) strncmp(key, def, strlen(def))

/*
"network_info":
    {"network_type":"ETH",
     "subnet_id":"unknown",
     "version":"1",
     "description":"This is an example network with two nodes and one switch"
    },
*/
#define JSON_KEY_NETWORK_INFO  "network_info"
#define JSON_KEY_NETWORK_INFO_NETWORK_TYPE "network_type"
#define JSON_KEY_NETWORK_INFO_SUBNET_ID    "subnet_id"
#define JSON_KEY_NETWORK_INFO_VERSION      "version"
#define JSON_KEY_NETWORK_INFO_DESCRIPTION  "description"

#define JSON_DEFAULT_NETWORK_INFO_SUBNET_ID    "unknown"
#define JSON_DEFAULT_NETWORK_INFO_VERSION      "1"
#define JSON_DEFAULT_NETWORK_INFO_DESCRIPTION  ""

/*
"node_info":
    {"nodes":[
      {"type":"SW",
       "logical_id":"10:00:00:00:00:00:00:06",
       "physical_id":"10:00:00:00:00:00:00:06",
       "description":"None"
      },
      {"type":"CA",
       "logical_id":"10.0.0.1",
       "physical_id":"00:00:00:00:00:01",
       "description":"node01"
      },
      {"type":"CA",
       "logical_id":"10.0.0.2",
       "physical_id":"00:00:00:00:00:02",
       "description":"node02"
      }
    ]},
 */
#define JSON_KEY_NODE_INFO  "node_info"
#define JSON_KEY_NODE_INFO_ARRAY  "nodes"
#define JSON_KEY_NODE_INFO_TYPE        "type"
#define JSON_KEY_NODE_INFO_LOGICAL_ID  "logical_id"
#define JSON_KEY_NODE_INFO_PHYSICAL_ID "physical_id"
#define JSON_KEY_NODE_INFO_DESCRIPTION "description"

#define JSON_DEFAULT_NODE_INFO_LOGICAL_ID  ""
#define JSON_DEFAULT_NODE_INFO_DESCRIPTION ""

/*
"edge_info":
    {"edges":[
      {"source_node_id":"10:00:00:00:00:00:00:06",
       "dest_node_id":"00:00:00:00:00:02",
       "source_port":"-1",
       "dest_port":"-1",
       "width":"1",
       "speed":"1",
       "gbits":1.0,
       "description":"None",
       "bidirectional":true
      },
      {"source_node_id":"00:00:00:00:00:01",
       "dest_node_id":"10:00:00:00:00:00:00:06"
      },
      {"dest_node_id":"00:00:00:00:00:01",
       "source_node_id":"10:00:00:00:00:00:00:06"
      }
    ]}
 */
#define JSON_KEY_EDGE_INFO  "edge_info"
#define JSON_KEY_EDGE_INFO_ARRAY  "edges"
#define JSON_KEY_EDGE_INFO_SRC_NODE_ID  "source_node_id"
#define JSON_KEY_EDGE_INFO_DST_NODE_ID  "dest_node_id"
#define JSON_KEY_EDGE_INFO_SRC_PORT     "source_port"
#define JSON_KEY_EDGE_INFO_DST_PORT     "dest_port"
#define JSON_KEY_EDGE_INFO_WIDTH        "width"
#define JSON_KEY_EDGE_INFO_SPEED        "speed"
#define JSON_KEY_EDGE_INFO_GBITS        "gbits"
#define JSON_KEY_EDGE_INFO_DIRECTIONAL  "bidirectional"
#define JSON_KEY_EDGE_INFO_DESCRIPTION  "description"


#define JSON_DEFAULT_EDGE_INFO_SRC_PORT     "-1"
#define JSON_DEFAULT_EDGE_INFO_DST_PORT     "-1"
#define JSON_DEFAULT_EDGE_INFO_WIDTH        "1"
#define JSON_DEFAULT_EDGE_INFO_SPEED        "1"
#define JSON_DEFAULT_EDGE_INFO_GBITS        1.0
#define JSON_DEFAULT_EDGE_INFO_DIRECTIONAL  false
#define JSON_DEFAULT_EDGE_INFO_DESCRIPTION  ""


const char * ARG_OUTDIR           = "--outdir";
const char * ARG_SHORT_OUTDIR     = "-o";
const char * ARG_INPUT            = "--input";
const char * ARG_SHORT_INPUT      = "-i";
const char * ARG_PROGRESS         = "--progress";
const char * ARG_SHORT_PROGRESS   = "-p";
const char * ARG_HELP             = "--help";
const char * ARG_SHORT_HELP       = "-h";

/*
 * Load the json object from the input file
 */
static int load_json_from_file(char * fname, json_t **json);

/*
 * Parse command line arguments
 */
static int parse_args(int argc, char ** argv);

/*
 * Fill in the network information
 */
static int extract_network_info(netloc_network_t *network);

/*
 * Convert the temporary node file to the proper netloc format
 */
static int convert_nodes_file(netloc_data_collection_handle_t *dc_handle);

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

/*
 * Subnet
 */
static char * subnet = NULL;

/*
 * Input file
 */
static char * inputfile = NULL;

/*
 * If progress should be shown
 */
static bool progress = false;

int main(int argc, char ** argv) {
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_network_t *network = NULL;
    netloc_data_collection_handle_t *dc_handle = NULL;

    /*
     * Parse Args
     */
    if( 0 != parse_args(argc, argv) ) {
        printf("Usage: %s %s|%s <input_file> [%s|%s <output directory>] [%s|%s] [%s|%s]\n",
               argv[0],
               ARG_INPUT, ARG_SHORT_INPUT,
               ARG_OUTDIR, ARG_SHORT_OUTDIR,
               ARG_PROGRESS, ARG_SHORT_PROGRESS,
               ARG_HELP, ARG_SHORT_HELP);
        printf("       Default %-10s = current working directory\n", ARG_OUTDIR);
        return NETLOC_ERROR;
    }


    /*
     * Setup network information
     */
    network = netloc_dt_network_t_construct();
    ret = extract_network_info(network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to extract network information from the file: %s\n", inputfile);
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
     * Convert the node input file into netloc data types and attach them to
     * the data collection handle
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
    free(inputfile);
    free(subnet);
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
         * --input
         */
        else if( 0 == strncmp(ARG_INPUT,       argv[i], strlen(ARG_INPUT)) ||
                 0 == strncmp(ARG_SHORT_INPUT, argv[i], strlen(ARG_SHORT_INPUT)) ) {
            ++i;
            if( i >= argc ) {
                fprintf(stderr, "Error: Must supply an argument to %s\n", ARG_INPUT );
                return NETLOC_ERROR;
            }
            inputfile = strdup(argv[i]);
        }
        /*
         * --progress
         */
        else if( 0 == strncmp(ARG_PROGRESS,       argv[i], strlen(ARG_PROGRESS)) ||
                 0 == strncmp(ARG_SHORT_PROGRESS, argv[i], strlen(ARG_SHORT_PROGRESS)) ) {
            progress = true;
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
     * Input file
     */
    if( NULL == inputfile || strlen(inputfile) <= 0 ) {
        fprintf(stderr, "Error: Must supply an input filename to process\n");
        return NETLOC_ERROR;
    }

    /*
     * Display Parsed Arguments
     */
    printf("  Input file       : %s\n", inputfile);
    printf("  Output Directory : %s\n", outdir);

    return ret;
}

static int extract_network_info(netloc_network_t *network) {
    int ret;
    json_t *json = NULL;

    const char * key_lvl_1 = NULL;
    json_t * value_lvl_1 = NULL;

    const char * key_lvl_2 = NULL;
    json_t * value_lvl_2 = NULL;

    const char * str_val = NULL;


    printf("Parsing the input file...\n");

    ret = load_json_from_file(inputfile, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to mmap the file.\n");
        return ret;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        return NETLOC_ERROR;
    }


    /*
     * Process all of the network information in the file
     * Note: Use foreach loops instead of direct access that way we can warn
     *       the user of bad keys in the input file.
     */
    json_object_foreach(json, key_lvl_1, value_lvl_1) {
        if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NETWORK_INFO) ) {
            printf("\tProcessing: Network Information...\n");

            network->network_type = NETLOC_NETWORK_TYPE_INVALID;
            network->subnet_id = NULL;
            network->description = NULL;
            asprintf(&network->data_uri, "file://%s", outdir);

            json_object_foreach(value_lvl_1, key_lvl_2, value_lvl_2) {
                if( 0 == CMP_JSON_KEYS(key_lvl_2, JSON_KEY_NETWORK_INFO_NETWORK_TYPE) ) {
                    str_val = json_string_value(value_lvl_2);
                    //printf("\tNetwork Type: %s\n", str_val);
                    network->network_type = netloc_encode_network_type(str_val);
                }
                else if( 0 == CMP_JSON_KEYS(key_lvl_2, JSON_KEY_NETWORK_INFO_SUBNET_ID) ) {
                    str_val = json_string_value(value_lvl_2);
                    //printf("\tSubnet ID   : %s\n", str_val);
                    network->subnet_id = (NULL == str_val ? NULL : strdup(str_val));
                }
                else if( 0 == CMP_JSON_KEYS(key_lvl_2, JSON_KEY_NETWORK_INFO_VERSION) ) {
                    str_val = json_string_value(value_lvl_2);
                    //printf("\tVersion     : %s\n", str_val);
                    network->version = (NULL == str_val ? NULL : strdup(str_val));
                }
                else if( 0 == CMP_JSON_KEYS(key_lvl_2, JSON_KEY_NETWORK_INFO_DESCRIPTION) ) {
                    str_val = json_string_value(value_lvl_2);
                    //printf("\tDescription : %s\n", str_val);
                    network->description = (NULL == str_val ? NULL : strdup(str_val));
                }
                else {
                    printf("Warning: Unknown key \"%s\" in %s. Skipping.\n", key_lvl_2, JSON_KEY_NETWORK_INFO);
                }
            }

            // Handle some default values
            if( NULL == network->subnet_id ) {
                network->subnet_id = strdup(JSON_DEFAULT_NETWORK_INFO_SUBNET_ID);
            }
            if( NULL == network->version ) {
                network->version = strdup(JSON_DEFAULT_NETWORK_INFO_VERSION);
            }
            if( NULL == network->description ) {
                network->description = strdup(JSON_DEFAULT_NETWORK_INFO_DESCRIPTION);
            }

            // Setup a global parameter for filenaming later
            if( NULL == subnet ) {
                subnet = strdup(network->subnet_id);
            }
        }
        else if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NODE_INFO) ||
                 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_EDGE_INFO) ) {
            // Skip for now...
        }
        else {
            printf("Warning: Unknown key \"%s\". Skipping.\n", key_lvl_1);
        }
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

static int convert_nodes_file(netloc_data_collection_handle_t *dc_handle) {
    int ret = NETLOC_SUCCESS;

    json_t *json = NULL;

    const char * key_lvl_1 = NULL;
    json_t * value_lvl_1 = NULL;

    json_t * value_nodes = NULL;
    json_t * value_edges = NULL;
    size_t num_nodes = 0, num_edges = 0, index;
    json_t * value_lvl_2 = NULL;

    const char * key_lvl_3 = NULL;
    json_t * value_lvl_3 = NULL;

    const char * str_val = NULL;

    netloc_node_t *node = NULL;
    netloc_node_t *dst_node = NULL;
    netloc_edge_t *edge = NULL;
    netloc_edge_t *edge_bi = NULL;

    json_t *dir_value = NULL;
    bool dir_bi = false;

    ret = load_json_from_file(inputfile, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to mmap the file.\n");
        return ret;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        return NETLOC_ERROR;
    }


    /*
     * Process all of the node information in the file first
     * Note: Use foreach loops instead of direct access that way we can warn
     *       the user of bad keys in the input file.
     */
    printf("\tProcessing: Node Information...\n");
    json_object_foreach(json, key_lvl_1, value_lvl_1) {
        if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NETWORK_INFO) ) {
            // Skip
        }
        /*
         * Setup the node information
         */
        else if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NODE_INFO) ) {

            value_nodes = json_object_get( value_lvl_1, JSON_KEY_NODE_INFO_ARRAY );
            num_nodes = json_array_size(value_nodes);

            json_array_foreach(value_nodes, index, value_lvl_2) {
                if( progress ) {
                    printf("\t\tProcessing Node: %3d of %3d\n", (int)index, (int)num_nodes);
                }

                node = netloc_dt_node_t_construct();

                node->network_type = dc_handle->network->network_type;
                node->subnet_id    = strdup(dc_handle->network->subnet_id);

                json_object_foreach(value_lvl_2, key_lvl_3, value_lvl_3) {
                    if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_NODE_INFO_TYPE) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tNode Type  : %s\n", str_val);
                        node->node_type    = netloc_encode_node_type(str_val);
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_NODE_INFO_LOGICAL_ID) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tLogical ID : %s\n", str_val);
                        node->logical_id   = (NULL == str_val ? NULL : strdup(str_val));
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_NODE_INFO_PHYSICAL_ID) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tPhysical ID: %s\n", str_val);
                        node->physical_id  = (NULL == str_val ? NULL : strdup(str_val));
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_NODE_INFO_DESCRIPTION) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tDescription: %s\n", str_val);
                        node->description  = (NULL == str_val ? NULL : strdup(str_val));
                    }
                    else {
                        printf("Warning: Unknown key \"%s\" in %s. Skipping.\n", key_lvl_3, JSON_KEY_NODE_INFO);
                    }
                }

                if( NULL == node->logical_id ) {
                    node->logical_id = strdup(node->physical_id);
                }
                if( NULL == node->description ) {
                    node->description = strdup(JSON_DEFAULT_NODE_INFO_DESCRIPTION);
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
        }
        else if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_EDGE_INFO) ) {
            // Skip for now... just focus on node information
        }
        else {
            printf("Warning: Unknown key \"%s\". Skipping.\n", key_lvl_1);
        }
    }

    /*
     * Process all of the edge information in the file next - now that the nodes are in there
     * Note: Use foreach loops instead of direct access that way we can warn
     *       the user of bad keys in the input file.
     */
    printf("\tProcessing: Edge Information...\n");
    json_object_foreach(json, key_lvl_1, value_lvl_1) {
        if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NETWORK_INFO) ) {
            // Skip
        }
        else if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_NODE_INFO) ) {
            // Skip - already finished above
        }
        /*
         * Setup the edge information
         */
        else if( 0 == CMP_JSON_KEYS(key_lvl_1, JSON_KEY_EDGE_INFO) ) {

            value_edges = json_object_get( value_lvl_1, JSON_KEY_EDGE_INFO_ARRAY );
            num_edges = json_array_size(value_edges);

            json_array_foreach(value_edges, index, value_lvl_2) {
                // Access the source node
                str_val = json_string_value( json_object_get( value_lvl_2, JSON_KEY_EDGE_INFO_SRC_NODE_ID ));
                if( NULL == str_val ) {
                    printf("Could not find the %s key/value pair\n", JSON_KEY_EDGE_INFO_SRC_NODE_ID);
                    continue;
                }

                node = netloc_dc_get_node_by_physical_id(dc_handle, (char*)str_val);
                if( NULL == node ) {
                    printf("Error: Could not find the source node %s\n", str_val);
                    continue;
                }


                // Access the destination node
                str_val = json_string_value( json_object_get( value_lvl_2, JSON_KEY_EDGE_INFO_DST_NODE_ID ));
                if( NULL == str_val ) {
                    printf("Could not find the %s key/value pair\n", JSON_KEY_EDGE_INFO_DST_NODE_ID);
                    continue;
                }

                dst_node = netloc_dc_get_node_by_physical_id(dc_handle, (char*)str_val);
                if( NULL == node ) {
                    printf("Error: Could not find the destination node %s\n", str_val);
                    continue;
                }

                edge = netloc_dt_edge_t_construct();

                // Is this a bi-directional edge?
                dir_value = json_object_get( value_lvl_2, JSON_KEY_EDGE_INFO_DIRECTIONAL );
                if( NULL == dir_value ) {
                    dir_bi = JSON_DEFAULT_EDGE_INFO_DIRECTIONAL;
                }
                else if( JSON_TRUE == json_typeof(dir_value) ) {
                    dir_bi = true;
                }
                else {
                    dir_bi = JSON_DEFAULT_EDGE_INFO_DIRECTIONAL;
                }

                // If so, keep a mirrored edge
                if( dir_bi ) {
                    edge_bi = netloc_dt_edge_t_construct();
                } else {
                    edge_bi = NULL;
                }

                if( progress ) {
                    printf("\t\tProcessing Edge: %3d of %3d (%s)\n", (int)index, (int)num_edges, (edge_bi ? "Bidirectional" : "Unidirectional"));
                }

                // Save source/destination node information
                edge->src_node_id    = strdup(node->physical_id);
                edge->src_node_type  = node->node_type;
                edge->dest_node_id   = strdup(dst_node->physical_id);
                edge->dest_node_type = dst_node->node_type;
                if( dir_bi ) {
                    edge_bi->src_node_id    = strdup(dst_node->physical_id);
                    edge_bi->src_node_type  = dst_node->node_type;
                    edge_bi->dest_node_id   = strdup(node->physical_id);
                    edge_bi->dest_node_type = node->node_type;
                }

                json_object_foreach(value_lvl_2, key_lvl_3, value_lvl_3) {
                    if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_SRC_NODE_ID) ) {
                        // Handled above
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_DST_NODE_ID) ) {
                        // Handled above
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_DIRECTIONAL) ) {
                        // Handled above
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_SRC_PORT) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tSource Port  : %s\n", str_val);
                        edge->src_port_id = (NULL == str_val ? NULL : strdup(str_val));
                        if( dir_bi ) {
                            edge_bi->dest_port_id = (NULL == str_val ? NULL : strdup(str_val));
                        }
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_DST_PORT) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tDest. Port   : %s\n", str_val);
                        edge->dest_port_id = (NULL == str_val ? NULL : strdup(str_val));
                        if( dir_bi ) {
                            edge_bi->src_port_id = (NULL == str_val ? NULL : strdup(str_val));
                        }
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_WIDTH) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tWidth        : %s\n", str_val);
                        edge->width = (NULL == str_val ? NULL : strdup(str_val));
                        if( dir_bi ) {
                            edge_bi->width = (NULL == str_val ? NULL : strdup(str_val));
                        }
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_SPEED) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tSpeed        : %s\n", str_val);
                        edge->speed = (NULL == str_val ? NULL : strdup(str_val));
                        if( dir_bi ) {
                            edge_bi->speed = (NULL == str_val ? NULL : strdup(str_val));
                        }
                    }
                    else if( 0 == CMP_JSON_KEYS(key_lvl_3, JSON_KEY_EDGE_INFO_DESCRIPTION) ) {
                        str_val = json_string_value(value_lvl_3);
                        //printf("\t\tDescription  : %s\n", str_val);
                        edge->description = (NULL == str_val ? NULL : strdup(str_val));
                        if( dir_bi ) {
                            edge_bi->description = (NULL == str_val ? NULL : strdup(str_val));
                        }
                    }
                    else {
                        printf("Warning: Unknown key \"%s\" in %s. Skipping.\n", key_lvl_3, JSON_KEY_EDGE_INFO);
                    }
                    // XXX gbits TODO
                }

                // Update defaults
                if( NULL == edge->src_port_id ) {
                    edge->src_port_id = strdup( JSON_DEFAULT_EDGE_INFO_SRC_PORT );
                    if( dir_bi ) {
                        edge_bi->src_port_id = strdup(edge->src_port_id);
                    }
                }
                if( NULL == edge->dest_port_id ) {
                    edge->dest_port_id = strdup( JSON_DEFAULT_EDGE_INFO_DST_PORT );
                    if( dir_bi ) {
                        edge_bi->dest_port_id = strdup(edge->dest_port_id);
                    }
                }
                if( NULL == edge->width ) {
                    edge->width = strdup( JSON_DEFAULT_EDGE_INFO_WIDTH );
                    if( dir_bi ) {
                        edge_bi->width = strdup(edge->width);
                    }
                }
                if( NULL == edge->speed ) {
                    edge->speed = strdup( JSON_DEFAULT_EDGE_INFO_SPEED );
                    if( dir_bi ) {
                        edge_bi->speed = strdup(edge->speed);
                    }
                }
                if( NULL == edge->description ) {
                    edge->description = strdup(JSON_DEFAULT_EDGE_INFO_DESCRIPTION);
                    if( dir_bi ) {
                        edge_bi->description = strdup(edge->description);
                    }
                }
                // XXX gbits TODO

                // Append to the edges list
                ret = netloc_dc_append_edge_to_node(dc_handle, node, edge);
                if( NETLOC_SUCCESS != ret ) {
                    fprintf(stderr, "Error: Failed to append the edge to the node to the data collection\n");
                    netloc_dt_edge_t_destruct(edge);
                    return ret;
                }

                printf("\t\t%s\n", netloc_pretty_print_edge_t(edge));

                if( dir_bi ) {
                    ret = netloc_dc_append_edge_to_node(dc_handle, dst_node, edge_bi);
                    if( NETLOC_SUCCESS != ret ) {
                        fprintf(stderr, "Error: Failed to append the edge to the node to the data collection\n");
                        netloc_dt_edge_t_destruct(edge_bi);
                        return ret;
                    }

                    printf("\t\t%s\n", netloc_pretty_print_edge_t(edge_bi));
                }


                // The append_edge duplicates the edge, so we should free it
                netloc_dt_edge_t_destruct(edge);
                edge = NULL;
                if( dir_bi ) {
                    netloc_dt_edge_t_destruct(edge_bi);
                    edge_bi = NULL;
                }
            }
        }
        else {
            printf("Warning: Unknown key \"%s\". Skipping.\n", key_lvl_1);
        }
    }


    /*
     * Cleanup
     */
    if(NULL != json) {
        json_decref(json);
    }

    return NETLOC_SUCCESS;
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
                netloc_dt_lookup_table_iterator_t_destruct(hti_src);
                netloc_dt_lookup_table_iterator_t_destruct(hti_dst);
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
                netloc_dt_lookup_table_iterator_t_destruct(hti_src);
                netloc_dt_lookup_table_iterator_t_destruct(hti_dst);
                free(edges);
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
    if(NULL != nodes ) {
        netloc_lookup_table_destroy(nodes);
        free(nodes);
    }

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
    if( NULL != nodes ) {
        netloc_lookup_table_destroy(nodes);
        free(nodes);
    }

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
    if( NULL != network) {
        netloc_dt_network_t_destruct(network);
    }

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
    json_error_t error;

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
    (*json) = json_loads(memblock, 0, &error);
    if(NULL == (*json)) {
        fprintf(stderr, "Error: json_loads on mmaped file failed\n");
        fprintf(stderr, "\tLine %d, Column %d) %s\n", error.line, error.column, error.text);
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
