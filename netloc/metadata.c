/*
 * Copyright © 2013      University of Wisconsin-La Crosse.
 *                         All rights reserved.
 * Copyright © 2014-2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#include <netloc.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "support.h"

/**
 * Extract network information from a file
 *
 * Caller is responsible for calling destructor on the returned value.
 *
 * \param filename The file to extract information from
 *
 * Returns
 *   NULL if no network information was found, or a problem with the file.
 *   A newly allocated network handle.
 */
static netloc_network_t *extract_network_info(char *filename);

/**
 * Search a single URI for all networks.
 * Works exactly like netloc_foreach_network() when it is provided only one search_uri.
 */
static int search_uri(const char * search_uri,
                      int (*func)(const netloc_network_t *network, void *funcdata),
                      void *funcdata,
                      int *num_networks,
                      netloc_network_t ***networks);


/*******************************************************************/

int netloc_find_network(const char * network_topo_uri, netloc_network_t* network)
{
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_network_t **all_networks = NULL;
    int i, num_networks;
    int num_found = 0;
    netloc_network_t *last_found = NULL;

    /*
     * Find all of the network information at this URI
     */
    num_networks = 0;
    ret = search_uri(network_topo_uri, NULL, NULL, &num_networks, &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to search the uri: %s\n", network_topo_uri);
        return ret;
    }

    /*
     * Compare the networks to see if they match
     */
    num_found = 0;
    for(i = 0; i < num_networks; ++i) {
        if( NETLOC_NETWORK_TYPE_INVALID != network->network_type ) {
            if(all_networks[i]->network_type != network->network_type) {
                continue;
            }
        }
        if( NULL != network->subnet_id ) {
            if( 0 != strncmp(all_networks[i]->subnet_id, network->subnet_id, strlen(all_networks[i]->subnet_id)) ) {
                continue;
            }
        }

        ++num_found;
        last_found = all_networks[i];
    }

    /*
     * Determine if we found too much or too little
     */
    if( num_found == 0 ) {
        exit_status = NETLOC_ERROR_EMPTY;
        goto cleanup;
    }

    if( num_found > 1 ) {
        exit_status = NETLOC_ERROR_MULTIPLE;
        goto cleanup;
    }

    /*
     * If we found exactly one then copy the information into the handle
     */
    netloc_dt_network_t_copy(last_found, network);

    /*
     * Cleanup
     */
 cleanup:
    for(i = 0; i < num_networks; ++i) {
        netloc_dt_network_t_destruct(all_networks[i]);
    }
    free(all_networks);

    return exit_status;
}

int netloc_foreach_network(const char * const * search_uris,
                             int num_uris,
                             int (*func)(const netloc_network_t *network, void *funcdata),
                             void *funcdata,
                             int *num_networks,
                             netloc_network_t ***networks)
{
    int i, ret;

    *num_networks = 0;
    for(i = 0; i < num_uris; ++i ) {
        ret = search_uri(search_uris[i], func, funcdata, num_networks, networks);
        if( NETLOC_SUCCESS != ret ) {
            fprintf(stderr, "Error: Processing networks at URI %s\n", search_uris[i]);
            return ret;
        }
    }

    return NETLOC_SUCCESS;
}

/*******************************************************************
 * Support Functionality
 *******************************************************************/
static netloc_network_t *extract_network_info(char *filename)
{
    netloc_network_t *network = NULL;

    network = netloc_dt_network_t_construct();
    if( NULL == network ) {
        return NULL;
    }

    // TODO read from file
    network->network_type = (netloc_network_type_t)NETLOC_NETWORK_TYPE_INFINIBAND;
    network->subnet_id = strdup("fe80:0000:0000:0000");
    network->version = strdup("12");

    return network;
}

