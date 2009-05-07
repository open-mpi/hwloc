/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#include "lstopo.h"

int
main (int argc, char *argv[])
{
  int err;
  int verbose_mode = 0;
  topo_topology_t topology;
  char *filename = NULL;
  unsigned long flags = 0;
  FILE *output;
  int merge = 0;
  int ignorecache = 0;
  char * synthetic = NULL;
  int opt;

  err = topo_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  while (argc >= 2)
    {
      opt = 0;
      if (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--verbose"))
	verbose_mode = 1;
      else if (!strcmp (argv[1], "-h") || !strcmp (argv[1], "--help")) {
        fprintf (stderr, "By default, lstopo displays a graphical window with the topology.\n");
        fprintf (stderr, "To produce a text output on the standard output, specify the parameter <-> or </dev/stdout>.\n");
        fprintf (stderr, "Output can also be saved in a file. Recognised file formats are: <txt>, <fig>, <pdf>, <ps>, <png>, <svg>\n");
        exit(EXIT_FAILURE);
      }
      else if (!strcmp (argv[1], "--no-caches"))
	ignorecache = 2;
      else if (!strcmp (argv[1], "--no-useless-caches"))
	ignorecache = 1;
      else if (!strcmp (argv[1], "--no-threads"))
	flags |= TOPO_FLAGS_IGNORE_THREADS;
      else if (!strcmp (argv[1], "--no-admin"))
	flags |= TOPO_FLAGS_IGNORE_ADMIN_DISABLE;
      else if (!strcmp (argv[1], "--merge"))
	merge = 1;
      else if (argc > 2 && !strcmp (argv[1], "--synthetic")) {
	synthetic = argv[2]; opt = 1;
      } else {
	if (filename)
	  fprintf (stderr, "Unrecognized options: %s\n", argv[1]);
	else
	  filename = argv[1];
      }
      argc -= opt+1;
      argv += opt+1;
    }

  if (!filename || !strcmp(filename, "-"))
    output = stdout;
  else
    output = fopen(filename, "w");

  topo_topology_set_flags(topology, flags);

  if (ignorecache > 1) {
    topo_topology_ignore_type(topology, TOPO_LEVEL_CACHE);
  } else if (ignorecache) {
    topo_topology_ignore_type_keep_structure(topology, TOPO_LEVEL_CACHE);
  }
  if (merge)
    topo_topology_ignore_all_keep_structure(topology);

  if (synthetic)
    topo_topology_set_synthetic(topology, synthetic);

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  if (!filename) {
#ifdef HAVE_CAIRO
#if CAIRO_HAS_XLIB_SURFACE
    if (getenv("DISPLAY"))
      output_x11(topology, output, verbose_mode);
    else
#endif /* CAIRO_HAS_XLIB_SURFACE */
#endif /* HAVE_CAIRO */
#ifdef WIN_SYS
      output_windows(topology, output, verbose_mode);
#else
      output_text(topology, output, verbose_mode);
#endif
  } else if (!strcmp(filename, "-")
	  || !strcmp(filename, "/dev/stdout")
	  ||  strstr(filename, ".txt"))
    output_text(topology, output, verbose_mode);
  else if (strstr(filename, ".fig"))
    output_fig(topology, output, verbose_mode);
#ifdef HAVE_CAIRO
#if CAIRO_HAS_PNG_FUNCTIONS
  else if (strstr(filename, ".png"))
    output_png(topology, output, verbose_mode);
#endif /* CAIRO_HAS_PNG_FUNCTIONS */
#if CAIRO_HAS_PDF_SURFACE
  else if (strstr(filename, ".pdf"))
    output_pdf(topology, output, verbose_mode);
#endif /* CAIRO_HAS_PDF_SURFACE */
#if CAIRO_HAS_PS_SURFACE
  else if (strstr(filename, ".ps"))
    output_ps(topology, output, verbose_mode);
#endif /* CAIRO_HAS_PS_SURFACE */
#if CAIRO_HAS_SVG_SURFACE
  else if (strstr(filename, ".svg"))
    output_svg(topology, output, verbose_mode);
#endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* HAVE_CAIRO */
  else {
    fprintf(stderr, "file format not supported\n");
    exit(EXIT_FAILURE);
  }

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
