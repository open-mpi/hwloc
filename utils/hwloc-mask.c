/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc-mask.h>
#include <hwloc.h>
#include <private/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: hwloc-mask [options] [string] ...\n");
  fprintf(where, "\n<string> may be <depth:index>\n");
  fprintf(where, "  -  <depth> may be system, machine, node, socket, core, proc or a numeric depth\n");
  fprintf(where, "  -  <index> may be\n");
  fprintf(where, "    .  X\tone object with index X\n");
  fprintf(where, "    .  X-Y\tall objects with index between X and Y\n");
  fprintf(where, "    .  X-\tall objects with index at least X\n");
  fprintf(where, "    .  X:N\tN objects starting with index X, possibly wrapping-around the end of the level\n");
  fprintf(where, "    .  all\tall objects\n");
  fprintf(where, "    .  odd\tall objects with odd index\n");
  fprintf(where, "    .  even\tall objects with even index\n");
  fprintf(where, "  -  several <depth:index> may be concatenated with `.' to select some specific children\n");
  fprintf(where, "\n<string> may also be a cpuset string\n");
  fprintf(where, "\nIf prefixed with `~', the given string will be cleared instead of added to the current cpuset\n");
  fprintf(where, "If prefixed with `x', the given string will be and'ed instead of added to the current cpuset\n");
  fprintf(where, "If prefixed with `^', the given string will be xor'ed instead of added to the current cpuset\n");
  fprintf(where, "\nString are processed in order, without priorities.\n");
  fprintf(where, "Compose multiple invokations for complex operations.\n");
  fprintf(where, "e.g. for (A|B)^(C|D), use: hwloc-mask A B ^$(hwloc-mask C D)\n");
  fprintf(where, "\nOptions:\n");
  fprintf(where, "  -v\tverbose\n");
  fprintf(where, "  --proclist\treport the list of processors in the CPU set\n");
  fprintf(where, "  --nodelist\treport the list of memory nodes near the CPU set\n");
  fprintf(where, "  --version\treport version and exit\n");
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  unsigned depth;
  hwloc_cpuset_t set;
  int verbose = 0;
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
      usage(stderr);
      return EXIT_FAILURE;
    }

    hwloc_mask_process_arg(topology, depth, argv[1], set, verbose);

 next:
    argc--;
    argv++;
  }

  if (proclist) {
    hwloc_obj_t proc, prev = NULL;
    while ((proc = hwloc_get_next_obj_covering_cpuset_by_type(topology, set, HWLOC_OBJ_PROC, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%d", proc->os_index);
      prev = proc;
    }
    printf("\n");
  } else if (nodelist) {
    hwloc_obj_t node, prev = NULL;
    while ((node = hwloc_get_next_obj_covering_cpuset_by_type(topology, set, HWLOC_OBJ_NODE, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%d", node->os_index);
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

  free(set);

  return EXIT_SUCCESS;
}
