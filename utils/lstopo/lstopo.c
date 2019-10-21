/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2019 Inria.  All rights reserved.
 * Copyright © 2009-2012, 2015, 2017 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#ifdef HWLOC_LINUX_SYS
#include "hwloc/linux.h"
#endif /* HWLOC_LINUX_SYS */
#include "hwloc/shmem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#ifdef LSTOPO_HAVE_GRAPHICS
#ifdef HWLOC_HAVE_CAIRO
#include <cairo.h>
#endif
#endif

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

#include "lstopo.h"
#include "common-ps.h"
#include "misc.h"

#ifdef __MINGW32__
# ifdef HAVE_CLOCK_GETTIME
#  undef HAVE_CLOCK_GETTIME
# endif
#endif

#ifdef HAVE_CLOCK_GETTIME
# ifndef CLOCK_MONOTONIC /* HP-UX doesn't have CLOCK_MONOTONIC */
#  define CLOCK_MONOTONIC CLOCK_REALTIME
# endif
#endif

FILE *open_output(const char *filename, int overwrite)
{
  const char *extn;
  struct stat st;

  if (!filename || !strcmp(filename, "-"))
    return stdout;

  extn = strrchr(filename, '.');
  if (filename[0] == '-' && extn == filename + 1)
    return stdout;

  if (!stat(filename, &st) && !overwrite) {
    errno = EEXIST;
    return NULL;
  }

  return fopen(filename, "w");
}

const char *task_background_color_string = "#ffff00";

static hwloc_obj_t insert_task(hwloc_topology_t topology, hwloc_cpuset_t cpuset, const char * name)
{
  hwloc_obj_t group, obj;

  hwloc_bitmap_and(cpuset, cpuset, hwloc_topology_get_topology_cpuset(topology));
  if (hwloc_bitmap_iszero(cpuset))
    return NULL;

  /* try to insert a group at exact position */
  group = hwloc_topology_alloc_group_object(topology);
  if (!group)
    return NULL;
  group->cpuset = hwloc_bitmap_dup(cpuset);
  group = hwloc_topology_insert_group_object(topology, group);
  if (!group) {
    /* try to insert in a larger parent */
    char *s;
    hwloc_bitmap_asprintf(&s, cpuset);
    group = hwloc_get_obj_covering_cpuset(topology, cpuset);
    fprintf(stderr, "Inserting process `%s' below parent larger than cpuset %s\n", name, s);
    free(s);
  }
  obj = hwloc_topology_insert_misc_object(topology, group, name);
  if (!obj)
    fprintf(stderr, "Failed to insert process `%s'\n", name);
  else {
    obj->subtype = strdup("Process");
    if (strcmp(task_background_color_string, "none")) {
      char style[19];
      snprintf(style, sizeof(style), "Background=%s", task_background_color_string);
      hwloc_obj_add_info(obj, "lstopoStyle", style);
    }
  }

  return obj;
}

static void foreach_process_cb(hwloc_topology_t topology,
			       struct hwloc_ps_process *proc,
			       void *cbdata __hwloc_attribute_unused)
{
  char name[100];
  unsigned i;

  snprintf(name, sizeof(name), "%ld", proc->pid);
  if (*proc->name)
    snprintf(name, sizeof(name), "%ld %s", proc->pid, proc->name);

  if (proc->bound)
    insert_task(topology, proc->cpuset, name);

  if (proc->nthreads)
    for(i=0; i<proc->nthreads; i++)
      if (proc->threads[i].cpuset
	  && !hwloc_bitmap_isequal(proc->threads[i].cpuset, proc->cpuset)) {
	char task_name[150];
	if (*proc->threads[i].name)
	  snprintf(task_name, sizeof(task_name), "%s %li %s", name, proc->threads[i].tid, proc->threads[i].name);
	else
	  snprintf(task_name, sizeof(task_name), "%s %li", name, proc->threads[i].tid);

	insert_task(topology, proc->threads[i].cpuset, task_name);
      }
}

static void add_process_objects(hwloc_topology_t topology)
{
  const struct hwloc_topology_support *support = hwloc_topology_get_support(topology);
  hwloc_obj_t root = hwloc_get_root_obj(topology);

  if (!support->cpubind->get_proc_cpubind)
    return;

  hwloc_ps_foreach_process(topology, root->cpuset,
			   foreach_process_cb, NULL,
			   HWLOC_PS_FLAG_THREADS | HWLOC_PS_FLAG_SHORTNAME, NULL, NULL);
}

static __hwloc_inline void lstopo_update_factorize_bounds(unsigned min, unsigned *first, unsigned *last)
{
  switch (min) {
  case 0:
  case 1:
  case 2:
    *first = 1;
    *last = 0;
    break;
  case 3:
    *first = 1;
    *last = 1;
    break;
  default:
    *first = 2;
    *last = 1;
    break;
  }
}

static __hwloc_inline void lstopo_update_factorize_alltypes_bounds(struct lstopo_output *loutput)
{
  hwloc_obj_type_t type;
  for(type = 0; type < HWLOC_OBJ_TYPE_MAX; type++)
    lstopo_update_factorize_bounds(loutput->factorize_min[type], &loutput->factorize_first[type], &loutput->factorize_last[type]);
}

static void
lstopo_add_factorized_attributes(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  hwloc_obj_t child;

  if (!obj->first_child)
    return;

  if (obj->symmetric_subtree && obj->arity > loutput->factorize_min[obj->first_child->type]){
    /* factorize those children */
    for_each_child(child, obj) {
      unsigned factorized;
      if (child->sibling_rank < loutput->factorize_first[child->type]
	  || child->sibling_rank >= obj->arity - loutput->factorize_last[child->type])
	factorized = 0; /* keep first and last */
      else if (child->sibling_rank == loutput->factorize_first[child->type])
	factorized = 1; /* replace with dots */
      else
	factorized = -1; /* remove that one */

      ((struct lstopo_obj_userdata *)child->userdata)->factorized = factorized;
    }
  }
  /* recurse */
  for_each_child(child, obj)
    lstopo_add_factorized_attributes(loutput, child);
}

