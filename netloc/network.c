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
#include <private/netloc.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

/***********************************************************************
 * URI Support
 ***********************************************************************/
/**
 * URI types
 */
typedef enum {
    URI_FILE   = 1,
    URI_OTHER  = 2,
    URI_INVALID = 3,
} uri_type_t;

#define URI_PREFIX_FILE "file://"

static netloc_network_t *extract_network_info(char *filename);
static int extract_filename_from_uri(const char * uri, uri_type_t *type, char **str);
static int search_uri(const char * search_uri,
                      int (*func)(const netloc_network_t *network, void *funcdata),
                      void *funcdata,
                      int *num_networks,
                      netloc_network_t ***networks);


int netloc_network_copy(netloc_network_t *from, netloc_network_t *to)
{

    if( NULL == to || NULL == from ) {
        return NETLOC_ERROR;
    }

    to->network_type = from->network_type;

    if( NULL != to->subnet_id ) {
        free(to->subnet_id);
    }
    to->subnet_id    = STRDUP_IF_NOT_NULL(from->subnet_id);

    if( NULL != to->data_uri ) {
        free(to->data_uri);
    }
    to->data_uri     = STRDUP_IF_NOT_NULL(from->data_uri);

    if( NULL != to->node_uri ) {
        free(to->node_uri);
    }
    to->node_uri     = STRDUP_IF_NOT_NULL(from->node_uri);

    if( NULL != to->description ) {
        free(to->description);
    }
    to->description  = STRDUP_IF_NOT_NULL(from->description);

    if( NULL != to->version ) {
        free(to->version);
    }
    to->version  = STRDUP_IF_NOT_NULL(from->version);

    to->userdata     = from->userdata;

    return NETLOC_SUCCESS;
}

int netloc_network_compare(netloc_network_t *a, netloc_network_t *b)
{
    /* Check: Network Type */
    if(a->network_type != b->network_type) {
        return NETLOC_CMP_DIFF;
    }

    /* Check: Subnet ID */
    if( (NULL == a->subnet_id && NULL != b->subnet_id) ||
        (NULL != a->subnet_id && NULL == b->subnet_id) ) {
        return NETLOC_CMP_DIFF;
    }
    if( NULL != a->subnet_id && NULL != b->subnet_id ) {
        if( 0 != strncmp(a->subnet_id, b->subnet_id, strlen(a->subnet_id)) ) {
            return NETLOC_CMP_DIFF;
        }
    }

    /* Check: Metadata */
    if( (NULL == a->version && NULL != b->version) ||
        (NULL != a->version && NULL == b->version) ) {
        return NETLOC_CMP_SIMILAR;
    }
    if( NULL != a->version && NULL != b->version ) {
        if( 0 != strncmp(a->version, b->version, strlen(a->version)) ) {
            return NETLOC_CMP_SIMILAR;
        }
    }

    return NETLOC_CMP_SAME;
}

netloc_network_t * netloc_network_dup(netloc_network_t *orig)
{
    netloc_network_t *network = NULL;

    if( NULL == orig ) {
        return NULL;
    }

    network = netloc_network_construct();
    if( NULL == network ) {
        return NULL;
    }

    netloc_network_copy(orig, network);

    return network;
}

char * netloc_network_pretty_print(netloc_network_t* network)
{
    char * str     = NULL;
    const char * tmp_str = NULL;

    tmp_str = netloc_network_type_decode(network->network_type);

    if( NULL != network->version ) {
        asprintf(&str, "%s-%s (version %s)", tmp_str, network->subnet_id, network->version);
    } else {
        asprintf(&str, "%s-%s", tmp_str, network->subnet_id);
    }

    return str;
}

netloc_network_t * netloc_network_construct(void)
{
    netloc_network_t *network = NULL;

    network = (netloc_network_t*)malloc(sizeof(netloc_network_t));
    if( NULL == network ) {
        return NULL;
    }

    network->network_type = NETLOC_NETWORK_TYPE_INVALID;
    network->subnet_id    = NULL;
    network->data_uri     = NULL;
    network->node_uri     = NULL;
    network->description  = NULL;
    network->version      = NULL;
    network->userdata     = NULL;

    return network;
}

int netloc_network_destruct(netloc_network_t * network)
{
    if (NULL == network)
        return NETLOC_SUCCESS;

    if (NULL != network->subnet_id) {
        free(network->subnet_id);
        network->subnet_id = NULL;
    }

    if (NULL != network->data_uri) {
        free(network->data_uri);
        network->data_uri = NULL;
    }

    if (NULL != network->node_uri) {
        free(network->node_uri);
        network->node_uri = NULL;
    }

    if( NULL != network->description) {
        free(network->description);
        network->description = NULL;
    }

    if( NULL != network->version) {
        free(network->version);
        network->version = NULL;
    }

    if( NULL != network->userdata) {
        network->userdata = NULL;
    }

    free(network);

    return NETLOC_SUCCESS;
}

