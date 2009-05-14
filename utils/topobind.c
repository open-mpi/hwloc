/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <topology.h>

#include <unistd.h>
#include <assert.h>

static void usage(void)
{
  fprintf(stderr, "Usage: topobind [options] <cpuset> -- command ...\n");
  fprintf(stderr, "  Options are:\n");
  fprintf(stderr, "   -v\tverbose messages\n");
}

int main(int argc, char *argv[])
{
  topo_cpuset_t cpu_set;
  int bind_cpus = 0;
  int verbose = 0;
  int ret;

  /* bind everywhere by default */
  /* FIXME bind on existing cpus only? */
  topo_cpuset_fill(&cpu_set);

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

      usage();
      return EXIT_FAILURE;
    }

    topo_cpuset_from_string(argv[0], &cpu_set);
    bind_cpus = 1;
    if (verbose)
      fprintf(stderr, "Will bind on cpu set %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(cpu_set));

  next:
    argc--;
    argv++;
  }

  if (bind_cpus) {
    ret = topo_set_cpubind(&cpu_set);
    if (ret && verbose)
      fprintf(stderr, "topo_set_cpubind failed\n");
  }

  ret = execvp(argv[0], argv);
  if (ret && verbose)
    perror("execvp");
  return EXIT_FAILURE;
}
