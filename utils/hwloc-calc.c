/*
 * Copyright © 2009 CNRS
 * Copyright © 2009 INRIA
 * Copyright © 2009-2011 Université Bordeaux 1
 * Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/private.h>
#include <hwloc-calc.h>
#include <hwloc.h>

#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void usage(const char *callname __hwloc_attribute_unused, FILE *where)
{
  fprintf(where, "Usage: hwloc-calc [options] <location> ...\n");
  fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
  fprintf(where, "            as supported by the hwloc-bind utility.\n");
  fprintf(where, "Options:\n");
  fprintf(where, "  -l --logical              Use logical object indexes (default)\n");
  fprintf(where, "  -p --physical             Use physical object indexes\n");
  fprintf(where, "  --li --logical-input      Use logical indexes for input (default)\n");
  fprintf(where, "  --lo --logical-output     Use logical indexes for output (default)\n");
  fprintf(where, "  --pi --physical-input     Use physical indexes for input\n");
  fprintf(where, "  --po --physical-output    Use physical indexes for output\n");
  fprintf(where, "  --intersect <type|depth>  Report the list of object indexes intersecting the CPU set\n");
  fprintf(where, "  --largest                 Report the list of largest objects in the CPU set\n");
  fprintf(where, "  --single                  Singlify the output to a single CPU\n");
  fprintf(where, "  --taskset                 Manipulate taskset-specific cpuset strings\n");
  hwloc_utils_input_format_usage(where, 10);
  fprintf(where, "  -v                        Show verbose messages\n");
  fprintf(where, "  --version                 Report version and exit\n");
}

static int verbose = 0;
static int logicali = 1;
static int logicalo = 1;
static int listdepth = -1;
static int showobjs = 0;
static int singlify = 0;
static int taskset = 0;

static int
hwloc_calc_output(hwloc_topology_t topology, hwloc_bitmap_t set)
{
  if (singlify)
    hwloc_bitmap_singlify(set);

  if (showobjs) {
    hwloc_bitmap_t remaining = hwloc_bitmap_dup(set);
    int first = 1;
    while (!hwloc_bitmap_iszero(remaining)) {
      char type[64];
      unsigned idx;
      hwloc_obj_t obj = hwloc_get_first_largest_obj_inside_cpuset(topology, remaining);
      if (!obj) {
        fprintf(stderr, "No object included in this cpuset\n");
        return EXIT_FAILURE;
      }
      hwloc_obj_type_snprintf(type, sizeof(type), obj, 1);
      idx = logicalo ? obj->logical_index : obj->os_index;
      if (idx == (unsigned) -1)
        printf("%s%s", first ? "" : " ", type);
      else
        printf("%s%s:%u", first ? "" : " ", type, idx);
      hwloc_bitmap_andnot(remaining, remaining, obj->cpuset);
      first = 0;
    }
    printf("\n");
    hwloc_bitmap_free(remaining);
  } else if (listdepth != -1) {
    hwloc_obj_t proc, prev = NULL;
    while ((proc = hwloc_get_next_obj_covering_cpuset_by_depth(topology, set, listdepth, prev)) != NULL) {
      if (prev)
	printf(",");
      printf("%u", logicalo ? proc->logical_index : proc->os_index);
      prev = proc;
    }
    printf("\n");
  } else {
    char *string = NULL;
    if (taskset)
      hwloc_bitmap_taskset_asprintf(&string, set);
    else
      hwloc_bitmap_asprintf(&string, set);
    printf("%s\n", string);
    free(string);
  }
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
  hwloc_topology_t topology;
  char *input = NULL;
  enum hwloc_utils_input_format input_format = HWLOC_UTILS_INPUT_DEFAULT;
  int input_changed = 0;
  unsigned depth;
  hwloc_bitmap_t set;
  int cmdline_args = 0;
  char **orig_argv = argv;
  hwloc_obj_type_t listtype = (hwloc_obj_type_t) -1;
  char *callname;
  int opt;
  int ret = EXIT_SUCCESS;

  callname = argv[0];

  set = hwloc_bitmap_alloc();

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);

  while (argc >= 2) {
    if (*argv[1] == '-') {
      if (!strcmp(argv[1], "-v")) {
        verbose = 1;
        goto next;
      }
      if (!strcmp(argv[1], "--help")) {
	usage(callname, stdout);
	return EXIT_SUCCESS;
      }
      if (!strcmp(argv[1], "--intersect")) {
	if (argc <= 2) {
	  usage(callname, stderr);
	  return EXIT_SUCCESS;
	}
	listtype = hwloc_obj_type_of_string(argv[2]);
	if (listtype == (hwloc_obj_type_t) -1) {
	  char *endptr;
	  unsigned _depth = strtoul(argv[2], &endptr, 0);
	  if (*endptr) {
	    fprintf(stderr, "unrecognized list type or depth %s\n", argv[2]);
	    usage(callname, stderr);
	    return EXIT_SUCCESS;
	  }
	  listdepth = _depth;
	}
	argv++;
	argc--;
	goto next;
      }
      if (!strcasecmp(argv[1], "--pulist") || !strcmp(argv[1], "--proclist")) {
	/* backward compat with 1.0 */
	listtype = HWLOC_OBJ_PU;
        goto next;
      }
      if (!strcmp(argv[1], "--nodelist")) {
	/* backward compat with 1.0 */
	listtype = HWLOC_OBJ_NODE;
        goto next;
      }
      if (!strcmp(argv[1], "--largest")  || !strcmp(argv[1], "--objects") /* backward compat with 1.0 */) {
	showobjs = 1;
        goto next;
      }
      if (!strcmp(argv[1], "--version")) {
        printf("%s %s\n", orig_argv[0], VERSION);
        exit(EXIT_SUCCESS);
      }
      if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--logical")) {
	logicali = 1;
	logicalo = 1;
	goto next;
      }
      if (!strcmp(argv[1], "--li") || !strcmp(argv[1], "--logical-input")) {
	logicali = 1;
	goto next;
      }
      if (!strcmp(argv[1], "--lo") || !strcmp(argv[1], "--logical-output")) {
	logicalo = 1;
	goto next;
      }
      if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--physical")) {
	logicali = 0;
	logicalo = 0;
	goto next;
      }
      if (!strcmp(argv[1], "--pi") || !strcmp(argv[1], "--physical-input")) {
	logicali = 0;
	goto next;
      }
      if (!strcmp(argv[1], "--po") || !strcmp(argv[1], "--physical-output")) {
	logicalo = 0;
	goto next;
      }
      if (!strcmp(argv[1], "--single")) {
	singlify = 1;
	goto next;
      }
      if (!strcmp(argv[1], "--taskset")) {
	taskset = 1;
	goto next;
      }
      if (hwloc_utils_lookup_input_option(argv+1, argc, &opt,
					  &input, &input_format,
					  callname)) {
	argv += opt;
	argc -= opt;
	input_changed = 1;
	goto next;
      }

      fprintf (stderr, "Unrecognized option: %s\n", argv[1]);
      usage(callname, stderr);
      return EXIT_FAILURE;
    }

    if (input_changed && input) {
      hwloc_utils_enable_input_format(topology, input, input_format, verbose, callname);
      hwloc_topology_load(topology);
      depth = hwloc_topology_get_depth(topology);
      input_changed = 0;
    }

    cmdline_args++;
    if (hwloc_mask_process_arg(topology, depth, argv[1], logicali, set, taskset, verbose) < 0)
      fprintf(stderr, "ignored unrecognized argument %s\n", argv[1]);

 next:
    argc--;
    argv++;
  }

  if (listtype != (hwloc_obj_type_t) -1) {
    listdepth = hwloc_get_type_depth(topology, listtype);
    if (listdepth == HWLOC_TYPE_DEPTH_UNKNOWN) {
      fprintf(stderr, "unavailable list type %s\n", hwloc_obj_type_string(listtype));
      goto out;
    } else  if (listdepth == HWLOC_TYPE_DEPTH_MULTIPLE) {
      fprintf(stderr, "cannot list type %s with multiple depth, please use the relevant depth directly\n", hwloc_obj_type_string(listtype));
      goto out;
    }
  }

  if (cmdline_args) {
    /* process command-line arguments */
    ret = hwloc_calc_output(topology, set);

  } else {
    /* process stdin arguments line-by-line */
#define HWLOC_CALC_LINE_LEN 1024
    char line[HWLOC_CALC_LINE_LEN];
    while (fgets(line, sizeof(line), stdin)) {
      char *current = line;
      hwloc_bitmap_zero(set);
      while (1) {
	char *token = strtok(current, " \n");
	if (!token)
	  break;
	current = NULL;
	if (hwloc_mask_process_arg(topology, depth, token, logicali, set, taskset, verbose) < 0)
	  fprintf(stderr, "ignored unrecognized argument %s\n", argv[1]);
      }
      hwloc_calc_output(topology, set);
    }
  }

 out:
  hwloc_topology_destroy(topology);

  hwloc_bitmap_free(set);

  return ret;
}
