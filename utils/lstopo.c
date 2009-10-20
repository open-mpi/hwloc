/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#include "lstopo.h"

unsigned int fontsize = 10;
unsigned int gridsize = 10;
unsigned int force_horiz = 0;
unsigned int force_vert = 0;

FILE *open_file(const char *filename, const char *mode)
{
  const char *extn = strrchr(filename, '.');

  if (filename[0] == '-' && extn == filename + 1)
    return stdout;

  return fopen(filename, mode);
}

static void usage(char *name, FILE *where)
{
  fprintf (where, "Usage: %s [ options ]... [ filename ]\n", name);
  fprintf (where, "\n");
  fprintf (where, "By default, lstopo displays a graphical window with the topology if DISPLAY is\nset, else a text output on the standard output.\n");
  fprintf (where, "To force a text output on the standard output, specify - or /dev/stdout as\nfilename.\n");
  fprintf (where, "To specify a semi-graphical text output on the standard output, specify -.txt\nas filename.\n");
  fprintf (where, "Recognised file formats are: .txt, .fig"
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
  fprintf (where, "\nOptions:\n");
  fprintf (where, "   -v --verbose          increase verbosity (disabled by default)\n");
  fprintf (where, "   -s --silent           decrease verbosity\n");
  fprintf (where, "   --no-caches           do not show caches\n");
  fprintf (where, "   --no-useless-caches   do not show caches which do not have a hierarchical\n"
                  "                         impact\n");
  fprintf (where, "   --whole-system        do not consider administration limitations\n");
  fprintf (where, "   --merge               do not show levels that do not have a hierarcical\n"
                  "                         impact\n");
#ifdef HAVE_XML
  fprintf (where, "   --xml <path>          read topology from XML file <path>\n");
#endif
#ifdef LINUX_SYS
  fprintf (where, "   --fsys-root <path>    chroot containing the /proc and /sys of another system\n");
#endif
  fprintf (where, "   --synthetic \"n:2 2\"   simulate a fake hierarchy, here with 2 NUMA nodes of 2\n"
                  "                         processors\n");
  fprintf (where, "   --fontsize 10         set size of text font\n");
  fprintf (where, "   --gridsize 10         set size of margin between elements\n");
  fprintf (where, "   --horiz               horizontal graphic layout instead of nearly 4/3 ratio\n");
  fprintf (where, "   --vert                vertical graphic layout instead of nearly 4/3 ratio\n");
  fprintf (where, "   --version             report version and exit\n");
}

int
main (int argc, char *argv[])
{
  int err;
  int verbose_mode = 1;
  hwloc_topology_t topology;
  char *filename = NULL;
  unsigned long flags = 0;
  int merge = 0;
  int ignorecache = 0;
  char * callname;
  char * synthetic = NULL;
  const char * xmlpath = NULL;
  char * fsysroot = NULL;
  int opt;

  callname = strrchr(argv[0], '/');
  if (!callname)
    callname = argv[0];
  else
    callname++;

  err = hwloc_topology_init (&topology);
  if (err)
    return EXIT_FAILURE;

  while (argc >= 2)
    {
      opt = 0;
      if (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--verbose"))
	verbose_mode++;
      else if (!strcmp (argv[1], "-s") || !strcmp (argv[1], "--silent"))
	verbose_mode--;
      else if (!strcmp (argv[1], "-h") || !strcmp (argv[1], "--help")) {
	usage(callname, stdout);
        exit(EXIT_SUCCESS);
      }
      else if (!strcmp (argv[1], "--no-caches"))
	ignorecache = 2;
      else if (!strcmp (argv[1], "--no-useless-caches"))
	ignorecache = 1;
      else if (!strcmp (argv[1], "--whole-system"))
	flags |= HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM;
      else if (!strcmp (argv[1], "--merge"))
	merge = 1;
      else if (!strcmp (argv[1], "--horiz"))
        force_horiz = 1;
      else if (!strcmp (argv[1], "--vert"))
        force_vert = 1;
      else if (!strcmp (argv[1], "--fontsize")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	fontsize = atoi(argv[2]);
	opt = 1;
      }
      else if (!strcmp (argv[1], "--gridsize")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	gridsize = atoi(argv[2]);
	opt = 1;
      }
      else if (!strcmp (argv[1], "--synthetic")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	synthetic = argv[2]; opt = 1;
#ifdef HAVE_XML
      } else if (!strcmp (argv[1], "--xml")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	xmlpath = argv[2]; opt = 1;
	if (!strcmp(xmlpath, "-"))
	  xmlpath = "/dev/stdin";
#endif /* HAVE_XML */
#ifdef LINUX_SYS
      } else if (!strcmp (argv[1], "--fsys-root")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	fsysroot = argv[2]; opt = 1;
#endif
      } else if (!strcmp (argv[1], "--version")) {
          printf("%s %s\n", callname, VERSION);
          exit(EXIT_SUCCESS);
      } else {
	if (filename) {
	  fprintf (stderr, "Unrecognized options: %s\n", argv[1]);
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	} else
	  filename = argv[1];
      }
      argc -= opt+1;
      argv += opt+1;
    }

  hwloc_topology_set_flags(topology, flags);

  if (ignorecache > 1) {
    hwloc_topology_ignore_type(topology, HWLOC_OBJ_CACHE);
  } else if (ignorecache) {
    hwloc_topology_ignore_type_keep_structure(topology, HWLOC_OBJ_CACHE);
  }
  if (merge)
    hwloc_topology_ignore_all_keep_structure(topology);

  if (synthetic)
    hwloc_topology_set_synthetic(topology, synthetic);
  if (xmlpath)
    hwloc_topology_set_xml(topology, xmlpath);
  if (fsysroot)
    hwloc_topology_set_fsroot(topology, fsysroot);

  err = hwloc_topology_load (topology);
  if (err)
    return EXIT_FAILURE;

  if (!filename) {
#ifdef HAVE_CAIRO
#if CAIRO_HAS_XLIB_SURFACE && defined HAVE_X11
    if (getenv("DISPLAY"))
      output_x11(topology, NULL, verbose_mode);
    else
#endif /* CAIRO_HAS_XLIB_SURFACE */
#endif /* HAVE_CAIRO */
#ifdef WIN_SYS
      output_windows(topology, NULL, verbose_mode);
#else
    output_console(topology, NULL, verbose_mode);
#endif
  } else if (!strcmp(filename, "-")
	  || !strcmp(filename, "/dev/stdout"))
    output_console(topology, filename, verbose_mode);
  else if (strstr(filename, ".txt"))
    output_text(topology, filename, verbose_mode);
  else if (strstr(filename, ".fig"))
    output_fig(topology, filename, verbose_mode);
#ifdef HAVE_CAIRO
#if CAIRO_HAS_PNG_FUNCTIONS
  else if (strstr(filename, ".png"))
    output_png(topology, filename, verbose_mode);
#endif /* CAIRO_HAS_PNG_FUNCTIONS */
#if CAIRO_HAS_PDF_SURFACE
  else if (strstr(filename, ".pdf"))
    output_pdf(topology, filename, verbose_mode);
#endif /* CAIRO_HAS_PDF_SURFACE */
#if CAIRO_HAS_PS_SURFACE
  else if (strstr(filename, ".ps"))
    output_ps(topology, filename, verbose_mode);
#endif /* CAIRO_HAS_PS_SURFACE */
#if CAIRO_HAS_SVG_SURFACE
  else if (strstr(filename, ".svg"))
    output_svg(topology, filename, verbose_mode);
#endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* HAVE_CAIRO */
#ifdef HAVE_XML
  else if (strstr(filename, ".xml"))
    output_xml(topology, filename, verbose_mode);
#endif
  else {
    fprintf(stderr, "file format not supported\n");
    usage(callname, stderr);
    exit(EXIT_FAILURE);
  }

  hwloc_topology_destroy (topology);

  return EXIT_SUCCESS;
}
