/*
 * Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright © 2013      University of Wisconsin-La Crosse.
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

#ifndef NETLOC_SUPPORT_H
#define NETLOC_SUPPORT_H

#include <netloc.h>
#include <private/netloc.h>

/***********************************************************************
 * JSON File Format Keywords
 * nodes.ndat
 * [metadata: [timestamp, version]],
 * [network: [type, subnet, description],
 * [node_uid: [node_type, phy_id, log_id, description,
 *    [edge_uid: src_id, src_type, src_port, dest_id, dest_type, dest_port, speed, width
 *    TODO update
 ***********************************************************************/
#define JSON_NODE_FILE_META_TIMESTAMP "timestamp"
#define JSON_NODE_FILE_META_VERSION   "version"

#define JSON_NODE_FILE_NETWORK_INFO   "network_info"
#define JSON_NODE_FILE_NODE_INFO      "node_info"
#define JSON_NODE_FILE_EDGE_INFO      "edge_info"
#define JSON_NODE_FILE_PATH_INFO      "path_info"

#define JSON_NODE_FILE_NETWORK_TYPE   "network_type"
#define JSON_NODE_FILE_NODE_TYPE      "node_type"
#define JSON_NODE_FILE_SUBNET_ID      "subnet_id"
#define JSON_NODE_FILE_DESCRIPTION    "desc"
#define JSON_NODE_FILE_PHY_ID         "phy_id"
#define JSON_NODE_FILE_LOG_ID         "log_id"
#define JSON_NODE_FILE_EDGE_LIST      "edges"
#define JSON_NODE_FILE_EDGE_ID_LIST   "edge_ids"
#define JSON_NODE_FILE_LOG_PATH_LIST  "log_paths"

#define JSON_NODE_FILE_EDGE_UID      "euid"
#define JSON_NODE_FILE_SRC_ID        "src_node_id"
#define JSON_NODE_FILE_SRC_TYPE      "src_node_type"
#define JSON_NODE_FILE_SRC_PORT      "src_port_id"
#define JSON_NODE_FILE_DEST_ID       "dest_node_id"
#define JSON_NODE_FILE_DEST_TYPE     "dest_node_type"
#define JSON_NODE_FILE_DEST_PORT     "dest_port_id"
#define JSON_NODE_FILE_EDGE_WIDTH    "width"
#define JSON_NODE_FILE_EDGE_SPEED    "speed"
#define JSON_NODE_FILE_EDGE_GBITS    "gbits"


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


#define SUPPORT_CONVERT_ADDR_TO_INT(addr, type, v) {        \
    if( NETLOC_NETWORK_TYPE_ETHERNET == type ) {            \
        v = netloc_dt_convert_mac_str_to_int(addr);         \
    } else {                                                \
        v = netloc_dt_convert_guid_str_to_int(addr);        \
    }                                                       \
}

#define SUPPORT_CONVERT_ADDR_TO_STR(addr, type, v) {    \
    if( NETLOC_NETWORK_TYPE_ETHERNET == type ) {        \
        v = netloc_dt_convert_mac_int_to_str(addr);     \
    } else {                                            \
        v = netloc_dt_convert_guid_int_to_str(addr);    \
    }                                                   \
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
int support_extract_filename_from_uri(const char * uri, uri_type_t *type, char **str);

#endif /* NETLOC_SUPPORT_H */