static void
lstopo_add_collapse_attributes(hwloc_topology_t topology)
{
  hwloc_obj_t obj, collapser = NULL;
  unsigned collapsed = 0;
  /* collapse identical PCI devs */
  for(obj = hwloc_get_next_pcidev(topology, NULL); obj; obj = hwloc_get_next_pcidev(topology, obj)) {
    if (collapser) {
      if (!obj->io_arity && !obj->misc_arity
	  && obj->parent == collapser->parent
	  && obj->attr->pcidev.vendor_id == collapser->attr->pcidev.vendor_id
	  && obj->attr->pcidev.device_id == collapser->attr->pcidev.device_id
	  && obj->attr->pcidev.subvendor_id == collapser->attr->pcidev.subvendor_id
	  && obj->attr->pcidev.subdevice_id == collapser->attr->pcidev.subdevice_id) {
	/* collapse another one */
	((struct lstopo_obj_userdata *)obj->userdata)->pci_collapsed = -1;
	collapsed++;
	continue;
      } else if (collapsed > 1) {
	/* end this collapsing */
	((struct lstopo_obj_userdata *)collapser->userdata)->pci_collapsed = collapsed;
	collapser = NULL;
	collapsed = 0;
      }
    }
    if (!obj->io_arity && !obj->misc_arity) {
      /* start a new collapsing */
      collapser = obj;
      collapsed = 1;
    }
  }
  if (collapsed > 1) {
    /* end this collapsing */
    ((struct lstopo_obj_userdata *)collapser->userdata)->pci_collapsed = collapsed;
  }
}

static int
lstopo_check_pci_domains(hwloc_topology_t topology)
{
  hwloc_obj_t obj;

  /* check PCI devices for domains.
   * they are listed by depth-first search, the order doesn't guarantee a domain at the end.
   */
  obj = NULL;
  while ((obj = hwloc_get_next_pcidev(topology, obj)) != NULL) {
    if (obj->attr->pcidev.domain)
      return 1;
  }

  /* check PCI Bridges for domains.
   * they are listed by depth-first search, the order doesn't guarantee a domain at the end.
   */
  obj = NULL;
  while ((obj = hwloc_get_next_bridge(topology, obj)) != NULL) {
    if (obj->attr->bridge.upstream_type != HWLOC_OBJ_BRIDGE_PCI)
      break;
    if (obj->attr->pcidev.domain)
      return 1;
  }

  return 0;
}

static void
lstopo_populate_userdata(hwloc_obj_t parent)
{
  hwloc_obj_t child;
  struct lstopo_obj_userdata *save = malloc(sizeof(*save));

  save->common.buffer = NULL; /* so that it is ignored on XML export */
  save->common.next = parent->userdata;
  save->factorized = 0;
  save->pci_collapsed = 0;
  parent->userdata = save;

  for_each_child(child, parent)
    lstopo_populate_userdata(child);
  for_each_memory_child(child, parent)
    lstopo_populate_userdata(child);
  for_each_io_child(child, parent)
    lstopo_populate_userdata(child);
  for_each_misc_child(child, parent)
    lstopo_populate_userdata(child);
}

static void
lstopo_destroy_userdata(hwloc_obj_t parent)
{
  hwloc_obj_t child;
  struct lstopo_obj_userdata *save = parent->userdata;

  if (save) {
    parent->userdata = save->common.next;
    free(save);
  }

  for_each_child(child, parent)
    lstopo_destroy_userdata(child);
  for_each_memory_child(child, parent)
    lstopo_destroy_userdata(child);
  for_each_io_child(child, parent)
    lstopo_destroy_userdata(child);
  for_each_misc_child(child, parent)
    lstopo_destroy_userdata(child);
}

void usage(const char *name, FILE *where)
{
  fprintf (where, "Usage: %s [ options ] ... [ filename.format ]\n\n", name);
  fprintf (where, "See lstopo(1) for more details.\n");

  fprintf (where, "\nDefault output is "
#ifdef LSTOPO_HAVE_GRAPHICS
#ifdef HWLOC_WIN_SYS
		  "graphical"
#elif (defined LSTOPO_HAVE_X11)
		  "graphical (X11) if DISPLAY is set, console otherwise"
#else
		  "console"
#endif
#else
		  "console"
#endif
		  ".\n");

  fprintf (where, "Supported output file formats: console, ascii, fig"
#ifdef LSTOPO_HAVE_GRAPHICS
#ifdef CAIRO_HAS_PDF_SURFACE
		  ", pdf"
#endif /* CAIRO_HAS_PDF_SURFACE */
#ifdef CAIRO_HAS_PS_SURFACE
		  ", ps"
#endif /* CAIRO_HAS_PS_SURFACE */
#ifdef CAIRO_HAS_PNG_FUNCTIONS
		  ", png"
#endif /* CAIRO_HAS_PNG_FUNCTIONS */
#ifdef CAIRO_HAS_SVG_SURFACE
		  ", svg(cairo,native)"
#endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* LSTOPO_HAVE_GRAPHICS */
#if !(defined LSTOPO_HAVE_GRAPHICS) || !(defined CAIRO_HAS_SVG_SURFACE)
		  ", svg(native)"
#endif
		  ", xml, synthetic"
		  "\n");
  fprintf (where, "\nFormatting options:\n");
  fprintf (where, "  -l --logical          Display hwloc logical object indexes\n");
  fprintf (where, "  -p --physical         Display OS/physical object indexes\n");
  fprintf (where, "Output options:\n");
  fprintf (where, "  --output-format <format>\n");
  fprintf (where, "  --of <format>         Force the output to use the given format\n");
  fprintf (where, "  -f --force            Overwrite the output file if it exists\n");
  fprintf (where, "Textual output options:\n");
  fprintf (where, "  --only <type>         Only show objects of the given type in the textual output\n");
  fprintf (where, "  -v --verbose          Include additional details\n");
  fprintf (where, "  -s --silent           Reduce the amount of details to show\n");
  fprintf (where, "  --distances           Only show distance matrices\n");
  fprintf (where, "  -c --cpuset           Show the cpuset of each object\n");
  fprintf (where, "  -C --cpuset-only      Only show the cpuset of each object\n");
  fprintf (where, "  --taskset             Show taskset-specific cpuset strings\n");
  fprintf (where, "Object filtering options:\n");
  fprintf (where, "  --filter <type>:<knd> Filter objects of the given type, or all.\n");
  fprintf (where, "     <knd> may be `all' (keep all), `none' (remove all), `structure' or `important'\n");
  fprintf (where, "  --ignore <type>       Ignore objects of the given type\n");
  fprintf (where, "  --no-smt              Ignore PUs\n");
  fprintf (where, "  --no-caches           Do not show caches\n");
  fprintf (where, "  --no-useless-caches   Do not show caches which do not have a hierarchical\n"
                  "                        impact\n");
  fprintf (where, "  --no-icaches          Do not show instruction caches\n");
  fprintf (where, "  --merge               Do not show levels that do not have a hierarchical\n"
                  "                        impact\n");
  fprintf (where, "  --no-collapse         Do not collapse identical PCI devices\n");
  fprintf (where, "  --restrict <cpuset>   Restrict the topology to processors listed in <cpuset>\n");
  fprintf (where, "  --restrict binding    Restrict the topology to the current process binding\n");
  fprintf (where, "  --restrict-flags <n>  Set the flags to be used during restrict\n");
  fprintf (where, "  --no-io               Do not show any I/O device or bridge\n");
  fprintf (where, "  --no-bridges          Do not any I/O bridge except hostbridges\n");
  fprintf (where, "  --whole-io            Show all I/O devices and bridges\n");
  fprintf (where, "Input options:\n");
  hwloc_utils_input_format_usage(where, 6);
  fprintf (where, "  --thissystem          Assume that the input topology provides the topology\n"
		  "                        for the system on which we are running\n");
  fprintf (where, "  --pid <pid>           Detect topology as seen by process <pid>\n");
  fprintf (where, "  --disallowed          Include objects disallowed by administrative limitations\n");
  fprintf (where, "  --allow <all|local|...>   Change the set of objects marked as allowed\n");
  fprintf (where, "Graphical output options:\n");
  fprintf (where, "  --children-order plain\n"
		  "                        Display memory children below the parent like any other child\n");
  fprintf (where, "  --no-factorize        Do not factorize identical objects\n");
  fprintf (where, "  --no-factorize=<type> Do not factorize identical objects of type <type>\n");
  fprintf (where, "  --factorize           Factorize identical objects (default)\n");
  fprintf (where, "  --factorize=[<type>,]<N>[,<L>[,<F>]]\n");
  fprintf (where, "                        Set the minimum number <N> of objects to factorize,\n");
  fprintf (where, "                        the numbers of first <F> and last <L> to keep,\n");
  fprintf (where, "                        for all or only the given object type <type>\n");
  fprintf (where, "  --fontsize 10         Set size of text font\n");
  fprintf (where, "  --gridsize 7          Set size of margin between elements\n");
  fprintf (where, "  --linespacing 4       Set spacing between lines of text\n");
  fprintf (where, "  --horiz[=<type,...>]  Horizontal graphical layout instead of nearly 4/3 ratio\n");
  fprintf (where, "  --vert[=<type,...>]   Vertical graphical layout instead of nearly 4/3 ratio\n");
  fprintf (where, "  --rect[=<type,...>]   Rectangular graphical layout with nearly 4/3 ratio\n");
  fprintf (where, "  --text[=<type,...>]   Display text for the given object types\n");
  fprintf (where, "  --no-text[=<type,..>] Do not display text for the given object types\n");
  fprintf (where, "  --index=[<type,...>]  Display indexes for the given object types\n");
  fprintf (where, "  --no-index=[<type,.>] Do not display indexes for the given object types\n");
  fprintf (where, "  --attrs=[<type,...>]  Display attributes for the given object types\n");
  fprintf (where, "  --no-attrs=[<type,.>] Do not display attributes for the given object types\n");
  fprintf (where, "  --no-legend           Remove the text legend at the bottom\n");
  fprintf (where, "  --append-legend <s>   Append a new line of text at the bottom of the legend\n");
  fprintf (where, "  --binding-color none    Do not colorize PU and NUMA nodes according to the binding\n");
  fprintf (where, "  --disallowed-color none Do not colorize disallowed PU and NUMA nodes\n");
  fprintf (where, "  --top-color <none|#xxyyzz> Change task background color for --top\n");
  fprintf (where, "Miscellaneous options:\n");
  fprintf (where, "  --export-xml-flags <n>\n"
		  "                        Set flags during the XML topology export\n");
  fprintf (where, "  --export-synthetic-flags <n>\n"
		  "                        Set flags during the synthetic topology export\n");
  /* --shmem-output-addr is undocumented on purpose */
  fprintf (where, "  --ps --top            Display processes within the hierarchy\n");
  fprintf (where, "  --version             Report version and exit\n");
}

