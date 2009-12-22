/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    mytest_hwloc_topology_t topology;
    unsigned depth;
    hwloc_cpuset_t cpu_set;

    /* Just call a bunch of functions to see if we can link and run */

    cpu_set = mytest_hwloc_cpuset_alloc();
    mytest_hwloc_topology_init(&topology);
    mytest_hwloc_topology_load(topology);
    depth = mytest_hwloc_topology_get_depth(topology);
    printf("Max depth: %u\n", depth);
    mytest_hwloc_topology_destroy(topology);
    mytest_hwloc_cpuset_free(cpu_set);

    return 0;
}
