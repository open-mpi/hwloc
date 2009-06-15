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
#include <topology/private.h>

#include <unistd.h>
#include <assert.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: topodistrib [options] number\n");
  fprintf(where, "Options:\n");
  fprintf(where, "   -v\t\t\tverbose messages\n");
  fprintf(where, "   --synthetic \"2 2\"\tsimulate a fake hierarchy\n");
#ifdef HAVE_XML
  fprintf(where, "   --xml <path>\t\tread topology from XML file <path>\n");
#endif
}

int main(int argc, char *argv[])
{
  long n = -1;
  char * synthetic = NULL;
  char * xmlpath = NULL;
  int verbose = 0;

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
      if (!strcmp(argv[0], "--help")) {
	usage(stdout);
	return EXIT_SUCCESS;
      }
      if (!strcmp (argv[0], "--synthetic")) {
	if (argc <= 2) {
	  usage(stdout);
	  exit(EXIT_FAILURE);
	}
	synthetic = argv[1];
	argv++;
	argc--;
	goto next;
      }
#ifdef HAVE_XML
      if (!strcmp (argv[0], "--xml")) {
	if (argc <= 2) {
	  usage(stdout);
	  exit(EXIT_FAILURE);
	}
	xmlpath = argv[1];
	argc--;
	argv++;
	if (!strcmp(xmlpath, "-"))
	  xmlpath = "/dev/stdin";
	goto next;
      }
#endif /* HAVE_XML */

      usage(stderr);
      return EXIT_FAILURE;
    }

    if (n != -1) {
      fprintf(stderr,"duplicate number\n");
      usage(stderr);
      return EXIT_FAILURE;
    }
    n = atol(argv[0]);

  next:
    argc--;
    argv++;
  }

  if (n == -1) {
    fprintf(stderr,"need a number\n");
    usage(stderr);
    return EXIT_FAILURE;
  }

  if (verbose)
    fprintf(stderr, "distributing %ld\n", n);

  {
    long i;
    topo_cpuset_t cpuset[n];
    char str[TOPO_CPUSET_STRING_LENGTH + 1];
    topo_topology_t topology;

    topo_topology_init(&topology);
    if (synthetic)
      topo_topology_set_synthetic(topology, synthetic);
    if (xmlpath)
      topo_topology_set_xml(topology, xmlpath);
    topo_topology_load(topology);

    topo_distribute(topology, topo_get_system_obj(topology), cpuset, n);
    for (i = 0; i < n; i++) {
      topo_cpuset_snprintf(str, sizeof(str), &cpuset[i]);
      printf("%s\n", str);
    }
    topo_topology_destroy(topology);
  }

  return EXIT_SUCCESS;
}