void lstopo_show_interactive_help(void)
{
  printf("\n");
  printf("Keyboard shortcuts:\n");
  printf(" Zooming, scrolling and closing:\n");
  printf("  Zoom-in or out ...................... + -\n");
  printf("  Try to fit scale to window .......... F\n");
  printf("  Reset scale to default .............. 1\n");
  printf("  Scroll vertically ................... Up Down PageUp PageDown\n");
  printf("  Scroll horizontally ................. Left Right Ctrl+PageUp/Down\n");
  printf("  Scroll to the top-left corner ....... Home\n");
  printf("  Scroll to the bottom-right corner ... End\n");
  printf("  Show this help ...................... h H\n");
  printf("  Exit ................................ q Q Esc\n");
  printf(" Configuration tweaks:\n");
  printf("  Toggle factorizing or collapsing .... f\n");
  printf("  Switch display mode for indexes ..... i\n");
  printf("  Toggle displaying of object text .... t\n");
  printf("  Toggle displaying of obj attributes . a\n");
  printf("  Toggle color for disallowed objects . d\n");
  printf("  Toggle color for binding objects .... b\n");
  printf("  Toggle displaying of the legend ..... l\n");
  printf("  Export to file with current config .. E\n");
  printf("\n\n");
  fflush(stdout);
}

static void lstopo__show_interactive_cli_options(const struct lstopo_output *loutput)
{
  if (loutput->index_type == LSTOPO_INDEX_TYPE_PHYSICAL)
    printf(" -p");
  else if (loutput->index_type == LSTOPO_INDEX_TYPE_LOGICAL)
    printf(" -l");
  else if (loutput->index_type == LSTOPO_INDEX_TYPE_NONE)
    printf(" --no-index");

  if (!loutput->show_attrs_enabled)
    printf(" --no-attrs");
  if (!loutput->show_text_enabled)
    printf(" --no-text");

  if(!loutput->factorize_enabled)
    printf(" --no-factorize");
  if (!loutput->pci_collapse_enabled)
    printf(" --no-collapse");
  if (!loutput->show_binding)
    printf(" --binding-color none");
  if (!loutput->show_disallowed)
    printf(" --disallowed-color none");
  if (!loutput->legend)
    printf(" --no-legend");
}

void lstopo_show_interactive_cli_options(const struct lstopo_output *loutput)
{
  printf("\nCommand-line options for the current configuration tweaks:\n");
  lstopo__show_interactive_cli_options(loutput);
  printf("\n\nTo export to PDF:\n");
  printf("  lstopo  <your options>");
  lstopo__show_interactive_cli_options(loutput);
  printf(" topology.pdf\n\n");
}

enum output_format {
  LSTOPO_OUTPUT_DEFAULT,
  LSTOPO_OUTPUT_CONSOLE,
  LSTOPO_OUTPUT_SYNTHETIC,
  LSTOPO_OUTPUT_ASCII,
  LSTOPO_OUTPUT_FIG,
  LSTOPO_OUTPUT_PNG,
  LSTOPO_OUTPUT_PDF,
  LSTOPO_OUTPUT_PS,
  LSTOPO_OUTPUT_SVG,
  LSTOPO_OUTPUT_CAIROSVG,
  LSTOPO_OUTPUT_NATIVESVG,
  LSTOPO_OUTPUT_XML,
  LSTOPO_OUTPUT_SHMEM,
  LSTOPO_OUTPUT_ERROR
};

