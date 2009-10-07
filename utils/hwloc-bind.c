/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc-mask.h>
#include <hwloc.h>
#include <private/private.h>

#include <unistd.h>
#include <errno.h>
#include <assert.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: topobind [options] <location> -- command ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the hwloc-mask utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "   --single\tbind on a single CPU to prevent migration\n");
  fprintf(where, "   --strict\trequire strict binding\n");
  fprintf(where, "   -v\t\tverbose messages\n");
  fprintf(where, "   --version\treport version and exit\n");
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  unsigned depth;
  hwloc_cpuset_t cpu_set; /* invalid until bind_cpus is set */
  int bind_cpus = 0;
  int single = 0;
  int verbose = 0;
  int flags = 0;
  int ret;
  char **orig_argv = argv;

  cpu_set = hwloc_cpuset_alloc();

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);

  /* skip argv[0], handle options */
  argv++;
  argc--;

  while (argc >= 1) {
    if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    }

    if (*argv[0] == '-') {
      if (!strcmp(argv[0], "-v")) {
	verbose = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--help")) {
	usage(stdout);
	return EXIT_SUCCESS;
      }
      else if (!strcmp(argv[0], "--single")) {
	single = 1;
	goto next;
      }
      else if (!strcmp(argv[0], "--strict")) {
	flags |= HWLOC_CPUBIND_STRICT;
	goto next;
      }
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", orig_argv[0], VERSION);
          exit(EXIT_SUCCESS);
      }

      usage(stderr);
      return EXIT_FAILURE;
    }

    hwloc_mask_process_arg(topology, depth, argv[0], cpu_set, verbose);
    bind_cpus = 1;

  next:
    argc--;
    argv++;
  }

  if (bind_cpus) {
    if (verbose) {
      char *s = hwloc_cpuset_printf_value(cpu_set);
      fprintf(stderr, "binding on cpu set %s\n", s);
      free(s);
    }
    if (single)
      hwloc_cpuset_singlify(cpu_set);
    ret = hwloc_set_cpubind(topology, cpu_set, flags);
    if (ret) {
      char *s = hwloc_cpuset_printf_value(cpu_set);
      fprintf(stderr, "hwloc_set_cpubind %s failed (errno %d %s)\n", s, errno, strerror(errno));
      free(s);
    }
  }

  free(cpu_set);

  hwloc_topology_destroy(topology);

  if (!argc)
    return EXIT_SUCCESS;

  ret = execvp(argv[0], argv);
  if (ret && verbose)
    perror("execvp");
  return EXIT_FAILURE;
}
