/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

#define indent(output, i) \
  fprintf (output, "%*s", i, "");

static void
output_topology (topo_topology_t topology, lt_level_t l, FILE *output, int i, int verbose_mode) {
  int x;
  const char * separator = " ";
  const char * indexprefix = "#";
  const char * labelseparator = ":";
  const char * levelterm = "";

  indent (output, 2*i);
  lt_print_level (topology, l, output, verbose_mode, separator, indexprefix, labelseparator, levelterm);
  fprintf(output, "\n");
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], output, i+1, verbose_mode);
  }
}

void output_text(topo_topology_t topology, FILE *output, int verbose_mode)
{
  output_topology (topology, topo_topology_get_machine_level(topology), output, 0, verbose_mode);

  if (verbose_mode)
    {
      struct topo_topology_info info;
      int l;

      topo_topology_get_info(topology, &info);

      if (info.dmi_board_vendor)
	fprintf (output, "DMI board vendor: %s\n", info.dmi_board_vendor);
      if (info.dmi_board_name)
	fprintf (output, "DMI board name: %s\n", info.dmi_board_name);
      if (info.huge_page_size_kB)
	fprintf (output, "Huge page size: %ldkB\n", info.huge_page_size_kB);

      for (l = 0; l < LT_LEVEL_MAX; l++)
	{
	  int depth = topo_topology_get_type_depth (topology, l);
	  indent(output, depth);
	  fprintf (output, "depth %d:\ttype #%d (%s)\n", depth, l, lt_level_string (l));
	}
    }
}
