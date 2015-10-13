/*
 * Copyright © 2013      University of Wisconsin-La Crosse.
 *                         All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _PERL_JSON_SUPPORT_H_
#define _PERL_JSON_SUPPORT_H_

/*
 * Note: These must match those keys defined in Perl_OF_support.pm.in
 */
#define PERL_JSON_NODE_NETWORK_TYPE   "network_type"
#define PERL_JSON_NODE_TYPE           "type"
#define PERL_JSON_NODE_LOGICAL_ID     "log_id"
#define PERL_JSON_NODE_PHYSICAL_ID    "phy_id"
#define PERL_JSON_NODE_SUBNET_ID      "subnet_id"
#define PERL_JSON_NODE_CONNECTIONS    "connections"
#define PERL_JSON_EDGE_DESCRIPTION    "description"

#define PERL_JSON_EDGE_PORT_FROM      "port_from"
#define PERL_JSON_EDGE_PORT_ID_FROM   "port_id_from"
#define PERL_JSON_EDGE_PORT_TYPE_FROM "port_type_from"
#define PERL_JSON_EDGE_WIDTH          "width"
#define PERL_JSON_EDGE_SPEED          "speed"
#define PERL_JSON_EDGE_GBITS          "gbits"
#define PERL_JSON_EDGE_PORT_TO        "port_to"
#define PERL_JSON_EDGE_PORT_ID_TO     "port_id_to"
#define PERL_JSON_EDGE_PORT_TYPE_TO   "port_type_to"
#define PERL_JSON_EDGE_DESCRIPTION    "description"


#endif /* _PERL_JSON_SUPPORT_H_ */
