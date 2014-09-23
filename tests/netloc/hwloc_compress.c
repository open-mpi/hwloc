//
// Copyright © 2013 Inria.  All rights reserved.
//
// Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
// See COPYING in top-level directory.
//
// $HEADER$
//

#include "netloc.h"
#include "netloc/map.h"

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

int main(int argc, char *argv[])
{
  netloc_map_t map;
  struct rusage rusage;
  int verbose __netloc_attribute_unused = 0;
  int err;

  argc--;
  argv++;
  if (argc >= 1) {
    if (!strcmp(argv[0], "--verbose")) {
      verbose = 1;
      argc--;
      argv++;
    }
  }

  /* map without any hwloc topology */
  err = netloc_map_create(&map);
  if (err) {
    fprintf(stderr, "Failed to create the map\n");
    return -1;
  }
  err = netloc_map_build(map, NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC);
  if (err) {
    fprintf(stderr, "Failed to build map data\n");
    return -1;
  }
  err = getrusage(RUSAGE_SELF, &rusage);
  printf("map WITHOUT hwloc topology: MAXRSS = %ld kB\n", rusage.ru_maxrss);
  err = netloc_map_destroy(map);

  /* map with compressed hwloc topologies */
  err = netloc_map_create(&map);
  if (err) {
    fprintf(stderr, "Failed to create the map\n");
    return -1;
  }
  err = netloc_map_load_hwloc_data(map, NETLOC_ABS_TOP_SRCDIR "../examples/avakas/hwloc");
  if (err) {
    fprintf(stderr, "Failed to load hwloc data\n");
    return -1;
  }
  err = netloc_map_build(map, NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC);
  if (err) {
    fprintf(stderr, "Failed to build map data\n");
    return -1;
  }
  err = getrusage(RUSAGE_SELF, &rusage);
  printf("map with COMPRESSED hwloc topologies: MAXRSS = %ld kB\n", rusage.ru_maxrss);
  err = netloc_map_destroy(map);

  /* map with noncompressed hwloc topologies */
  err = netloc_map_create(&map);
  if (err) {
    fprintf(stderr, "Failed to create the map\n");
    return -1;
  }
  err = netloc_map_load_hwloc_data(map, NETLOC_ABS_TOP_SRCDIR "../examples/avakas/hwloc");
  if (err) {
    fprintf(stderr, "Failed to load hwloc data\n");
    return -1;
  }
  err = netloc_map_build(map, 0);
  if (err) {
    fprintf(stderr, "Failed to build map data\n");
    return -1;
  }
  err = getrusage(RUSAGE_SELF, &rusage);
  printf("map with NON-COMPRESSED hwloc topologies: MAXRSS = %ld kB\n", rusage.ru_maxrss);
  err = netloc_map_destroy(map);

  return 0;
}
