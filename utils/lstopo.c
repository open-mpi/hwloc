/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void output_topology (lt_topo_t *topology, lt_level_t *l, FILE *output, int i, int verbose_mode);

int
main (int argc, char *argv[])
{
  int err;
  int verbose_mode = 0;
  lt_topo_t *topology = topo_default_allocator.allocate (sizeof (lt_topo_t));
  FILE *output;

  err = topo_init (topology);
  if (!err)
    return EXIT_FAILURE;

  while (argc >= 2)
    {
      if (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--verbose"))
	verbose_mode = 1;
      else
	fprintf (stderr, "Unrecognized options: %s\n", argv[1]);
      argc--;
      argv++;
    }

  output = stdout;

  output_topology (topology, &topology->levels[0][0], output, 0, verbose_mode);

  if (verbose_mode) 
    {
      int l,i;
      for (l = 0; l < LT_LEVEL_MAX; l++) 
	{
	  int depth = topology->type_depth[l];
	  for (i = 0; i<depth; i++)
	    fprintf (output, " ");
	  fprintf (output, "depth %d:\ttype #%d (%s)\n", depth, l, lt_level_string (l));
	}
    }

  topo_fini (topology);

  return EXIT_SUCCESS;
}

#define indent(output, i) \
  fprintf (output, "%*s", 2*i, "");

static void 
output_topology (lt_topo_t *topology, lt_level_t *l, FILE *output, int i, int verbose_mode) {
  int x;
  const char * separator = " ";
  const char * indexprefix = "#";
  const char * labelseparator = ":";
  const char * levelterm = "";

  indent (output, i);
  lt_print_level (topology, l, output, verbose_mode, separator, indexprefix, labelseparator, levelterm);
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], output, i+1, verbose_mode);
  }
}
