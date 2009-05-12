/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void usage(void)
{
  fprintf(stderr, "Usage: topomask [depth:index] ...\n");
  fprintf(stderr, "  depth may be machine, node, die, core, proc or a numeric depth\n");
}

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  int verbose = 0;

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  if (argc < 2) {
    usage();
    return EXIT_FAILURE;
  }

  while (argc >= 2) {
    char string[TOPO_CPUSET_STRING_LENGTH+1];
    topo_obj_t obj;
    unsigned depth;
    char *colon;
    unsigned index;

    if (!strcmp(argv[1], "-v")) {
      verbose = 1;
      goto next;
    }

    colon = strchr(argv[1], ':');
    if (!colon) {
      usage();
      return EXIT_FAILURE;
    }

    if (!strncmp(argv[1], "machine", 1))
      depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
    else if (!strncmp(argv[1], "node", 1))
      depth = topo_get_type_depth(topology, TOPO_OBJ_NODE);
    else if (!strncmp(argv[1], "die", 1))
      depth = topo_get_type_depth(topology, TOPO_OBJ_DIE);
    else if (!strncmp(argv[1], "core", 1))
      depth = topo_get_type_depth(topology, TOPO_OBJ_CORE);
    else if (!strncmp(argv[1], "proc", 1))
      depth = topo_get_type_depth(topology, TOPO_OBJ_PROC);
    else
      depth = atoi(argv[1]);

    index = atoi(colon+1);

    obj = topo_get_object(topology, depth, index);
    if (obj) {
      topo_object_cpuset_snprintf(string, sizeof(string), 1, &obj);
    } else {
      topo_cpuset_t empty;
      if (verbose)
        printf("object (%d,%d) does not exist\n", depth, index);
      topo_cpuset_zero(&empty);
      snprintf(string, sizeof(string), "%" TOPO_PRIxCPUSET, TOPO_CPUSET_PRINTF_VALUE(empty));
    }
    if (verbose)
      printf("object (%d,%d) has cpuset %s\n", depth, index, string);
    else
      printf("%s\n", string);

 next:
    argc--;
    argv++;
  }

  topo_topology_destroy(topology);

  return 0;
}
