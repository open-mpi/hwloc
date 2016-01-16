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
#include "private/netloc.h"

#include <stdlib.h>

int main(void) {
    int exit_status = NETLOC_SUCCESS;
    unsigned long value;
    char * tmp_str = NULL;

    char * mac_addr = NULL;
    char * guid_addr = NULL;

    mac_addr = strdup("F8:66:F2:22:EF:A2");
    guid_addr = strdup("78E7:D103:0021:4A85");

    /*
     * Test Mac: str -> int
     */
    printf("Test MAC str <-> int: ");
    fflush(NULL);

    value = netloc_dt_convert_mac_str_to_int(mac_addr);
    tmp_str = netloc_dt_convert_mac_int_to_str(value);
    //printf("\tDEBUG: (%s) =\t%lu =\t (%s)\n", mac_addr, value, tmp_str);
    if( NULL == tmp_str || 0 != strncmp(mac_addr, tmp_str, strlen(mac_addr) ) ) {
        fprintf(stderr, "Error: Invalid Conversion (%s) =\t%lu =\t (%s)\n", mac_addr, value, tmp_str);
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }
    printf("Success\n");

    free(tmp_str);

    /*
     * Test GUID: str -> int
     */
    printf("Test GUID str <-> int: ");
    fflush(NULL);

    value = netloc_dt_convert_guid_str_to_int(guid_addr);
    tmp_str = netloc_dt_convert_guid_int_to_str(value);
    //printf("\tDEBUG: (%s) =\t%lu =\t (%s)\n", guid_addr, value, tmp_str);
    if( NULL == tmp_str || 0 != strncmp(guid_addr, tmp_str, strlen(guid_addr) ) ) {
        fprintf(stderr, "Error: Invalid Conversion (%s) =\t%lu =\t (%s)\n", guid_addr, value, tmp_str);
        exit_status = NETLOC_ERROR;
        goto cleanup;
    }
    printf("Success\n");



 cleanup:
    /*
     * Cleanup
     */
    free(mac_addr);
    free(guid_addr);
    free(tmp_str);
    return exit_status;
}