static int search_uri(const char * search_uri,
                      int (*func)(const netloc_network_t *network, void *funcdata),
                      void *funcdata,
                      int *num_networks,
                      netloc_network_t ***networks)
{
    int ret, i;
    netloc_network_t *tmp_network = NULL;
    netloc_network_t **all_networks = NULL;
    int all_num_networks;
    char * uri_str = NULL;
    uri_type_t uri_type;

    char * filename = NULL;
    DIR *dirp = NULL;
    struct dirent *dir_entry = NULL;
    bool found;

    /*
     * Process the URI
     */
    ret = support_extract_filename_from_uri(search_uri, &uri_type, &uri_str);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Malformed URI <%s>.\n", search_uri);
        return ret;
    }

    if( URI_FILE != uri_type ) {
        fprintf(stderr, "Error: Unsupported protocol in URI <%s>.\n", search_uri);
        return NETLOC_ERROR;
    }

    /*
     * Search the directory
     */
    dirp = opendir(uri_str);
    if( NULL == dirp ) {
        fprintf(stderr, "Error: Cannot open the directory <%s>.\n", uri_str);
        free(uri_str);
        return NETLOC_ERROR_NOENT;
    }

    all_num_networks = 0;

    while( NULL != (dir_entry = readdir(dirp)) ) {
#ifdef _DIRENT_HAVE_D_TYPE
        /*
         * Skip directories if the filesystem returns a useful d_type.
         * Otherwise, continue and let the actual opening will fail later.
         */
        if( DT_DIR == dir_entry->d_type ) {
            continue;
        }
#endif

        /*
         * Skip if does not end in .txt extension
         */
        if( NULL == strstr(dir_entry->d_name, ".txt") ) {
            continue;
        }

        /*
         * Extract the network metadata from the file
         */
        asprintf(&filename, "%s%s", uri_str, dir_entry->d_name);
        tmp_network = extract_network_info(filename);
        if( NULL == tmp_network ) {
            fprintf(stderr, "Error: Failed to extract network data from the file %s\n", filename);
            continue;
        }

        /*
         * Determine file type: nodes or paths
         */
        if( NULL != strstr(filename, "nodes.txt") ) {
            tmp_network->node_uri = strdup(filename);
        }
        else if( NULL != strstr(filename, "phy-paths.txt") ) {
            tmp_network->phy_path_uri = strdup(filename);
        }
        else if( NULL != strstr(filename, "log-paths.txt") ) {
            tmp_network->path_uri = strdup(filename);
        }

        asprintf(&tmp_network->data_uri, "%s%s", URI_PREFIX_FILE, uri_str);

        /*
         * Have we seen this network before?
         */
        found = false;
        for( i = 0; i < all_num_networks; ++i ) {
            if( NETLOC_CMP_SAME == netloc_dt_network_t_compare(all_networks[i], tmp_network) ) {
                found = true;
                break;
            }
        }

        /*
         * Append only the unique networks
         */
        if( !found ) {
            all_num_networks += 1;
            all_networks = (netloc_network_t**)realloc(all_networks, sizeof(netloc_network_t*)*all_num_networks);
            if( NULL == all_networks ) {
                fprintf(stderr, "Error: Failed to allocate space for %d networks\n", all_num_networks);

                closedir(dirp);
                dirp = NULL;
                netloc_dt_network_t_destruct(tmp_network);
                free(uri_str);
                return NETLOC_ERROR;
            }
            all_networks[all_num_networks-1] = tmp_network;
        } else {
            // If not unique, then we may still need to copy over the filenames
            // Determine file type: nodes or paths
            if( NULL != strstr(filename, "nodes.ndat") ) {
                all_networks[i]->node_uri = strdup(filename);
            }
            else if( NULL != strstr(filename, "phy-paths.ndat") ) {
                all_networks[i]->phy_path_uri = strdup(filename);
            }
            else if( NULL != strstr(filename, "log-paths.ndat") ) {
                all_networks[i]->path_uri = strdup(filename);
            }
            netloc_dt_network_t_destruct(tmp_network);
        }

	free(filename);
    }

    /*
     * From those unique networks, ask the user if they should be included
     * in the final vector returned
     */
    for( i = 0; i < all_num_networks; ++i ) {
        ret = -1;
        /*
         * Call the user callback function to decide if we should
         * include this in the vector returned.
         */
        if( NULL != func ) {
            ret = func(all_networks[i], funcdata);
        }
        if( 0 != ret ) {
            // Note: we could be extending an existing networks array, so make sure not to clear that memory.
            (*num_networks)++;
            (*networks) = (netloc_network_t**)realloc((*networks), sizeof(netloc_network_t*)*(*num_networks));
            if( NULL == (*networks) ) {
                while( i < all_num_networks ) {
                    netloc_dt_network_t_destruct(all_networks[i]);
                    all_networks[i] = NULL;
                    ++i;
                }

                closedir(dirp);
                free(all_networks);
                free(uri_str);

                return NETLOC_ERROR;
            }
            (*networks)[(*num_networks)-1] = all_networks[i];
        }
        else {
            netloc_dt_network_t_destruct(all_networks[i]);
            all_networks[i] = NULL;
        }
    }

    /*
     * Cleanup
     */
    closedir(dirp);
    free(all_networks);
    free(uri_str);

    return NETLOC_SUCCESS;
}
