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

#ifdef HWLOC_HAVE_CAIRO
#include <cairo.h>
#endif

#include "lstopo.h"

int logical = 1;
hwloc_obj_type_t show_only = (hwloc_obj_type_t) -1;
int show_cpuset = 0;
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
#ifdef HWLOC_HAVE_CAIRO
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
#endif /* HWLOC_HAVE_CAIRO */
#ifdef HWLOC_HAVE_XML
		  ", .xml"
#endif /* HWLOC_HAVE_XML */
		  "\n");
  fprintf (where, "\nOptions:\n");
  fprintf (where, "   -l --logical          display logical object indexes (default)\n");
  fprintf (where, "   -p --physical         display physical object indexes\n");
  fprintf (where, "   -v --verbose          increase verbosity (disabled by default)\n");
  fprintf (where, "   -s --silent           decrease verbosity\n");
  fprintf (where, "   -c --cpuset           show the cpuset\n");
  fprintf (where, "   -C --cpuset-only      only show the cpuset\n");
  fprintf (where, "   --only <type>         only show the given type\n");
  fprintf (where, "   --no-caches           do not show caches\n");
  fprintf (where, "   --no-useless-caches   do not show caches which do not have a hierarchical\n"
                  "                         impact\n");
  fprintf (where, "   --whole-system        do not consider administration limitations\n");
  fprintf (where, "   --merge               do not show levels that do not have a hierarcical\n"
                  "                         impact\n");
#ifdef HWLOC_HAVE_LIBPCI
  fprintf (where, "   --whole-pci           show all PCI devices and bridges\n");
  fprintf (where, "   --no-pci              do not show any PCI device or bridge\n");
#endif
#ifdef HWLOC_HAVE_XML
  fprintf (where, "   --xml <path>          read topology from XML file <path>\n");
#endif
#ifdef HWLOC_LINUX_SYS
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
  const char *filename = NULL;
  unsigned long flags = 0;
  int merge = 0;
  int ignorecache = 0;
  char * callname;
  char * synthetic = NULL;
  const char * xmlpath = NULL;
  char * fsysroot = NULL;
  int force_console = 0;
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
      if (!strcmp (argv[1], "-v") || !strcmp (argv[1], "--verbose")) {
	verbose_mode++;
	force_console = 1;
      } else if (!strcmp (argv[1], "-s") || !strcmp (argv[1], "--silent")) {
	verbose_mode--;
	force_console = 1;
      } else if (!strcmp (argv[1], "-h") || !strcmp (argv[1], "--help")) {
	usage(callname, stdout);
        exit(EXIT_SUCCESS);
      } else if (!strcmp (argv[1], "-l") || !strcmp (argv[1], "--logical"))
	logical = 1;
      else if (!strcmp (argv[1], "-p") || !strcmp (argv[1], "--physical"))
	logical = 0;
      else if (!strcmp (argv[1], "-c") || !strcmp (argv[1], "--cpuset"))
	show_cpuset = 1;
      else if (!strcmp (argv[1], "-C") || !strcmp (argv[1], "--cpuset-only"))
	show_cpuset = 2;
      else if (!strcmp (argv[1], "--only")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
        show_only = hwloc_obj_type_of_string(argv[2]);
	opt = 1;
      }
      else if (!strcmp (argv[1], "--no-caches"))
	ignorecache = 2;
      else if (!strcmp (argv[1], "--no-useless-caches"))
	ignorecache = 1;
      else if (!strcmp (argv[1], "--whole-system"))
	flags |= HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM;
      else if (!strcmp (argv[1], "--whole-pci"))
	flags |= HWLOC_TOPOLOGY_FLAG_WHOLE_PCI;
      else if (!strcmp (argv[1], "--no-pci"))
	flags |= HWLOC_TOPOLOGY_FLAG_NO_PCI;
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
#ifdef HWLOC_HAVE_XML
      } else if (!strcmp (argv[1], "--xml")) {
	if (argc <= 2) {
	  usage (callname, stderr);
	  exit(EXIT_FAILURE);
	}
	xmlpath = argv[2]; opt = 1;
	if (!strcmp(xmlpath, "-"))
	  xmlpath = "/dev/stdin";
#endif /* HWLOC_HAVE_XML */
#ifdef HWLOC_LINUX_SYS
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

  if (show_only != (hwloc_obj_type_t)-1) {
    merge = 0;
    force_console = 1;
  }
  if (show_cpuset)
    force_console = 1;

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

  if (!filename && !strcmp(callname,"hwloc-info")) {
    /* behave kind-of plpa-info */
    filename = "-";
    verbose_mode--;
  }

  if (!filename) {
#ifdef HWLOC_HAVE_CAIRO
#if CAIRO_HAS_XLIB_SURFACE && defined HWLOC_HAVE_X11
    if (!force_console && getenv("DISPLAY"))
      output_x11(topology, NULL, logical, verbose_mode);
    else
#endif /* CAIRO_HAS_XLIB_SURFACE */
#endif /* HWLOC_HAVE_CAIRO */
#ifdef HWLOC_WIN_SYS
      output_windows(topology, NULL, logical, verbose_mode);
#else
    output_console(topology, NULL, logical, verbose_mode);
#endif
  } else if (!strcmp(filename, "-")
	  || !strcmp(filename, "/dev/stdout"))
    output_console(topology, filename, logical, verbose_mode);
  else if (strstr(filename, ".txt"))
    output_text(topology, filename, logical, verbose_mode);
  else if (strstr(filename, ".fig"))
    output_fig(topology, filename, logical, verbose_mode);
#ifdef HWLOC_HAVE_CAIRO
#if CAIRO_HAS_PNG_FUNCTIONS
  else if (strstr(filename, ".png"))
    output_png(topology, filename, logical, verbose_mode);
#endif /* CAIRO_HAS_PNG_FUNCTIONS */
#if CAIRO_HAS_PDF_SURFACE
  else if (strstr(filename, ".pdf"))
    output_pdf(topology, filename, logical, verbose_mode);
#endif /* CAIRO_HAS_PDF_SURFACE */
#if CAIRO_HAS_PS_SURFACE
  else if (strstr(filename, ".ps"))
    output_ps(topology, filename, logical, verbose_mode);
#endif /* CAIRO_HAS_PS_SURFACE */
#if CAIRO_HAS_SVG_SURFACE
  else if (strstr(filename, ".svg"))
    output_svg(topology, filename, logical, verbose_mode);
#endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* HWLOC_HAVE_CAIRO */
#ifdef HWLOC_HAVE_XML
  else if (strstr(filename, ".xml"))
    output_xml(topology, filename, logical, verbose_mode);
#endif
  else {
    fprintf(stderr, "file format not supported\n");
    usage(callname, stderr);
    exit(EXIT_FAILURE);
  }

  hwloc_topology_destroy (topology);

  return EXIT_SUCCESS;
}
