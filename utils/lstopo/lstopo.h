/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2016 Inria.  All rights reserved.
 * Copyright © 2009-2010, 2012, 2015 Université Bordeaux
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef UTILS_LSTOPO_H
#define UTILS_LSTOPO_H

#include <private/autogen/config.h>
#include <hwloc.h>

enum lstopo_orient_e {
  LSTOPO_ORIENT_NONE = 0,
  LSTOPO_ORIENT_HORIZ,
  LSTOPO_ORIENT_VERT,
  LSTOPO_ORIENT_RECT
};

FILE *open_output(const char *filename, int overwrite) __hwloc_attribute_malloc;

struct draw_methods;

/* if embedded in backend-specific output structure, must be at the beginning */
struct lstopo_output {
  hwloc_topology_t topology;

  /* file config */
  FILE *file;
  int overwrite;

  /* misc config */
  int logical;
  int verbose_mode;
  int ignore_pus;
  int collapse;
  int pid_number;
  hwloc_pid_t pid;

  /* synthetic export config */
  unsigned long export_synthetic_flags;

  /* legend */
  int legend;
  char ** legend_append;
  unsigned legend_append_nr;

  /* text config */
  hwloc_obj_type_t show_only;
  int show_cpuset;
  int show_taskset;

  /* draw config */
  unsigned int gridsize, fontsize;
  enum lstopo_orient_e force_orient[HWLOC_OBJ_TYPE_MAX]; /* orientation of children within an object of the given type */
  struct draw_methods *methods;
  unsigned min_pu_textwidth;
};

struct lstopo_obj_userdata {
  /* draw info */
  unsigned width;
  unsigned height;
  unsigned fontsize;
  unsigned gridsize;
};

typedef void output_method (struct lstopo_output *output, const char *filename);
extern output_method output_console, output_synthetic, output_ascii, output_x11, output_fig, output_png, output_pdf, output_ps, output_svg, output_windows, output_xml;

struct draw_methods {
  void (*init) (void *output);
  void (*declare_color) (void *output, int r, int g, int b);
  void (*box) (void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height);
  void (*line) (void *output, int r, int g, int b, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2);
  void (*text) (void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text);
  void (*textsize) (void *output, const char *text, unsigned textlength, unsigned fontsize, unsigned *width);
};

extern void output_draw_start(struct lstopo_output *output);
extern void output_draw(struct lstopo_output *output);

int rgb_to_color(int r, int g, int b) __hwloc_attribute_const;
int declare_color(int r, int g, int b);


static __hwloc_inline int lstopo_pu_forbidden(hwloc_obj_t l)
{
  return !hwloc_bitmap_isset(l->allowed_cpuset, l->os_index);
}

static __hwloc_inline int lstopo_pu_running(struct lstopo_output *loutput, hwloc_obj_t l)
{
  hwloc_topology_t topology = loutput->topology;
  hwloc_bitmap_t bind = hwloc_bitmap_alloc();
  int res;
  if (loutput->pid_number != -1 && loutput->pid_number != 0)
    hwloc_get_proc_cpubind(topology, loutput->pid, bind, 0);
  else if (loutput->pid_number == 0)
    hwloc_get_cpubind(topology, bind, 0);
  res = bind && hwloc_bitmap_isset(bind, l->os_index);
  hwloc_bitmap_free(bind);
  return res;
}

static __hwloc_inline int lstopo_busid_snprintf(char *text, size_t textlen, hwloc_obj_t firstobj, unsigned collapse, unsigned needdomain)
{
  hwloc_obj_t lastobj;
  char domain[6] = "";
  unsigned i;

  if (needdomain)
    snprintf(domain, sizeof(domain), "%04x:", firstobj->attr->pcidev.domain);

  /* single busid */
  if (collapse <= 1) {
      return snprintf(text, textlen, "%s%02x:%02x.%01x",
		      domain,
		      firstobj->attr->pcidev.bus,
		      firstobj->attr->pcidev.dev,
		      firstobj->attr->pcidev.func);
  }

  for(lastobj=firstobj, i=1; i<collapse; i++)
    lastobj = lastobj->next_cousin;

  /* multiple busid functions for same busid device */
  if (firstobj->attr->pcidev.dev == lastobj->attr->pcidev.dev)
    return snprintf(text, textlen, "%s%02x:%02x.%01x-%01x",
		    domain,
		    firstobj->attr->pcidev.bus,
		    firstobj->attr->pcidev.dev,
		    firstobj->attr->pcidev.func,
		    lastobj->attr->pcidev.func);

  /* multiple busid devices */
  return snprintf(text, textlen, "%s%02x:%02x.%01x-%02x.%01x",
		  domain,
		  firstobj->attr->pcidev.bus,
		  firstobj->attr->pcidev.dev,
		  firstobj->attr->pcidev.func,
		  lastobj->attr->pcidev.dev,
		  lastobj->attr->pcidev.func);
}

#endif /* UTILS_LSTOPO_H */
