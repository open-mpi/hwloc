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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

#define indent(output, i) \
  fprintf (output, "%*s", i, "");

static void
output_topology (topo_topology_t topology, topo_obj_t l, topo_obj_t parent, FILE *output, int i, int verbose_mode) {
  int x;
  const char * indexprefix = "#";
  char line[256];

  if (!verbose_mode
      && parent && parent->arity == 1 && topo_cpuset_isequal(&l->cpuset, &parent->cpuset)) {
    /* in non-verbose mode, merge objects with their parent is they are exactly identical */
    fprintf(output, " + ");
  } else {
    if (parent)
      fprintf(output, "\n");
    indent (output, 2*i);
    i++;
  }
  topo_obj_snprintf (line, sizeof(line), topology, l, indexprefix, 1);
  fprintf(output, line);
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], l, output, i, verbose_mode);
  }
}

void output_text(topo_topology_t topology, FILE *output, int verbose_mode)
{
  output_topology (topology, topo_get_system_obj(topology), NULL, output, 0, verbose_mode);
  fprintf(output, "\n");

  if (verbose_mode)
    {
      struct topo_topology_info info;
      enum topo_obj_type_e l;

      topo_topology_get_info(topology, &info);
      if (info.is_fake)
	fprintf (output, "Topology is fake\n");
      if (info.dmi_board_vendor)
	fprintf (output, "DMI board vendor: %s\n", info.dmi_board_vendor);
      if (info.dmi_board_name)
	fprintf (output, "DMI board name: %s\n", info.dmi_board_name);
      if (info.huge_page_size_kB)
	fprintf (output, "Huge page size: %lukB\n", info.huge_page_size_kB);

      for (l = TOPO_OBJ_SYSTEM; l < TOPO_OBJ_FAKE; l++)
	{
	  int depth = topo_get_type_depth (topology, l);
	  if (depth == TOPO_TYPE_DEPTH_UNKNOWN) {
	    fprintf (output, "absent:\t\ttype #%u (%s)\n", l, topo_obj_type_string (l));
	  } else if (depth == TOPO_TYPE_DEPTH_MULTIPLE) {
	    fprintf (output, "multiple:\ttype #%u (%s)\n", l, topo_obj_type_string (l));
	  } else {
	    indent(output, depth);
	    fprintf (output, "depth %d:\ttype #%u (%s)\n", depth, l, topo_obj_type_string (l));
	  }
	}
    }
}
