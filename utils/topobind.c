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

#include <topomask.h>
#include <topology.h>
#include <topology/private.h>

#include <unistd.h>
#include <errno.h>
#include <assert.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: topobind [options] <location> -- command ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the topomask utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "   --single\tbind on a single CPU to prevent migration\n");
  fprintf(where, "   --strict\trequire strict binding\n");
  fprintf(where, "   -v\t\tverbose messages\n");
}

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t cpu_set; /* invalid until bind_cpus is set */
  int bind_cpus = 0;
  int single = 0;
  int verbose = 0;
  int flags = 0;
  int ret;

  topo_cpuset_zero(&cpu_set);

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

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
	flags |= TOPO_CPUBIND_STRICT;
	goto next;
      }

      usage(stderr);
      return EXIT_FAILURE;
    }

    topomask_process_arg(topology, &topoinfo, argv[0], &cpu_set, verbose);
    bind_cpus = 1;

  next:
    argc--;
    argv++;
  }

  if (bind_cpus) {
    if (verbose)
      fprintf(stderr, "binding on cpu set %" TOPO_PRIxCPUSET "\n",
	      TOPO_CPUSET_PRINTF_VALUE(&cpu_set));
    if (single)
      topo_cpuset_singlify(&cpu_set);
    ret = topo_set_cpubind(topology, &cpu_set, flags);
    if (ret)
      fprintf(stderr, "topo_set_cpubind %"TOPO_PRIxCPUSET" failed (errno %d %s)\n", TOPO_CPUSET_PRINTF_VALUE(&cpu_set), errno, strerror(errno));
  }

  topo_topology_destroy(topology);

  if (!argc)
    return EXIT_SUCCESS;

  ret = execvp(argv[0], argv);
  if (ret && verbose)
    perror("execvp");
  return EXIT_FAILURE;
}