static enum output_format
parse_output_format(const char *name, char *callname __hwloc_attribute_unused)
{
  if (!hwloc_strncasecmp(name, "default", 3))
    return LSTOPO_OUTPUT_DEFAULT;
  else if (!hwloc_strncasecmp(name, "console", 3))
    return LSTOPO_OUTPUT_CONSOLE;
  else if (!strcasecmp(name, "synthetic"))
    return LSTOPO_OUTPUT_SYNTHETIC;
  else if (!strcasecmp(name, "ascii")
	   || !strcasecmp(name, "txt") /* backward compat with 1.10 */)
    return LSTOPO_OUTPUT_ASCII;
  else if (!strcasecmp(name, "fig"))
    return LSTOPO_OUTPUT_FIG;
  else if (!strcasecmp(name, "png"))
    return LSTOPO_OUTPUT_PNG;
  else if (!strcasecmp(name, "pdf"))
    return LSTOPO_OUTPUT_PDF;
  else if (!strcasecmp(name, "ps"))
    return LSTOPO_OUTPUT_PS;
  else if (!strcasecmp(name, "svg"))
    return LSTOPO_OUTPUT_SVG;
  else if (!strcasecmp(name, "cairosvg") || !strcasecmp(name, "svg(cairo)"))
    return LSTOPO_OUTPUT_CAIROSVG;
  else if (!strcasecmp(name, "nativesvg") || !strcasecmp(name, "svg(native)"))
    return LSTOPO_OUTPUT_NATIVESVG;
  else if (!strcasecmp(name, "xml"))
    return LSTOPO_OUTPUT_XML;
  else if (!strcasecmp(name, "shmem"))
    return LSTOPO_OUTPUT_SHMEM;
  else
    return LSTOPO_OUTPUT_ERROR;
}

#define LSTOPO_VERBOSE_MODE_DEFAULT 1

int
main (int argc, char *argv[])
{
  int err;
  hwloc_topology_t topology;
  const char *filename = NULL;
  unsigned long flags = 0;
  unsigned long restrict_flags = 0;
  unsigned long allow_flags = 0;
  hwloc_bitmap_t allow_cpuset = NULL, allow_nodeset = NULL;
  char * callname;
  char * input = NULL;
  enum hwloc_utils_input_format input_format = HWLOC_UTILS_INPUT_DEFAULT;
  enum output_format output_format = LSTOPO_OUTPUT_DEFAULT;
  char *restrictstring = NULL;
  struct lstopo_output loutput;
  output_method *output_func;
  hwloc_membind_policy_t policy;
#ifdef HAVE_CLOCK_GETTIME
  struct timespec ts1, ts2;
  unsigned long ms;
  int measure_load_time = !!getenv("HWLOC_DEBUG_LOAD_TIME");
#endif
  char *env;
  int top = 0;
  int opt;
  unsigned i;

  callname = strrchr(argv[0], '/');
  if (!callname)
    callname = argv[0];
  else
    callname++;
  /* skip argv[0], handle options */
  argc--;
  argv++;

  hwloc_utils_check_api_version(callname);

  loutput.overwrite = 0;

  loutput.index_type = LSTOPO_INDEX_TYPE_DEFAULT;
  loutput.verbose_mode = LSTOPO_VERBOSE_MODE_DEFAULT;
  loutput.ignore_pus = 0;
  loutput.ignore_numanodes = 0;
  loutput.pci_collapse_enabled = 1;
  loutput.pid_number = -1;
  loutput.pid = 0;
  loutput.need_pci_domain = 0;

  loutput.factorize_enabled = 1;
  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
    loutput.factorize_min[i] = FACTORIZE_MIN_DEFAULT;
  lstopo_update_factorize_alltypes_bounds(&loutput);

  loutput.export_synthetic_flags = 0;
  loutput.export_xml_flags = 0;
  loutput.shmem_output_addr = 0;

  loutput.legend = 1;
  loutput.legend_append = NULL;
  loutput.legend_append_nr = 0;

  loutput.show_distances_only = 0;
  loutput.show_only = HWLOC_OBJ_TYPE_NONE;
  loutput.show_cpuset = 0;
  loutput.show_taskset = 0;

  loutput.backend_data = NULL;
  loutput.methods = NULL;

  loutput.no_half_lines = 0;
  loutput.plain_children_order = 0;
  loutput.fontsize = 10;
  loutput.gridsize = 7;
  loutput.linespacing = 4;

  loutput.text_xscale = 1.0f;
  env = getenv("LSTOPO_TEXT_XSCALE");
  if (env)
    loutput.text_xscale = (float) atof(env);

  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
    loutput.force_orient[i] = LSTOPO_ORIENT_NONE;
  loutput.force_orient[HWLOC_OBJ_PU] = LSTOPO_ORIENT_HORIZ;
  for(i=HWLOC_OBJ_L1CACHE; i<=HWLOC_OBJ_L3ICACHE; i++)
    loutput.force_orient[i] = LSTOPO_ORIENT_HORIZ;
  loutput.force_orient[HWLOC_OBJ_NUMANODE] = LSTOPO_ORIENT_HORIZ;
  loutput.force_orient[HWLOC_OBJ_MEMCACHE] = LSTOPO_ORIENT_HORIZ;
  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++) {
    loutput.show_indexes[i] = 1;
    loutput.show_attrs[i] = 1;
    loutput.show_text[i] = 1;
  }
  loutput.show_attrs_enabled = 1;
  loutput.show_text_enabled = 1;

  loutput.show_binding = 1;
  loutput.show_disallowed = 1;

  /* enable verbose backends */
  if (!getenv("HWLOC_XML_VERBOSE"))
    putenv((char *) "HWLOC_XML_VERBOSE=1");
  if (!getenv("HWLOC_SYNTHETIC_VERBOSE"))
    putenv((char *) "HWLOC_SYNTHETIC_VERBOSE=1");

  /* Use localized time prints, and utf-8 characters in the ascii output */
#ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif

  loutput.cpubind_set = hwloc_bitmap_alloc();
  loutput.membind_set = hwloc_bitmap_alloc();
  if (!loutput.cpubind_set || !loutput.membind_set)
    goto out;

  err = hwloc_topology_init (&topology);
  if (err)
    goto out;
  hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
  hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);

  while (argc >= 1)
    {
      opt = 0;
      if (!strcmp (argv[0], "-v") || !strcmp (argv[0], "--verbose")) {
	loutput.verbose_mode++;
      } else if (!strcmp (argv[0], "-s") || !strcmp (argv[0], "--silent")) {
	loutput.verbose_mode--;
      } else if (!strcmp (argv[0], "--distances")) {
	loutput.show_distances_only = 1;
      } else if (!strcmp (argv[0], "-h") || !strcmp (argv[0], "--help")) {
	usage(callname, stdout);
        exit(EXIT_SUCCESS);
      } else if (!strcmp (argv[0], "-f") || !strcmp (argv[0], "--force"))
	loutput.overwrite = 1;
      else if (!strcmp (argv[0], "-l") || !strcmp (argv[0], "--logical"))
	loutput.index_type = LSTOPO_INDEX_TYPE_LOGICAL;
      else if (!strcmp (argv[0], "-p") || !strcmp (argv[0], "--physical"))
	loutput.index_type = LSTOPO_INDEX_TYPE_PHYSICAL;
      else if (!strcmp (argv[0], "-c") || !strcmp (argv[0], "--cpuset"))
	loutput.show_cpuset = 1;
      else if (!strcmp (argv[0], "-C") || !strcmp (argv[0], "--cpuset-only"))
	loutput.show_cpuset = 2;
      else if (!strcmp (argv[0], "--taskset")) {
	loutput.show_taskset = 1;
	if (!loutput.show_cpuset)
	  loutput.show_cpuset = 1;
      } else if (!strcmp (argv[0], "--only")) {
	if (argc < 2)
	  goto out_usagefailure;
        if (hwloc_type_sscanf(argv[1], &loutput.show_only, NULL, 0) < 0)
	  fprintf(stderr, "Unsupported type `%s' passed to --only, ignoring.\n", argv[1]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--filter")) {
	hwloc_obj_type_t type = HWLOC_OBJ_TYPE_NONE;
	char *colon;
	enum hwloc_type_filter_e filter = HWLOC_TYPE_FILTER_KEEP_ALL;
	int all = 0;
	int allio = 0;
	int allcaches = 0;
	int allicaches = 0;
	if (argc < 2)
	  goto out_usagefailure;
	colon = strchr(argv[1], ':');
	if (colon) {
	  *colon = '\0';
	  if (!strcmp(colon+1, "none"))
	    filter = HWLOC_TYPE_FILTER_KEEP_NONE;
	  else if (!strcmp(colon+1, "all"))
	    filter = HWLOC_TYPE_FILTER_KEEP_ALL;
	  else if (!strcmp(colon+1, "structure"))
	    filter = HWLOC_TYPE_FILTER_KEEP_STRUCTURE;
	  else if (!strcmp(colon+1, "important"))
	    filter = HWLOC_TYPE_FILTER_KEEP_IMPORTANT;
	  else {
	    fprintf(stderr, "Unsupported filtering kind `%s' passed to --filter.\n", colon+1);
	    goto out_usagefailure;
	  }
	}
	if (!strcmp(argv[1], "all"))
	  all = 1;
	else if (!strcmp(argv[1], "io"))
	  allio = 1;
	else if (!strcmp(argv[1], "cache"))
	  allcaches = 1;
	else if (!strcmp(argv[1], "icache"))
	  allicaches = 1;
	else if (hwloc_type_sscanf(argv[1], &type, NULL, 0) < 0) {
	  fprintf(stderr, "Unsupported type `%s' passed to --filter.\n", argv[1]);
	  goto out_usagefailure;
	}
	if (type == HWLOC_OBJ_PU) {
	  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
	    loutput.ignore_pus = 1;
	}
	else if (type == HWLOC_OBJ_NUMANODE) {
	  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
	    loutput.ignore_numanodes = 1;
	}
	else if (all)
	  hwloc_topology_set_all_types_filter(topology, filter);
	else if (allio)
	  hwloc_topology_set_io_types_filter(topology, filter);
	else if (allcaches) {
	  hwloc_topology_set_cache_types_filter(topology, filter);
	  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_MEMCACHE, filter);
	} else if (allicaches)
	  hwloc_topology_set_icache_types_filter(topology, filter);
	else
	  hwloc_topology_set_type_filter(topology, type, filter);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--ignore")) {
	hwloc_obj_type_t type;
	if (argc < 2)
	  goto out_usagefailure;
	if (!strcasecmp(argv[1], "cache")) {
	  fprintf(stderr, "--ignore Cache not supported anymore, use --no-caches instead.\n");
	  goto out_usagefailure;
	}
	if (hwloc_type_sscanf(argv[1], &type, NULL, 0) < 0)
	  fprintf(stderr, "Unsupported type `%s' passed to --ignore, ignoring.\n", argv[1]);
	else if (type == HWLOC_OBJ_PU)
	  loutput.ignore_pus = 1;
	else if (type == HWLOC_OBJ_NUMANODE)
	  loutput.ignore_numanodes = 1;
	else
	  hwloc_topology_set_type_filter(topology, type, HWLOC_TYPE_FILTER_KEEP_NONE);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--no-smt")) {
	loutput.ignore_pus = 1;
      }
      else if (!strcmp (argv[0], "--no-caches")) {
	hwloc_topology_set_cache_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_MEMCACHE, HWLOC_TYPE_FILTER_KEEP_NONE);
      }
      else if (!strcmp (argv[0], "--no-useless-caches")) {
	hwloc_topology_set_cache_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_MEMCACHE, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
      }
      else if (!strcmp (argv[0], "--no-icaches")) {
	hwloc_topology_set_icache_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_NONE);
      }
      else if (!strcmp (argv[0], "--disallowed") || !strcmp (argv[0], "--whole-system"))
	flags |= HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED;
      else if (!strcmp (argv[0], "--allow")) {
	if (argc < 2)
	  goto out_usagefailure;
	if (!strcmp(argv[1], "all")) {
	  allow_flags = HWLOC_ALLOW_FLAG_ALL;
	} else if (!strcmp(argv[1], "local")) {
	  allow_flags = HWLOC_ALLOW_FLAG_LOCAL_RESTRICTIONS;
	  flags |= HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM;
	} else {
	  hwloc_bitmap_t set = hwloc_bitmap_alloc();
	  const char *begin = argv[1];
	  if (!strncmp(begin, "nodeset=", 8))
	    begin += 8;
	  hwloc_bitmap_sscanf(set, begin);
	  if (begin == argv[1])
	    allow_cpuset = set;
	  else
	    allow_nodeset = set;
	  allow_flags = HWLOC_ALLOW_FLAG_CUSTOM;
	}
	opt = 1;
	flags |= HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED;

      } else if (!strcmp (argv[0], "--no-io")) {
	hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_NONE);
      } else if (!strcmp (argv[0], "--no-bridges")) {
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_BRIDGE, HWLOC_TYPE_FILTER_KEEP_NONE);
      } else if (!strcmp (argv[0], "--whole-io")) {
	hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
      } else if (!strcmp (argv[0], "--merge")) {
	hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
      }
      else if (!strcmp (argv[0], "--no-collapse"))
	loutput.pci_collapse_enabled = 0;

      else if (!strcmp (argv[0], "--no-factorize")) {
	for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	  loutput.factorize_min[i] = FACTORIZE_MIN_DISABLED;
      }
      else if (!strncmp (argv[0], "--no-factorize=", 15)) {
	hwloc_obj_type_t type;
	const char *tmp = argv[0]+15;
	if (hwloc_type_sscanf(tmp, &type, NULL, 0) < 0) {
	  fprintf(stderr, "Unsupported parameter `%s' passed to %s, ignoring.\n", tmp, argv[0]);
	  goto out_usagefailure;
	}
	loutput.factorize_min[type] = FACTORIZE_MIN_DISABLED;
      }
      else if (!strcmp (argv[0], "--factorize")) {
	for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	  loutput.factorize_min[i] = FACTORIZE_MIN_DEFAULT;
	lstopo_update_factorize_alltypes_bounds(&loutput);
      }
      else if (!strncmp (argv[0], "--factorize=", 12)) {
	hwloc_obj_type_t type, type_min, type_max;
	unsigned min, first, last;
	const char *tmp = argv[0]+12;
	const char *sep1, *sep2, *sep3;

	if (*tmp < '0' || *tmp > '9') {
	  if (hwloc_type_sscanf(tmp, &type, NULL, 0) < 0) {
	    fprintf(stderr, "Unsupported type `%s' passed to %s, ignoring.\n", tmp, argv[0]);
	    goto out_usagefailure;
	  }
	  type_min = type;
	  type_max = type+1;
	  sep1 = strchr(tmp, ',');
	} else {
	  type_min = HWLOC_OBJ_TYPE_MIN;
	  type_max = HWLOC_OBJ_TYPE_MAX;
	  sep1 = tmp-1;
	}

	if (!sep1) {
	  min = FACTORIZE_MIN_DEFAULT;
	  lstopo_update_factorize_bounds(min, &first, &last);
	} else {
	  min = atoi(sep1+1);
	  lstopo_update_factorize_bounds(min, &first, &last);
	  sep2 = strchr(sep1+1, ',');
	  if (sep2) {
	    first = atoi(sep2+1);
	    sep3 = strchr(sep2+1, ',');
	    if (sep3)
	      last = atoi(sep3+1);
	  }
	}

	for(i=type_min; i<type_max; i++) {
	  loutput.factorize_min[i] = min;
	  loutput.factorize_first[i] = first;
	  loutput.factorize_last[i] = last;
	}
      }

      else if (!strcmp (argv[0], "--thissystem"))
	flags |= HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM;
      else if (!strcmp (argv[0], "--flags")) {
	if (argc < 2)
	  goto out_usagefailure;
	flags = strtoul(argv[1], NULL, 0);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--restrict")) {
	if (argc < 2)
	  goto out_usagefailure;
	restrictstring = strdup(argv[1]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--restrict-flags")) {
	if (argc < 2)
	  goto out_usagefailure;
	restrict_flags = (unsigned long) strtoull(argv[1], NULL, 0);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--export-xml-flags")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.export_xml_flags = (unsigned long) strtoull(argv[1], NULL, 0);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--export-synthetic-flags")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.export_synthetic_flags = (unsigned long) strtoull(argv[1], NULL, 0);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--horiz"))
	for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	  loutput.force_orient[i] = LSTOPO_ORIENT_HORIZ;
      else if (!strcmp (argv[0], "--vert"))
	for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	  loutput.force_orient[i] = LSTOPO_ORIENT_VERT;
      else if (!strcmp (argv[0], "--rect"))
	for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	  loutput.force_orient[i] = LSTOPO_ORIENT_RECT;
      else if (!strncmp (argv[0], "--horiz=", 8)
	       || !strncmp (argv[0], "--vert=", 7)
	       || !strncmp (argv[0], "--rect=", 7)) {
	enum lstopo_orient_e orient = (argv[0][2] == 'h') ? LSTOPO_ORIENT_HORIZ : (argv[0][2] == 'v') ? LSTOPO_ORIENT_VERT : LSTOPO_ORIENT_RECT;
	char *tmp = argv[0] + ((argv[0][2] == 'h') ? 8 : 7);
	while (tmp) {
	  char *end = strchr(tmp, ',');
	  hwloc_obj_type_t type;
	  if (end)
	    *end = '\0';
	  if (hwloc_type_sscanf(tmp, &type, NULL, 0) < 0)
	    fprintf(stderr, "Unsupported type `%s' passed to %s, ignoring.\n", tmp, argv[0]);
	  else
	    loutput.force_orient[type] = orient;
	  if (!end)
	    break;
	  tmp = end+1;
        }
      }

      else if (!strcmp (argv[0], "--binding-color")) {
	if (argc < 2)
	  goto out_usagefailure;
	if (!strcmp(argv[1], "none"))
	  loutput.show_binding = 0;
	else
	  fprintf(stderr, "Unsupported color `%s' passed to %s, ignoring.\n", argv[1], argv[0]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--disallowed-color")) {
	if (argc < 2)
	  goto out_usagefailure;
	if (!strcmp(argv[1], "none"))
	  loutput.show_disallowed = 0;
	else
	  fprintf(stderr, "Unsupported color `%s' passed to %s, ignoring.\n", argv[1], argv[0]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--top-color")) {
	if (argc < 2)
	  goto out_usagefailure;
	task_background_color_string = argv[1];
      }
      else if (!strncmp (argv[0], "--no-text", 9)
	       || !strncmp (argv[0], "--text", 6)
	       || !strncmp (argv[0], "--no-index", 10)
	       || !strncmp (argv[0], "--index", 7)
	       || !strncmp (argv[0], "--no-attrs", 10)
	       || !strncmp (argv[0], "--attrs", 7)) {
	int enable = argv[0][2] != 'n';
	const char *kind = enable ? argv[0]+2 : argv[0]+5;
	const char *end;
	int *array;
	if (*kind == 't') {
	  array = loutput.show_text;
	  end = kind+4;
	} else if (*kind == 'a') {
	  array = loutput.show_attrs;
	  end = kind+5;
	} else if (*kind == 'i') {
	  array = loutput.show_indexes;
	  end = kind+5;
	} else {
	  abort();
	}
	if (!*end) {
	  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	    array[i] = enable;

	} else if (*end == '=') {
	  const char *tmp = end+1;
	  while (tmp) {
	    char *sep = strchr(tmp, ',');
	    hwloc_obj_type_t type;
	    if (sep)
	      *sep = '\0';
	    if (hwloc_type_sscanf(tmp, &type, NULL, 0) < 0)
	      if (!hwloc_strncasecmp(tmp, "cache", 5)) {
		for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
		  if (hwloc_obj_type_is_cache(i))
		    array[i] = enable;
	      } else if (!hwloc_strncasecmp(tmp, "io", 2)) {
		for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
		  if (hwloc_obj_type_is_io(i))
		    array[i] = enable;
	      } else
		fprintf(stderr, "Unsupported type `%s' passed to %s, ignoring.\n", tmp, argv[0]);
	    else
	      array[type] = enable;
	    if (!sep)
	      break;
	    tmp = sep+1;
	  }

	} else {
	  fprintf(stderr, "Unexpected character %c in option %s\n", *end, argv[0]);
	  goto out_usagefailure;
	}
      }

      else if (!strcmp (argv[0], "--children-order")) {
	if (argc < 2)
	  goto out_usagefailure;
	if (!strcmp(argv[1], "plain"))
	  loutput.plain_children_order = 1;
	else if (strcmp(argv[1], "memoryabove"))
	  fprintf(stderr, "Unsupported order `%s' passed to %s, ignoring.\n", argv[1], argv[0]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--fontsize")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.fontsize = atoi(argv[1]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--gridsize")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.gridsize = atoi(argv[1]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--linespacing")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.linespacing = atoi(argv[1]);
	opt = 1;
      }
      else if (!strcmp (argv[0], "--no-legend")) {
	loutput.legend = 0;
      }
      else if (!strcmp (argv[0], "--append-legend")) {
	char **tmp;
	if (argc < 2)
	  goto out_usagefailure;
	tmp = realloc(loutput.legend_append, (loutput.legend_append_nr+1) * sizeof(*loutput.legend_append));
	if (!tmp) {
	  fprintf(stderr, "Failed to realloc legend append array, legend ignored.\n");
	} else {
	  loutput.legend_append = tmp;
	  loutput.legend_append[loutput.legend_append_nr] = strdup(argv[1]);
	  loutput.legend_append_nr++;
	}
	opt = 1;
      }

      else if (!strcmp (argv[0], "--shmem-output-addr")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.shmem_output_addr = strtoull(argv[1], NULL, 0);
	opt = 1;
      }

      else if (hwloc_utils_lookup_input_option(argv, argc, &opt,
					       &input, &input_format,
					       callname)) {
	/* nothing to do anymore */

      } else if (!strcmp (argv[0], "--pid")) {
	if (argc < 2)
	  goto out_usagefailure;
	loutput.pid_number = atoi(argv[1]); opt = 1;
      } else if (!strcmp (argv[0], "--ps") || !strcmp (argv[0], "--top"))
        top = 1;
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", callname, HWLOC_VERSION);
          exit(EXIT_SUCCESS);
      } else if (!strcmp (argv[0], "--output-format") || !strcmp (argv[0], "--of")) {
	if (argc < 2)
	  goto out_usagefailure;
        output_format = parse_output_format(argv[1], callname);
        opt = 1;
      } else {
	if (filename) {
	  fprintf (stderr, "Unrecognized option: %s\n", argv[0]);
	  goto out_usagefailure;
	} else
	  filename = argv[0];
      }
      argc -= opt+1;
      argv += opt+1;
    }

  if (!loutput.fontsize) {
    for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
      loutput.show_text[i] = 0;
    loutput.legend = 0;
  }

  err = hwloc_topology_set_flags(topology, flags);
  if (err < 0) {
    fprintf(stderr, "Failed to set flags %lx (%s).\n", flags, strerror(errno));
    goto out_with_topology;
  }

  if (input) {
    err = hwloc_utils_enable_input_format(topology, flags, input, &input_format, loutput.verbose_mode > 1, callname);
    if (err)
      goto out_with_topology;
  }

  if (loutput.pid_number > 0) {
    if (hwloc_pid_from_number(&loutput.pid, loutput.pid_number, 0, 1 /* verbose */) < 0
	|| hwloc_topology_set_pid(topology, loutput.pid)) {
      perror("Setting target pid");
      goto out_with_topology;
    }
  }

  /* if the output format wasn't enforced, look at the filename */
  if (filename && output_format == LSTOPO_OUTPUT_DEFAULT) {
    if (!strcmp(filename, "-")
	|| !strcmp(filename, "/dev/stdout")) {
      output_format = LSTOPO_OUTPUT_CONSOLE;
    } else {
      char *dot = strrchr(filename, '.');
      if (dot)
        output_format = parse_output_format(dot+1, callname);
      else {
	fprintf(stderr, "Cannot infer output type for file `%s' without any extension, using default output.\n", filename);
	filename = NULL;
      }
    }
  }
  if (output_format == LSTOPO_OUTPUT_ERROR)
    goto out_usagefailure;

  /* if  the output format wasn't enforced, think a bit about what the user probably want */
  if (output_format == LSTOPO_OUTPUT_DEFAULT) {
    if (loutput.show_cpuset
        || loutput.show_only != HWLOC_OBJ_TYPE_NONE
	|| loutput.show_distances_only
        || loutput.verbose_mode != LSTOPO_VERBOSE_MODE_DEFAULT)
      output_format = LSTOPO_OUTPUT_CONSOLE;
  }

  if (input_format == HWLOC_UTILS_INPUT_XML
      && output_format == LSTOPO_OUTPUT_XML) {
    /* must be after parsing output format and before loading the topology */
    putenv((char *) "HWLOC_XML_USERDATA_NOT_DECODED=1");
    hwloc_topology_set_userdata_import_callback(topology, hwloc_utils_userdata_import_cb);
    hwloc_topology_set_userdata_export_callback(topology, hwloc_utils_userdata_export_cb);
  }

#ifdef HAVE_CLOCK_GETTIME
  if (measure_load_time)
    clock_gettime(CLOCK_MONOTONIC, &ts1);
#endif

  if (input_format == HWLOC_UTILS_INPUT_SHMEM) {
#ifdef HWLOC_WIN_SYS
    fprintf(stderr, "shmem topology not supported\n"); /* this line must match the grep line in test-lstopo-shmem */
    goto out;
#else /* !HWLOC_WIN_SYS */
    /* load from shmem, and duplicate onto topology, so that we may modify it */
    hwloc_topology_destroy(topology);
    err = lstopo_shmem_adopt(input, &topology);
    if (err < 0)
      goto out;
    hwloc_utils_userdata_clear_recursive(hwloc_get_root_obj(topology));
#endif /* !HWLOC_WIN_SYS */

  } else {
    /* normal load */
    err = hwloc_topology_load (topology);
    if (err) {
      fprintf(stderr, "hwloc_topology_load() failed (%s).\n", strerror(errno));
      goto out_with_topology;
    }
  }

  if (allow_flags) {
    if (allow_flags == HWLOC_ALLOW_FLAG_CUSTOM) {
      err = hwloc_topology_allow(topology, allow_cpuset, allow_nodeset, HWLOC_ALLOW_FLAG_CUSTOM);
    } else {
      err = hwloc_topology_allow(topology, NULL, NULL, allow_flags);
    }
    if (err < 0) {
      fprintf(stderr, "hwloc_topology_allow() failed (%s)\n", strerror(errno));
      goto out_with_topology;
    }
  }

  hwloc_bitmap_fill(loutput.cpubind_set);
  if (loutput.pid_number != -1 && loutput.pid_number != 0)
    hwloc_get_proc_cpubind(topology, loutput.pid, loutput.cpubind_set, 0);
  else
    /* get our binding even if --pid not given, it may be used by --restrict */
    hwloc_get_cpubind(topology, loutput.cpubind_set, 0);

  hwloc_bitmap_fill(loutput.membind_set);
  if (loutput.pid_number != -1 && loutput.pid_number != 0)
    hwloc_get_proc_membind(topology, loutput.pid, loutput.membind_set, &policy, HWLOC_MEMBIND_BYNODESET);
  else
    /* get our binding even if --pid not given, it may be used by --restrict */
    hwloc_get_membind(topology, loutput.membind_set, &policy, HWLOC_MEMBIND_BYNODESET);

#ifdef HAVE_CLOCK_GETTIME
  if (measure_load_time) {
    clock_gettime(CLOCK_MONOTONIC, &ts2);
    ms = (ts2.tv_nsec-ts1.tv_nsec)/1000000+(ts2.tv_sec-ts1.tv_sec)*1000UL;
    printf("hwloc_topology_load() took %lu ms\n", ms);
  }
#endif

  loutput.need_pci_domain = lstopo_check_pci_domains(topology);

  if (top)
    add_process_objects(topology);

  if (restrictstring) {
    hwloc_bitmap_t restrictset = hwloc_bitmap_alloc();
    if (!strcmp (restrictstring, "binding")) {
      hwloc_bitmap_copy(restrictset, loutput.cpubind_set);
    } else {
      hwloc_bitmap_sscanf(restrictset, restrictstring);
    }
    err = hwloc_topology_restrict (topology, restrictset, restrict_flags);
    if (err) {
      perror("Restricting the topology");
      /* FALLTHRU */
    }
    hwloc_bitmap_free(restrictset);
    free(restrictstring);
  }

  switch (output_format) {
  case LSTOPO_OUTPUT_DEFAULT:
#ifdef LSTOPO_HAVE_GRAPHICS
#if (defined LSTOPO_HAVE_X11)
    if (getenv("DISPLAY")) {
      output_func = output_x11;
    } else
#endif /* LSTOPO_HAVE_X11 */
#ifdef HWLOC_WIN_SYS
    {
      output_func = output_windows;
    }
#endif
#endif /* !LSTOPO_HAVE_GRAPHICS */
#if !defined HWLOC_WIN_SYS || !defined LSTOPO_HAVE_GRAPHICS
    {
      output_func = output_console;
    }
#endif
    break;

  case LSTOPO_OUTPUT_CONSOLE:
    output_func = output_console;
    break;
  case LSTOPO_OUTPUT_SYNTHETIC:
    output_func = output_synthetic;
    break;
  case LSTOPO_OUTPUT_ASCII:
    output_func = output_ascii;
    break;
  case LSTOPO_OUTPUT_FIG:
    output_func = output_fig;
    break;
#ifdef LSTOPO_HAVE_GRAPHICS
# ifdef CAIRO_HAS_PNG_FUNCTIONS
  case LSTOPO_OUTPUT_PNG:
    output_func = output_png;
    break;
# endif /* CAIRO_HAS_PNG_FUNCTIONS */
# ifdef CAIRO_HAS_PDF_SURFACE
  case LSTOPO_OUTPUT_PDF:
    output_func = output_pdf;
    break;
# endif /* CAIRO_HAS_PDF_SURFACE */
# ifdef CAIRO_HAS_PS_SURFACE
  case LSTOPO_OUTPUT_PS:
    output_func = output_ps;
    break;
# endif /* CAIRO_HAS_PS_SURFACE */
# ifdef CAIRO_HAS_SVG_SURFACE
  case LSTOPO_OUTPUT_SVG:
  case LSTOPO_OUTPUT_CAIROSVG:
    output_func = output_cairosvg;
    break;
# endif /* CAIRO_HAS_SVG_SURFACE */
#endif /* LSTOPO_HAVE_GRAPHICS */
#if !(defined LSTOPO_HAVE_GRAPHICS) || !(defined CAIRO_HAS_SVG_SURFACE)
  case LSTOPO_OUTPUT_SVG:
#endif
  case LSTOPO_OUTPUT_NATIVESVG:
    output_func = output_nativesvg;
    break;
  case LSTOPO_OUTPUT_XML:
    output_func = output_xml;
    break;
#ifndef HWLOC_WIN_SYS
  case LSTOPO_OUTPUT_SHMEM:
    output_func = output_shmem;
    break;
#endif
  default:
    fprintf(stderr, "file format not supported\n");
    goto out_usagefailure;
  }

  loutput.topology = topology;
  loutput.depth = hwloc_topology_get_depth(topology);
  loutput.file = NULL;

  if (output_format != LSTOPO_OUTPUT_XML) {
    /* there might be some xml-imported userdata in objects, add lstopo-specific userdata in front of them */
    lstopo_populate_userdata(hwloc_get_root_obj(topology));
    lstopo_add_factorized_attributes(&loutput, hwloc_get_root_obj(topology));
    lstopo_add_collapse_attributes(topology);
  }

  err = output_func(&loutput, filename);

  if (output_format != LSTOPO_OUTPUT_XML) {
    /* remove lstopo-specific userdata in front of the list of userdata */
    lstopo_destroy_userdata(hwloc_get_root_obj(topology));
  }
  /* remove the remaining lists of xml-imported userdata */
  hwloc_utils_userdata_free_recursive(hwloc_get_root_obj(topology));

  hwloc_topology_destroy (topology);

  for(i=0; i<loutput.legend_append_nr; i++)
    free(loutput.legend_append[i]);
  free(loutput.legend_append);

  hwloc_bitmap_free(loutput.cpubind_set);
  hwloc_bitmap_free(loutput.membind_set);

  return err ? EXIT_FAILURE : EXIT_SUCCESS;

 out_usagefailure:
  usage (callname, stderr);
 out_with_topology:
  lstopo_destroy_userdata(hwloc_get_root_obj(topology));
  hwloc_topology_destroy(topology);
 out:
  hwloc_bitmap_free(allow_cpuset);
  hwloc_bitmap_free(allow_nodeset);
  hwloc_bitmap_free(loutput.cpubind_set);
  hwloc_bitmap_free(loutput.membind_set);
  return EXIT_FAILURE;
}