int netloc_network_find(const char * network_topo_uri,
        netloc_network_t *ref_network, int *num_networks,
        netloc_network_t ***networks)
{
    int ret;
    netloc_network_t **all_networks = NULL;
    int i, num_all_networks;
    int num_found = 0;

    /*
     * Find all of the network information at this URI
     */
    num_all_networks = 0;
    ret = search_uri(network_topo_uri, NULL, NULL, &num_all_networks, &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: Failed to search the uri: %s\n", network_topo_uri);
        goto cleanup;
    }

    /*
     * Compare the networks to see if they match
     */
    num_found = 0;
    for (i = 0; i < num_all_networks; ++i) {
        int match = 1;
        if (ref_network && NETLOC_NETWORK_TYPE_INVALID != ref_network->network_type ) {
            if(all_networks[i]->network_type != ref_network->network_type) {
                match = 0;
            }
        }
        if (ref_network && NULL != ref_network->subnet_id ) {
            if( 0 != strncmp(all_networks[i]->subnet_id, ref_network->subnet_id,
                        strlen(all_networks[i]->subnet_id)) ) {
                match = 0;
            }
        }

        if (!match) {
            netloc_network_destruct(all_networks[i]);

        } else {
            all_networks[num_found] = all_networks[i];
            ++num_found;
        }
    }

    *networks = (netloc_network_t **)
        realloc(all_networks, sizeof(all_networks[num_found]));
    *num_networks = num_found;

    return NETLOC_SUCCESS;

 cleanup:
    for(i = 0; i < num_all_networks; ++i) {
        netloc_network_destruct(all_networks[i]);
    }
    free(all_networks);

    return ret;
}

int netloc_network_foreach(const char * const * search_uris,
                             int num_uris,
                             int (*func)(const netloc_network_t *network, void *funcdata),
                             void *funcdata,
                             int *num_networks,
                             netloc_network_t ***networks)
{
    int i;

    *num_networks = 0;
    for(i = 0; i < num_uris; ++i ) {
        int ret = search_uri(search_uris[i], func, funcdata, num_networks, networks);
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
static netloc_network_t *extract_network_info(char *filename)
{
    netloc_network_t *network = NULL;

    network = netloc_network_construct();
    if( NULL == network ) {
        return NULL;
    }

    // TODO read from file
    network->network_type = (netloc_network_type_t)NETLOC_NETWORK_TYPE_INFINIBAND;
    char *subnet_id = strdup(filename+strlen(filename)-29);
    subnet_id[19] = '\0';
    network->subnet_id = subnet_id;
    network->version = strdup("12");

    return network;
}

/***********************************************************************
 *        Support Functions
 ***********************************************************************/
/**
 * Extract the filename and URI information from a URI string
 *
 * \param uri URI to process
 * \param type Type of the URI
 * \param str String following the URI type prefix
 *
 * Returns
 *   NETLOC_SUCCESS on success
 *   NETLOC_ERROR otherwise
 */
static int extract_filename_from_uri(const char * uri, uri_type_t *type, char **str)
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
    *str = strdup(&uri[strlen(URI_PREFIX_FILE)]);

    // Append a '/' if needed
    len = strlen(*str);
    if( (*str)[len-1] != '/' ) {
        char *new_str;
        new_str = (char *)realloc(*str, sizeof(char[len+2]));
        if (!new_str) {
            free(*str);
            return NETLOC_ERROR;
        }
        *str = new_str;
        (*str)[len] = '/';
        (*str)[len+1] = '\0';
    }

    return NETLOC_SUCCESS;
}

/**
 * Search a single URI for all networks.
 * Works exactly like netloc_foreach_network() when it is provided only one search_uri.
 */
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

    /*
     * Process the URI
     */
    ret = extract_filename_from_uri(search_uri, &uri_type, &uri_str);
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
        if( NULL == strstr(dir_entry->d_name, "-nodes.txt") ) {
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

        tmp_network->node_uri = strdup(filename);

        asprintf(&tmp_network->data_uri, "%s%s", URI_PREFIX_FILE, uri_str);

        all_num_networks += 1;
        netloc_network_t **old_all_networks = all_networks;
        all_networks = (netloc_network_t **)
            realloc(all_networks, sizeof(netloc_network_t *)
                    *all_num_networks);
        if( NULL == all_networks ) {
            fprintf(stderr, "Error: Failed to allocate space for %d networks\n", all_num_networks);

            closedir(dirp);
            dirp = NULL;
            netloc_network_destruct(tmp_network);
            free(uri_str);
            free(all_networks);
            free(filename);
            return NETLOC_ERROR;
        }
        all_networks[all_num_networks-1] = tmp_network;

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
                    netloc_network_destruct(all_networks[i]);
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
            netloc_network_destruct(all_networks[i]);
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


