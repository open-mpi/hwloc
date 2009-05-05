/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

#define indent(output, i) \
  fprintf (output, "%*s", i, "");

static void
output_topology (topo_topology_t topology, topo_level_t l, topo_level_t parent, FILE *output, int i, int verbose_mode) {
  int x;
  const char * indexprefix = "#";
  const char * levelterm = "";

  if (!verbose_mode
      && parent && parent->arity == 1 && topo_cpuset_isequal(&l->cpuset, &parent->cpuset)) {
    /* in non-verbose mode, merge levels with their parent is they are exactly identical */
    fprintf(output, " + ");
  } else {
    if (parent)
      fprintf(output, "\n");
    indent (output, 2*i);
    i++;
  }
  topo_print_level (topology, l, output, verbose_mode, indexprefix, levelterm);
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], l, output, i, verbose_mode);
  }
}

void output_text(topo_topology_t topology, FILE *output, int verbose_mode)
{
  output_topology (topology, topo_get_machine_level(topology), NULL, output, 0, verbose_mode);
  fprintf(output, "\n");

  if (verbose_mode)
    {
      struct topo_topology_info info;
      int l;

      topo_topology_get_info(topology, &info);
      if (info.is_fake)
	fprintf (output, "Topology is fake\n");
      if (info.dmi_board_vendor)
	fprintf (output, "DMI board vendor: %s\n", info.dmi_board_vendor);
      if (info.dmi_board_name)
	fprintf (output, "DMI board name: %s\n", info.dmi_board_name);
      if (info.huge_page_size_kB)
	fprintf (output, "Huge page size: %ldkB\n", info.huge_page_size_kB);

      for (l = 0; l < TOPO_LEVEL_MAX; l++)
	{
	  int depth = topo_get_type_depth (topology, l);
	  indent(output, depth);
	  fprintf (output, "depth %d:\ttype #%d (%s)\n", depth, l, topo_level_string (l));
	}
    }
}
