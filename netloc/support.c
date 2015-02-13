/*
 * Copyright © 2013-2014 University of Wisconsin-La Crosse.
 *                         All rights reserved.
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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * Decode an edge
 */
void * dc_decode_edge(const char * key, json_t* json_obj);

/**
 * Debugging function
 */
//void check_edge_data(struct netloc_dt_lookup_table *edges);

int support_extract_filename_from_uri(const char * uri, uri_type_t *type, char **str)
{
    size_t len;

    *type = URI_INVALID;

    // Sanity check
    if( strlen(uri) < strlen(URI_PREFIX_FILE) ) {
        fprintf(stderr, "Error: URI too short. Must start with %s (Provided: %s)\n", URI_PREFIX_FILE, uri);
        return NETLOC_ERROR;
    }

    // The file uri is the only uri we support at the moment
    if( 0 != strncmp(uri, URI_PREFIX_FILE, strlen(URI_PREFIX_FILE)) ) {
        fprintf(stderr, "Error: Unsupported URI specifier. Must start with %s (Provided: %s)\n", URI_PREFIX_FILE, uri);
        return NETLOC_ERROR;
    }
    *type = URI_FILE;

    // Strip of the file prefix
    (*str) = strdup(&uri[strlen(URI_PREFIX_FILE)]);

    // Append a '/' if needed
    len = strlen(*str);
    if( (*str)[len-1] != '/' ) {
        (*str) = (char *)realloc(*str, sizeof(char) * (len+2));
        (*str)[len] = '/';
        (*str)[len+1] = '\0';
    }

    return NETLOC_SUCCESS;
}

