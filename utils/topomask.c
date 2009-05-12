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
  fprintf(stderr, "  <depth> may be machine, node, die, core, proc or a numeric depth\n");
  fprintf(stderr, "  <index> may be a single index <N>, a dash-separated range <min-max>,\n");
  fprintf(stderr, "    or a beginning and amount of objects <begin:number> (with wrap-around)\n");
}

#define OBJECT_MAX LIBTOPO_NBMAXCPUS

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_obj_t objs[OBJECT_MAX];
  unsigned nobj = 0;
  int verbose = 0;
  char string[TOPO_CPUSET_STRING_LENGTH+1];

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  while (argc >= 2) {
    topo_obj_t obj;
    unsigned depth;
    char *colon;
    char *sep;
    unsigned first, wrap, amount;
    unsigned i,j;

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

    first = atoi(colon+1);
    amount = 1;
    wrap = 0;

    sep = strchr(colon+1, '-');
    if (sep) {
      amount = atoi(sep+1)-first+1;
    } else {
      sep = strchr(colon+1, ':');
      if (sep) {
        amount = atoi(sep+1);
	wrap = 1;
      }
    }

    for(i=first, j=0; j<amount; i++, j++) {
      if (wrap && i==topo_get_depth_nbitems(topology, depth))
	i = 0;

      obj = topo_get_object(topology, depth, i);
      if (obj) {
	if (nobj == OBJECT_MAX) {
	  if (verbose)
	    fprintf(stderr, "Cannot work on more than %d objects, ignoring the other ones\n", OBJECT_MAX);
	  break;
	}
	objs[nobj++] = obj;
      }
      if (verbose) {
	if (obj)
          printf("object (%d,%d) found\n", depth, i);
	else
          printf("object (%d,%d) does not exist\n", depth, i);
      }
    }

 next:
    argc--;
    argv++;
  }

  topo_object_cpuset_snprintf(string, sizeof(string), nobj, objs);
  printf("%s\n", string);

  topo_topology_destroy(topology);

  return 0;
}
