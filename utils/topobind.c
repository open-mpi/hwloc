/* 
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

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
  topo_cpuset_t cpu_set; /* invalid until bind_cpus is set */
  int bind_cpus = 0;
  int verbose = 0;
  int ret;

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
