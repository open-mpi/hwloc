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

#include "netloc.h"

#include <stdlib.h>

/*
 * 0 = off, 1 = on
 */
#define DEBUG 0

/*
 * Testing support functions
 */
int netloc_callback_all_fn(const netloc_network_t *network, void *funcdata);
int netloc_callback_IB_fn(const netloc_network_t *network, void *funcdata);

int test_find_all_networks();
int test_find_all_networks_no_cb();
int test_find_all_networks_IB_cb();

int test_find_specific_IB_network();
int test_find_specific_ETH_network();
int test_find_specific_error_network();
int test_find_specific_error_multi_IB_network();
int test_find_specific_error_multi_network();

/*
 * Global variables
 */
int global_counter;

char **search_uris = NULL;
int num_uris = 1;


int main(void) {
    int i;

    search_uris = (char**)malloc(sizeof(char*) * num_uris );
    search_uris[0] = strdup("file://data/netloc");

    /*
     * Find all networks
     */
    printf("Checking All Networks: ");
    fflush(NULL);
    if( NETLOC_SUCCESS != test_find_all_networks() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find All networks without a callback
     */
    printf("Checking All Networks (no callback): ");
    fflush(NULL);
    if( NETLOC_SUCCESS != test_find_all_networks_no_cb() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find IB networks
     */
    printf("Checking IB Networks: ");
    fflush(NULL);
    if( NETLOC_SUCCESS != test_find_all_networks_IB_cb() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find a specific network
     */
    printf("Find a specific IB Network: ");
    fflush(NULL);
    if( NETLOC_SUCCESS != test_find_specific_IB_network() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find a specific network
     */
    printf("Find a specific Ethernet Network: ");
    fflush(NULL);
    if( NETLOC_SUCCESS != test_find_specific_ETH_network() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find an unknown network
     */
    printf("Find a specific Network (unknown networks): ");
    fflush(NULL);
    if( NETLOC_ERROR_EMPTY != test_find_specific_error_network() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");


    /*
     * Find an multiple matching network (IB)
     */
    /*
    printf("Find a specific Network (multiple IB networks): ");
    fflush(NULL);
    if( NETLOC_ERROR_MULTIPLE != test_find_specific_error_multi_IB_network() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");
    */

    /*
     * Find an multiple matching network
     */
    printf("Find a specific Network (multiple networks): ");
    fflush(NULL);
    if( NETLOC_ERROR_MULTIPLE != test_find_specific_error_multi_network() ) {
        return NETLOC_ERROR;
    }
    printf("Success\n");

    if( NULL != search_uris ) {
        for(i = 0; i < num_uris; ++i) {
            free(search_uris[i]);
            search_uris[i] = NULL;
        }
        free(search_uris);
        search_uris = NULL;
    }

    return NETLOC_SUCCESS;
}

int netloc_callback_all_fn(const netloc_network_t *network, void *funcdata) {
    //int *fdata = (int*)funcdata;
    ++global_counter;
    return -1;
}

int netloc_callback_IB_fn(const netloc_network_t *network, void *funcdata) {
    //int *fdata = (int*)funcdata;

    if( NETLOC_NETWORK_TYPE_INFINIBAND == network->network_type ) {
        ++global_counter;
        return -1;
    }
    else {
        return 0;
    }
}

int test_find_all_networks()
{
    int ret, exit_status = NETLOC_SUCCESS;
    int i;
    int funcdata = 1234;
    int num_all_networks = 0;
    netloc_network_t **all_networks = NULL;

    global_counter = 0;
    ret = netloc_foreach_network((const char * const *) search_uris,
                                   num_uris,
                                   &netloc_callback_all_fn,
                                   (void*)&funcdata,
                                   &num_all_networks,
                                   &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_foreach_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    if( num_all_networks != global_counter ) {
        fprintf(stderr, "Error: Callback not called enough times. %d but expected %d\n", num_all_networks, global_counter);
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

#if DEBUG == 1
    for(i = 0; i < num_all_networks; ++i ) {
        printf("\tIncluded Network: %s\n", netloc_network_pretty_print(all_networks[i]) );
    }
#endif

 cleanup:
    if( NULL != all_networks ) {
        for(i = 0; i < num_all_networks; ++i ) {
            netloc_network_destruct(all_networks[i]);
            all_networks[i] = NULL;
        }
        free(all_networks);
        all_networks = NULL;
    }

    return exit_status;
}

int test_find_all_networks_no_cb()
{
    int ret, exit_status = NETLOC_SUCCESS;
    int i;
    int num_all_networks = 0;
    netloc_network_t **all_networks = NULL;

    global_counter = 0;
    ret = netloc_foreach_network((const char * const *) search_uris,
                                   num_uris,
                                   NULL,
                                   NULL,
                                   &num_all_networks,
                                   &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_foreach_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    if( num_all_networks == global_counter ) {
        fprintf(stderr, "Error: Callback called. %d but expected 0\n", global_counter);
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

#if DEBUG == 1
    for(i = 0; i < num_all_networks; ++i ) {
        printf("\tIncluded Network: %s\n", netloc_network_pretty_print(all_networks[i]) );
    }
#endif

 cleanup:
    if( NULL != all_networks ) {
        for(i = 0; i < num_all_networks; ++i ) {
            netloc_network_destruct(all_networks[i]);
            all_networks[i] = NULL;
        }
        free(all_networks);
        all_networks = NULL;
    }

    return exit_status;
}

int test_find_all_networks_IB_cb()
{
    int ret, exit_status = NETLOC_SUCCESS;
    int funcdata = 1234;
    int i;
    int num_all_networks = 0;
    netloc_network_t **all_networks = NULL;

    global_counter = 0;
    ret = netloc_foreach_network((const char * const *) search_uris,
                                   num_uris,
                                   &netloc_callback_IB_fn,
                                   (void*)&funcdata,
                                   &num_all_networks,
                                   &all_networks);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_foreach_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    if( num_all_networks != global_counter ) {
        fprintf(stderr, "Error: Callback not called enough times. %d but expected %d\n", num_all_networks, global_counter);
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

#if DEBUG == 1
    for(i = 0; i < num_all_networks; ++i ) {
        printf("\tIncluded Network: %s\n", netloc_network_pretty_print(all_networks[i]) );
    }
#endif

 cleanup:
    if( NULL != all_networks ) {
        for(i = 0; i < num_all_networks; ++i ) {
            netloc_network_destruct(all_networks[i]);
            all_networks[i] = NULL;
        }
        free(all_networks);
        all_networks = NULL;
    }

    return exit_status;
}


int test_find_specific_IB_network()
{
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_network_t *tmp_network = NULL;

    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    tmp_network->subnet_id    = strdup("fe80:0000:0000:0000");

    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    if( strlen(tmp_network->subnet_id) <= 0 ) {
        printf("Error: Subnet not filled in. <%s>\n", netloc_network_pretty_print(tmp_network) );
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

#if DEBUG == 1
    printf("\tFound Network: %s\n", netloc_network_pretty_print(tmp_network));
#endif

 cleanup:
    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return exit_status;
}

int test_find_specific_ETH_network()
{
    int ret, exit_status = NETLOC_SUCCESS;
    netloc_network_t *tmp_network = NULL;

    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_ETHERNET;

    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
        exit_status = ret;
        goto cleanup;
    }

    if( strlen(tmp_network->subnet_id) <= 0 ) {
        printf("Error: Subnet not filled in. <%s>\n", netloc_network_pretty_print(tmp_network) );
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }

#if DEBUG == 1
    printf("\tFound Network: %s\n", netloc_network_pretty_print(tmp_network));
#endif

 cleanup:
    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return exit_status;
}

int test_find_specific_error_network()
{
    int ret;
    netloc_network_t *tmp_network = NULL;

    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    tmp_network->subnet_id    = strdup("12345");

    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_ERROR_EMPTY == ret ) {
        ;
    }
    else if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
    }
    else {
        fprintf(stderr, "Error: netloc_find_network returned success!\n");
    }

    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return ret;
}

int test_find_specific_error_multi_IB_network()
{
    int ret;
    netloc_network_t *tmp_network = NULL;

    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    //tmp_network->subnet_id    = strdup("12345");

    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_ERROR_MULTIPLE == ret ) {
        ;
    }
    else if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
    }
    else {
        fprintf(stderr, "Error: netloc_find_network returned success!\n");
    }

    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return ret;
}

int test_find_specific_error_multi_network()
{
    int ret;
    netloc_network_t *tmp_network = NULL;

    tmp_network = netloc_network_construct();
    //tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    //tmp_network->subnet_id    = strdup("12345");

    ret = netloc_find_network(search_uris[0], tmp_network);
    if( NETLOC_ERROR_MULTIPLE == ret ) {
        ;
    }
    else if( NETLOC_SUCCESS != ret ) {
        fprintf(stderr, "Error: netloc_find_network returned an error (%d)\n", ret);
    }
    else {
        fprintf(stderr, "Error: netloc_find_network returned success!\n");
    }

    netloc_network_destruct(tmp_network);
    tmp_network = NULL;

    return ret;
}
