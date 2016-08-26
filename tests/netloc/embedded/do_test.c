#include <hwloc.h>
#include <netloc.h>
#include <stdio.h>

/* The body of the test is in a separate .c file and a separate
   library, just to ensure that hwloc didn't force compilation with
   visibility flags enabled. */

int do_test_hwloc(void)
{
    mytest_hwloc_topology_t topology;
    unsigned depth;
    hwloc_bitmap_t cpu_set;

    /* Just call a bunch of functions to see if we can link and run */

    printf("*** Test 1: bitmap alloc\n");
    cpu_set = mytest_hwloc_bitmap_alloc();
    if (NULL == cpu_set) return 1;
    printf("*** Test 2: topology init\n");
    if (0 != mytest_hwloc_topology_init(&topology)) return 1;
    printf("*** Test 3: topology load\n");
    if (0 != mytest_hwloc_topology_load(topology)) return 1;
    printf("*** Test 4: topology get depth\n");
    depth = mytest_hwloc_topology_get_depth(topology);
    if (depth > 1000) return 1;
    printf("    Max depth: %u\n", depth);
    printf("*** Test 5: topology destroy\n");
    mytest_hwloc_topology_destroy(topology);
    printf("*** Test 6: bitmap free\n");
    mytest_hwloc_bitmap_free(cpu_set);

    return 0;
}

int do_test_netloc(void)
{
    // Netloc-specific tests
    char **search_uris = NULL;
    int num_uris = 1, ret;
    netloc_network_t *tmp_network = NULL;

    // Specify where to search for network data
    search_uris = (char**)malloc(sizeof(char*) * num_uris );
    search_uris[0] = strdup("file://data/netloc");

    // Find a specific InfiniBand network
    tmp_network = netloc_network_construct();
    tmp_network->network_type = NETLOC_NETWORK_TYPE_INFINIBAND;
    tmp_network->subnet_id    = strdup("fe80:0000:0000:0000");

    // Search for the specific network
    ret = netloc_find_network(search_uris[0], tmp_network);

    return 0;
}

int do_test(void)
{
    int ret;

    /* First: do hwloc, just to make sure the basics are covered */
    ret = do_test_hwloc();
    if (0 != ret) return ret;

    /* Now do netloc */
    return do_test_netloc();
}
