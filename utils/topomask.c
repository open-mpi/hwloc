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
#include <private/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void usage(FILE *where)
{
  fprintf(where, "Usage: topomask [options] [string] ...\n");
  fprintf(where, "\n<string> may be <depth:index>\n");
  fprintf(where, "  -  <depth> may be system, machine, node, socket, core, proc or a numeric depth\n");
  fprintf(where, "  -  <index> may be\n");
  fprintf(where, "    .  X\tone object with index X\n");
  fprintf(where, "    .  X-Y\tall objects with index between X and Y\n");
  fprintf(where, "    .  X-\tall objects with index at least X\n");
  fprintf(where, "    .  X:N\tN objects starting with index X, possibly wrapping-around the end of the level\n");
  fprintf(where, "  -  several <depth:index> may be concatenated with `.' to select some specific children\n");
  fprintf(where, "\n<string> may also be a cpuset string\n");
  fprintf(where, "\nIf prefixed with `~', the given string will be cleared instead of added to the current cpuset\n");
  fprintf(where, "If prefixed with `x', the given string will be and'ed instead of added to the current cpuset\n");
  fprintf(where, "If prefixed with `^', the given string will be xor'ed instead of added to the current cpuset\n");
  fprintf(where, "\nString are processed in order, without priorities.\n");
  fprintf(where, "Compose multiple invokations for complex operations.\n");
  fprintf(where, "e.g. for (A|B)^(C|D), use: topomask A B ^$(topomask C D)\n");
  fprintf(where, "\nOptions:\n");
  fprintf(where, "  -v\tverbose\n");
  fprintf(where, "  --proclist\treport the list of processors in the CPU set\n");
  fprintf(where, "  --nodelist\treport the list of memory nodes near the CPU set\n");
}

int main(int argc, char *argv[])
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t set;
  int verbose = 0;
  int nodelist = 0;
  int proclist = 0;

  topo_cpuset_zero(&set);

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

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
      usage(stderr);
      return EXIT_FAILURE;
    }

    topomask_process_arg(topology, &topoinfo, argv[1], &set, verbose);

 next:
    argc--;
    argv++;
  }

  if (proclist) {
    topo_obj_t proc, prev = NULL;
    while ((proc = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_PROC, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%u", proc->os_index);
      prev = proc;
    }
    printf("\n");
  } else if (nodelist) {
    topo_obj_t node, prev = NULL;
    while ((node = topo_get_next_obj_above_cpuset(topology, &set, TOPO_OBJ_NODE, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%u", node->os_index);
      prev = node;
    }
    printf("\n");
  } else {
    char string[TOPO_CPUSET_STRING_LENGTH+1];
    topo_cpuset_snprintf(string, sizeof(string), &set);
    printf("%s\n", string);
  }

  topo_topology_destroy(topology);

  return EXIT_SUCCESS;
}