int support_load_json(struct netloc_topology * topology)
{
    int ret, exit_status = NETLOC_SUCCESS;
    int i;
    json_t *json = NULL;
    netloc_node_t *node = NULL;

    int cur_idx;
    json_t *json_path_list = NULL;
    json_t *json_path = NULL;
    json_t *json_node_list = NULL;
    json_t *json_node = NULL;
    json_t *json_edge_list = NULL;

    struct netloc_dt_lookup_table_iterator *hti = NULL;
    netloc_edge_t *cur_edge = NULL;

    const char * key = NULL;

    if( topology->nodes_loaded ) {
        return NETLOC_SUCCESS;
    }

    /*
     * Load the json object (nodes)
     */
    ret = support_load_json_from_file(topology->network->node_uri, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to load the node file %s\n", topology->network->node_uri);
        exit_status = ret;
        goto cleanup;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    /*
     * Read in the edges
     */
    json_edge_list = json_object_get(json, JSON_NODE_FILE_EDGE_INFO);
    topology->edges = netloc_dt_lookup_table_t_json_decode(json_edge_list, &dc_decode_edge);
    //netloc_lookup_table_pretty_print(topology->edges);
    //check_edge_data(topology->edges);

    /*
     * Read in the nodes
     */
    json_node_list = json_object_get(json, JSON_NODE_FILE_NODE_INFO);
    topology->num_nodes = (int) json_object_size(json_node_list);
    topology->nodes = (netloc_node_t**)malloc(sizeof(netloc_node_t*) * (topology->num_nodes));
    if( NULL == topology->nodes ) {
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    cur_idx = 0;
    json_object_foreach(json_node_list, key, json_node) {
        topology->nodes[cur_idx] = netloc_dt_node_t_json_decode(topology->edges, json_object_get(json_node_list, key) );
        ++cur_idx;
    }

    if(NULL != json) {
        json_decref(json);
        json = NULL;
    }

    /*
     * For each edge, find the correct pointer for the dest_node.
     * Note: the src_node is filled in during the creation of a node in the
     *       netloc_dt_node_t_json_decode() operation, above.
     */
    hti = netloc_dt_lookup_table_iterator_t_construct(topology->edges);
    while( !netloc_lookup_table_iterator_at_end(hti) ) {
        cur_edge = (netloc_edge_t*)netloc_lookup_table_iterator_next_entry(hti);
        if( NULL == cur_edge ) {
            break;
        }

        for(i = 0; i < topology->num_nodes; ++i ) {
            if( NULL != topology->nodes[i] ) {
                if( 0 == strncmp(cur_edge->dest_node_id,
                                 topology->nodes[i]->physical_id,
                                 strlen(cur_edge->dest_node_id)) ) {
                    cur_edge->dest_node = topology->nodes[i];
                    break;
                }
            }
        }
        if( NULL == cur_edge->dest_node ) {
            fprintf(stderr, "Error: Failed to find a node to match the following edge\n");
            char * tmp_str = NULL;
            tmp_str = netloc_pretty_print_edge_t(cur_edge);
            fprintf(stderr, "       %s\n", tmp_str);
            free(tmp_str);
        }
    }
    netloc_dt_lookup_table_iterator_t_destruct(hti);
    hti = NULL;

    // for each edge
    //   find the corresponding node, and point to it.

    /*
     * Load the json object (physical paths)
     */
    ret = support_load_json_from_file(topology->network->phy_path_uri, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to load the physical path file %s\n", topology->network->phy_path_uri);
        exit_status = ret;
        goto cleanup;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        exit_status = NETLOC_SUCCESS;
        goto cleanup;
    }

    /*
     * Read in the paths
     */
    json_path_list = json_object_get(json, JSON_NODE_FILE_PATH_INFO);

    cur_idx = 0;
    json_object_foreach(json_path_list, key, json_path) {
        node = NULL;
        for(i = 0; i < topology->num_nodes; ++i) {
            if( 0 == strncmp(topology->nodes[i]->physical_id, key, strlen(key) ) ) {
                node = topology->nodes[i];
                break;
            }
        }
        if( NULL == node ) {
            fprintf(stderr, "Error: Failed to find the node with physical ID %s for physical path\n", key);
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }

        if( NULL != node->physical_paths ) {
            netloc_lookup_table_destroy(node->physical_paths);
	    free(node->physical_paths);
            node->physical_paths = NULL;
        }
        node->physical_paths = netloc_dt_node_t_json_decode_paths(topology->edges, json_path);
        if( NULL == node->physical_paths ) {
            fprintf(stderr, "Error: Failed to decode the physical path for node\n");
            fprintf(stderr, "Error: Node: %s\n", netloc_pretty_print_node_t(node));
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }
        node->num_phy_paths  = netloc_lookup_table_size(node->physical_paths);
    }

    if(NULL != json) {
        json_decref(json);
        json = NULL;
    }


    /*
     * Load the json object (logical paths)
     */
    ret = support_load_json_from_file(topology->network->path_uri, &json);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to load the path file %s\n", topology->network->path_uri);
        exit_status = ret;
        goto cleanup;
    }

    if( !json_is_object(json) ) {
        fprintf(stderr, "Error: json handle is not a valid object\n");
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

    /*
     * Read in the paths
     */
    json_path_list = json_object_get(json, JSON_NODE_FILE_PATH_INFO);

    cur_idx = 0;
    json_object_foreach(json_path_list, key, json_path) {
        node = NULL;
        for(i = 0; i < topology->num_nodes; ++i) {
            if( 0 == strncmp(topology->nodes[i]->physical_id, key, strlen(key) ) ) {
                node = topology->nodes[i];
                break;
            }
        }
        if( NULL == node ) {
            fprintf(stderr, "Error: Failed to find the node with physical ID %s for logical path\n", key);
            return NETLOC_ERROR;
        }

        if( NULL != node->logical_paths ) {
            netloc_lookup_table_destroy(node->logical_paths);
            free(node->logical_paths);
            node->logical_paths = NULL;
        }
        node->logical_paths = netloc_dt_node_t_json_decode_paths(topology->edges, json_path);
        if( NULL == node->logical_paths ) {
            fprintf(stderr, "Error: Failed to decode the logical path for node\n");
            fprintf(stderr, "Error: Node: %s\n", netloc_pretty_print_node_t(node));
            exit_status = NETLOC_ERROR;
            goto cleanup;
        }
        node->num_log_paths = netloc_lookup_table_size(node->logical_paths);
    }


    topology->nodes_loaded = true;

    cleanup:
    if(NULL != json) {
        json_decref(json);
        json = NULL;
    }

    return exit_status;
}

int support_load_json_from_file(const char * fname, json_t **json)
{
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

void * dc_decode_edge(const char * key, json_t* json_obj)
{
    return netloc_dt_edge_t_json_decode(json_obj);
}
