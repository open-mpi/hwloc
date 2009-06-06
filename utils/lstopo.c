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

#include <config.h>
#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#include "lstopo.h"

static void usage(void)
{
  fprintf (stderr, "Usage: lstopo [ options ]... [ filename ]\n");
  fprintf (stderr, "\n");
  fprintf (stderr, "By default, lstopo displays a graphical window with the topology if DISPLAY is\nset, else a text output on the standard output.\n");
  fprintf (stderr, "To force a text output on the standard output, specify - or /dev/stdout as\nfilename.\n");
  fprintf (stderr, "Recognised file formats are: .txt, .fig"
#ifdef HAVE_CAIRO
#if CAIRO_HAS_PDF_SURFACE
		  ", .pdf"
#endif /* CAIRO_HAS_PDF_SURFACE */
#if CAIRO_HAS_PS_SURFACE
		  ", .ps"
#endif /* CAIRO_HAS_PS_SURFACE */
#if CAIRO_HAS_PNG_FUNCTIONS
		  ", .png"
#endif /* CAIRO_HAS_PNG_FUNCTIONS */
#if CAIRO_HAS_SVG_SURFACE
		  ", .svg"
#endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* HAVE_CAIRO */
#ifdef HAVE_XML
		  ", .xml"
#endif /* HAVE_XML */
		  "\n");
  fprintf (stderr, "\nRecognized options:\n");
  fprintf (stderr, "--no-caches         do not show caches\n");
  fprintf (stderr, "--no-useless-caches do not show caches which do not have a hierarchical impact\n");
  fprintf (stderr, "--no-threads        do not show SMT threads\n");
  fprintf (stderr, "--no-admin          do not consider administration limitations\n");
  fprintf (stderr, "--merge             do not show levels that do not have a hierarcical impact\n");
  fprintf (stderr, "--synthetic \"2 2\"   simulate a fake hierarchy\n");
#ifdef HAVE_XML
  fprintf (stderr, "--xml <path>        read topology from XML file <path>\n");
#endif
#ifdef LINUX_SYS
  fprintf (stderr, "\nThe TOPO_FSYS_ROOT_PATH environment variable can be used to specify the path to\n a chroot containing the /proc and /sys of another system\n");
#endif
}

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
  char * xmlpath = NULL;
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
	usage();
        exit(EXIT_SUCCESS);
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
      else if (!strcmp (argv[1], "--synthetic")) {
	if (argc <= 2) {
	  usage ();
	  exit(EXIT_FAILURE);
	}
	synthetic = argv[2]; opt = 1;
#ifdef HAVE_XML
      } else if (!strcmp (argv[1], "--xml")) {
	if (argc <= 2) {
	  usage ();
	  exit(EXIT_FAILURE);
	}
	xmlpath = argv[2]; opt = 1;
#endif /* HAVE_XML */
      } else {
	if (filename) {
	  fprintf (stderr, "Unrecognized options: %s\n", argv[1]);
	  usage ();
	} else
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
    topo_topology_ignore_type(topology, TOPO_OBJ_CACHE);
  } else if (ignorecache) {
    topo_topology_ignore_type_keep_structure(topology, TOPO_OBJ_CACHE);
  }
  if (merge)
    topo_topology_ignore_all_keep_structure(topology);

  if (synthetic)
    topo_topology_set_synthetic(topology, synthetic);
  if (xmlpath)
    topo_topology_set_xml(topology, xmlpath);

  err = topo_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  if (!filename) {
#ifdef HAVE_CAIRO
#if CAIRO_HAS_XLIB_SURFACE && defined HAVE_X11
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
#ifdef HAVE_XML
  else if (strstr(filename, ".xml"))
    output_xml(topology, filename, verbose_mode);
#endif
  else {
    fprintf(stderr, "file format not supported\n");
    usage();
    exit(EXIT_FAILURE);
  }

  topo_topology_destroy (topology);

  return EXIT_SUCCESS;
}
