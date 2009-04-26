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
output_topology (lt_topo_t *topology, lt_level_t l, FILE *output, int i, int verbose_mode) {
  int x;
  const char * separator = " ";
  const char * indexprefix = "#";
  const char * labelseparator = ":";
  const char * levelterm = "";

  indent (output, 2*i);
  lt_print_level (topology, l, output, verbose_mode, separator, indexprefix, labelseparator, levelterm);
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], output, i+1, verbose_mode);
  }
}

void output_text(lt_topo_t *topology, FILE *output, int verbose_mode)
{
  output_topology (topology, &topology->levels[0][0], output, 0, verbose_mode);

  if (verbose_mode)
    {
      int l;

      if (topology->dmi_board_vendor)
	fprintf (output, "DMI board vendor: %s\n", topology->dmi_board_vendor);
      if (topology->dmi_board_name)
	fprintf (output, "DMI board name: %s\n", topology->dmi_board_name);
      if (topology->huge_page_size_kB)
	fprintf (output, "Huge page size: %ldkB\n", topology->huge_page_size_kB);

      for (l = 0; l < LT_LEVEL_MAX; l++)
	{
	  int depth = topology->type_depth[l];
	  indent(output, depth);
	  fprintf (output, "depth %d:\ttype #%d (%s)\n", depth, l, lt_level_string (l));
	}
    }
}
