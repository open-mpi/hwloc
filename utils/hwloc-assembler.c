/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage(char *name, FILE *where)
{
  fprintf (where, "Usage: %s [options] <output>.xml <input1>.xml <input2>.xml ...\n", name);
  fprintf (where, "Options:\n");
  fprintf (where, "  -v --verbose   Show verbose messages\n");
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  char *callname;
  char *output;
  int verbose = 0;
  int opt;
  int i;

  callname = strrchr(argv[0], '/');
  if (!callname)
    callname = argv[0];
  else
    callname++;
  argc--;
  argv++;

  while (argc >= 1) {
    opt = 0;
    if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose")) {
      verbose++;
    } else if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    } else if (*argv[0] == '-') {
      fprintf(stderr, "Unrecognized option: %s\n", argv[0]);
      usage(callname, stderr);
      exit(EXIT_FAILURE);
    } else
      break;
    argc -= opt+1;
    argv += opt+1;
  }

  if (!argc) {
    fprintf(stderr, "Missing output file name\n");
    usage(callname, stderr);
    exit(EXIT_FAILURE);
  }
  output = argv[0];
  argc--;
  argv++;

  hwloc_topology_init(&topology);
  hwloc_topology_set_custom(topology);

  for(i=0; i<argc; i++) {
    hwloc_topology_t input;
    if (verbose)
      printf("Importing XML topology %s ...\n", argv[i]);
    hwloc_topology_init(&input);
    if (hwloc_topology_set_xml(input, argv[i])) {
      fprintf(stderr, "Failed to set source XML file %s (%s)\n", argv[i], strerror(errno));
      hwloc_topology_destroy(input);
      continue;
    }
    hwloc_topology_load(input);
    hwloc_topology_insert_topology(topology, hwloc_get_root_obj(topology), input);
    hwloc_topology_destroy(input);
  }

  if (verbose)
    printf("Assembling global topology...\n");
  hwloc_topology_load(topology);
  if (hwloc_topology_export_xml(topology, output) < 0) {
    fprintf(stderr, "Failed to export XML to %s (%s)\n", output, strerror(errno));
    return EXIT_FAILURE;
  }
  hwloc_topology_destroy(topology);
  if (verbose)
    printf("Exported topology to XML file %s\n", output);
  return 0;
}
