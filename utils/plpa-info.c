/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/*
 * The original plpa-info.c is
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2008 Cisco Systems, Inc.  All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <hwloc/plpa.h>

int main(int argc, char *argv[]) 
{
    hwloc_topology_t topology;
    hwloc_cpuset_t offline_processors;
    int i;
    int ret = 0;
    int need_help = 0;
    int show_topo = 0;
    int have_topo, num_sockets, max_socket_num, num_cores, max_core_id;
    int num_processors_online, max_processor_id_online;
    int num_processors_offline, max_processor_id_offline;
    int num_processors_total;
    int socket_id;
    hwloc_plpa_api_type_t api_probe;

    for (i = 1; i < argc; ++i) {
        if (0 == strcmp("--version", argv[i])) {
            printf("PLPA HWLOC version %s\n", PACKAGE_VERSION);
            exit(0);
        } else if (0 == strcmp("--help", argv[i])) {
            need_help = 1;
            ret = 0;
            break;
        } else if (0 == strcmp("--topo", argv[i])) {
            show_topo = 1;
        } else {
            printf("%s: unrecognized option: %s\n",
                   argv[0], argv[i]);
            need_help = 1;
            ret = 1;
        }
    }

    if (need_help) {
        printf("usage: %s [--version | --topo] [--help]\n", argv[0]);
        return ret;
    }

    ret = hwloc_plpa_init(&topology);
    if (ret) {
	fprintf(stderr, "plpa_init failed\n");
	exit(1);
    }

    /* Is affinity supported at all? */

    if (0 != hwloc_plpa_api_probe(topology, &api_probe)) {
        api_probe = HWLOC_PLPA_PROBE_NOT_SUPPORTED;
    }
    printf("Kernel affinity support: ");
    switch (api_probe) {
    case HWLOC_PLPA_PROBE_OK:
        printf("yes\n");
        break;
    case HWLOC_PLPA_PROBE_NOT_SUPPORTED:
        printf("no\n");
        break;
    }

    /* What about topology? */

    if (0 != hwloc_plpa_have_topology_information(topology, &have_topo)) {
        have_topo = 0;
    }
    printf("Kernel topology support: %s\n", have_topo ? "yes" : "no");
    if (0 != hwloc_plpa_get_socket_info(topology, &num_sockets, &max_socket_num)) {
        num_sockets = max_socket_num = -1;
    }
    printf("Number of processor sockets: ");
    if (have_topo && num_sockets >= 0) {
        printf("%d\n", num_sockets);
    } else {
        printf("unknown\n");
    }

    /* If asked, print the map */

    if (show_topo) {
        if (have_topo) {
            if (0 != hwloc_plpa_get_processor_data(topology, &num_processors_online, &max_processor_id_online)) {
                fprintf(stderr, "plpa_get_processor_data failed\n");
                exit(1);
            }

	    offline_processors = hwloc_topology_get_offline_cpuset(topology);
	    num_processors_offline = hwloc_cpuset_weight(offline_processors);
	    num_processors_total = num_processors_offline+num_processors_online;
	    max_processor_id_offline = hwloc_cpuset_last(offline_processors);
	    hwloc_cpuset_free(offline_processors);

            printf("Number of processors online: %d\n", num_processors_online);
            printf("Number of processors offline: %d (no topology information available)\n", 
                   num_processors_offline);

            /* Go through all the sockets */
            for (i = 0; i < num_sockets; ++i) {
                /* Turn the socket number into a Linux socket ID */
                if (0 != hwloc_plpa_get_socket_id(topology, i, &socket_id)) {
                    fprintf(stderr, "plpa_get_socket_id failed\n");
                    break;
                }
                /* Find out about the cores on that socket */
                if (0 != hwloc_plpa_get_core_info(topology, socket_id,
                                                  &num_cores, &max_core_id)) {
                    fprintf(stderr, "plpa_get_core_info failed\n");
                    break;
                }

                printf("Socket %d (ID %d): %d core%s (max core ID: %d)\n",
                       i, socket_id, num_cores, (1 == num_cores) ? "" : "s",
                       max_core_id);
            }
        } else {
            printf("Kernel topology not supported -- cannot show topology information\n");
            exit(1);
        }
    }

    hwloc_plpa_finalize(topology);

    return 0;
}
