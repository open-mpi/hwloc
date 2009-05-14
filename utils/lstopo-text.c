/* Copyright 2009 INRIA, Université Bordeaux 1  */

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
  topo_print_object (topology, l, output, verbose_mode, indexprefix, "");
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], l, output, i, verbose_mode);
  }
}

void output_text(topo_topology_t topology, FILE *output, int verbose_mode)
{
  output_topology (topology, topo_get_machine_object(topology), NULL, output, 0, verbose_mode);
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

      for (l = TOPO_OBJ_MACHINE; l < TOPO_OBJ_TYPE_MAX; l++)
	{
	  int depth = topo_get_type_depth (topology, l);
	  if (depth == TOPO_TYPE_DEPTH_UNKNOWN) {
	    fprintf (output, "absent:\t\ttype #%u (%s)\n", l, topo_object_type_string (l));
	  } else if (depth == TOPO_TYPE_DEPTH_MULTIPLE) {
	    fprintf (output, "multiple:\ttype #%u (%s)\n", l, topo_object_type_string (l));
	  } else {
	    indent(output, depth);
	    fprintf (output, "depth %d:\ttype #%u (%s)\n", depth, l, topo_object_type_string (l));
	  }
	}
    }
}
