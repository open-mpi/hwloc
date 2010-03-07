/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/private.h>
#include <hwloc-mask.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: hwloc-mask [options] <location> ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the hwloc-bind utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "  -l --logical\ttake logical object indexes (default)\n");
  fprintf(where, "  -p --physical\ttake physical object indexes\n");
  fprintf(where, "  --proclist\treport the list of processors' indexes in the CPU set\n");
  fprintf(where, "  --nodelist\treport the list of memory nodes' indexes near the CPU set\n");
  fprintf(where, "  -v\t\tverbose messages\n");
  fprintf(where, "  --version\treport version and exit\n");
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  unsigned depth;
  hwloc_cpuset_t set;
  int verbose = 0;
  int logical = 1;
  int nodelist = 0;
  int proclist = 0;
  char **orig_argv = argv;

  set = hwloc_cpuset_alloc();

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);

  while (argc >= 2) {
    if (*argv[1] == '-') {
      if (!strcmp(argv[1], "-v")) {
        verbose = 1;
        goto next;
      }
      if (!strcmp(argv[1], "--help")) {
	usage(stdout);
	return EXIT_SUCCESS;
      }
      if (!strcmp(argv[1], "--proclist")) {
	proclist = 1;
        goto next;
      }
      if (!strcmp(argv[1], "--nodelist")) {
	nodelist = 1;
        goto next;
      }
      if (!strcmp(argv[1], "--version")) {
        printf("%s %s\n", orig_argv[0], VERSION);
        exit(EXIT_SUCCESS);
      }
      if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--logical")) {
	logical = 1;
	goto next;
      }
      if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--physical")) {
	logical = 0;
	goto next;
      }
      usage(stderr);
      return EXIT_FAILURE;
    }

    if (hwloc_mask_process_arg(topology, depth, argv[1], logical, set, verbose) < 0) {
      if (verbose)
	fprintf(stderr, "ignored unrecognized argument %s\n", argv[1]);
    }

 next:
    argc--;
    argv++;
  }

  if (proclist) {
    hwloc_obj_t proc, prev = NULL;
    while ((proc = hwloc_get_next_obj_covering_cpuset_by_type(topology, set, HWLOC_OBJ_PROC, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%u", logical ? proc->logical_index : proc->os_index);
      prev = proc;
    }
    printf("\n");
  } else if (nodelist) {
    hwloc_obj_t node, prev = NULL;
    while ((node = hwloc_get_next_obj_covering_cpuset_by_type(topology, set, HWLOC_OBJ_NODE, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%u", logical ? node->logical_index : node->os_index);
      prev = node;
    }
    printf("\n");
  } else {
    char *string = NULL;
    hwloc_cpuset_asprintf(&string, set);
    printf("%s\n", string);
    free(string);
  }

  hwloc_topology_destroy(topology);

  hwloc_cpuset_free(set);

  return EXIT_SUCCESS;
}
