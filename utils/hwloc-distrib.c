/*
 * Copyright © 2009, 2010 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/private.h>
#include <hwloc.h>

#include "misc.h"

#include <unistd.h>

void usage(const char *callname, FILE *where)
{
  fprintf(where, "Usage: hwloc-distrib [options] number\n");
  fprintf(where, "Options:\n");
  fprintf(where, "   --single\tsinglify each output to a single CPU\n");
  fprintf(where, "   --taskset\tShow taskset-specific cpuset strings\n");
  fprintf(where, "   -v\t\t\tverbose messages\n");
  hwloc_utils_input_format_usage(where);
  fprintf(where, "   --version\t\treport version and exit\n");
}

int main(int argc, char *argv[])
{
  long n = -1;
  char *callname;
  char *input = NULL;
  enum hwloc_utils_input_format input_format = HWLOC_UTILS_INPUT_DEFAULT;
  int taskset = 0;
  int singlify = 0;
  int verbose = 0;
  char **orig_argv = argv;
  int opt;

  /* skip argv[0], handle options */
  callname = argv[0];
  argv++;
  argc--;

  while (argc >= 1) {
    if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    }

    if (*argv[0] == '-') {
      if (!strcmp(argv[0], "--single")) {
	singlify = 1;
	goto next;
      }
      if (!strcmp(argv[0], "--taskset")) {
	taskset = 1;
	goto next;
      }
      if (!strcmp(argv[0], "-v")) {
	verbose = 1;
	goto next;
      }
      if (!strcmp(argv[0], "--help")) {
	usage(callname, stdout);
	return EXIT_SUCCESS;
      }
      if (hwloc_utils_lookup_input_option(argv, argc, &opt,
					  &input, &input_format,
					  callname)) {
	argv += opt;
	argc -= opt;
	goto next;
      }
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", orig_argv[0], VERSION);
          exit(EXIT_SUCCESS);
      }

      usage(callname, stderr);
      return EXIT_FAILURE;
    }

    if (n != -1) {
      fprintf(stderr,"duplicate number\n");
      usage(callname, stderr);
      return EXIT_FAILURE;
    }
    n = atol(argv[0]);

  next:
    argc--;
    argv++;
  }

  if (n == -1) {
    fprintf(stderr,"need a number\n");
    usage(callname, stderr);
    return EXIT_FAILURE;
  }

  if (verbose)
    fprintf(stderr, "distributing %ld\n", n);

  {
    long i;
    hwloc_cpuset_t cpuset[n];
    hwloc_topology_t topology;

    hwloc_topology_init(&topology);
    if (input)
      hwloc_utils_enable_input_format(topology, input, input_format, callname);
    hwloc_topology_load(topology);

    hwloc_distribute(topology, hwloc_get_root_obj(topology), cpuset, n);
    for (i = 0; i < n; i++) {
      char *str = NULL;
      if (singlify)
	hwloc_cpuset_singlify(cpuset[i]);
      if (taskset)
	hwloc_cpuset_taskset_asprintf(&str, cpuset[i]);
      else
	hwloc_cpuset_asprintf(&str, cpuset[i]);
      printf("%s\n", str);
      free(str);
      hwloc_cpuset_free(cpuset[i]);
    }
    hwloc_topology_destroy(topology);
  }

  return EXIT_SUCCESS;
}
