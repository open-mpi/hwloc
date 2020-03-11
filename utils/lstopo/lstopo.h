/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2019 Inria.  All rights reserved.
 * Copyright © 2009-2010, 2012, 2015 Université Bordeaux
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef UTILS_LSTOPO_H
#define UTILS_LSTOPO_H

#include "private/autogen/config.h"
#include "hwloc.h"
#include "misc.h"

enum lstopo_drawing_e {
  LSTOPO_DRAWING_PREPARE,
  LSTOPO_DRAWING_DRAW
};

enum lstopo_orient_e {
  LSTOPO_ORIENT_NONE = 0,
  LSTOPO_ORIENT_HORIZ,
  LSTOPO_ORIENT_VERT,
  LSTOPO_ORIENT_RECT
};

enum lstopo_index_type_e {
  LSTOPO_INDEX_TYPE_DEFAULT,
  LSTOPO_INDEX_TYPE_PHYSICAL,
  LSTOPO_INDEX_TYPE_LOGICAL,
  LSTOPO_INDEX_TYPE_NONE /* only used during the interactive display */
};

FILE *open_output(const char *filename, int overwrite) __hwloc_attribute_malloc;

struct draw_methods;

/* if embedded in backend-specific output structure, must be at the beginning */
struct lstopo_output {
  hwloc_topology_t topology;
  unsigned depth;

  /* file config */
  FILE *file;
  int overwrite;

  /* misc config */
  enum lstopo_index_type_e index_type;
  int verbose_mode;
  int ignore_pus;
  int ignore_numanodes;
  int pci_collapse_enabled; /* global toggle for PCI collapsing */
  int pid_number;
  hwloc_pid_t pid;
  int need_pci_domain;
  hwloc_bitmap_t cpubind_set, membind_set;

  /* export config */
  unsigned long export_synthetic_flags;
  unsigned long export_xml_flags;
  uint64_t shmem_output_addr;

  /* legend */
  int legend;
  char ** legend_append;
  unsigned legend_append_nr;

  /* text config */
  int show_distances_only;
  hwloc_obj_type_t show_only;
  int show_cpuset;
  int show_taskset;

  /* draw config */
  char title[256];
  unsigned plain_children_order;
  unsigned int gridsize, fontsize, linespacing;
  float text_xscale;
  enum lstopo_orient_e force_orient[HWLOC_OBJ_TYPE_MAX]; /* orientation of children within an object of the given type */
  unsigned no_half_lines; /* set by ASCII backend because it cannot write between lines of the terminal */
  int show_indexes[HWLOC_OBJ_TYPE_MAX]; /* enabled by global toggle index_type */
  int show_text_enabled; /* global toggle for interactive keyboard shortcuts */
  int show_text[HWLOC_OBJ_TYPE_MAX];
  int show_attrs_enabled; /* global toggle for interactive keyboard shortcuts */
  int show_attrs[HWLOC_OBJ_TYPE_MAX];
  int show_binding;
  int show_disallowed;
  int factorize_enabled; /* global toggle for interactive keyboard shortcuts */
  unsigned factorize_min[HWLOC_OBJ_TYPE_MAX]; /* minimum number of object before factorizing (parent->arity must be strictly higher) */
#define FACTORIZE_MIN_DEFAULT 4
#define FACTORIZE_MIN_DISABLED UINT_MAX
  unsigned factorize_first[HWLOC_OBJ_TYPE_MAX]; /* number of first children to keep before factorizing */
  unsigned factorize_last[HWLOC_OBJ_TYPE_MAX]; /* number of last children to keep after factorizing */

  /* draw internal data */
  void *backend_data;
  struct draw_methods *methods;
  enum lstopo_drawing_e drawing;
  unsigned width, height; /* total output size */
};

struct lstopo_color {
  /* these variables must be initialized before passing the structure to declare_color() */
  int r, g, b;
  int free; /* 1 if lstopo should free() this structure at exit */

  /* these variable are initialized by declare_color() */
  /* backend specific private data */
  union lstopo_color_private_u {
    struct lstopo_color_private_ascii_s {
      int color;
    } ascii;
    struct lstopo_color_private_fig_s {
      int color;
    } fig;
#ifdef HWLOC_WIN_SYS
    struct lstopo_color_private_windows_s {
      HGDIOBJ brush;
      COLORREF color;
    } windows;
#endif
  } private;
  /* list of colors */
  struct lstopo_color *next;
};

struct lstopo_style {
  struct lstopo_color
	*bg,	/* main box background color */
	*t,	/* main text color */
	*t2;	/* other text color */
};

#define LSTOPO_CHILD_KIND_NORMAL 0x1
#define LSTOPO_CHILD_KIND_MEMORY 0x2
#define LSTOPO_CHILD_KIND_IO     0x4
#define LSTOPO_CHILD_KIND_MISC   0x8
#define LSTOPO_CHILD_KIND_ALL    0xf

struct lstopo_obj_userdata {
  /* original common userdata (we replace the first one with this extended structure) */
  struct hwloc_utils_userdata common;

  /* PCI collapsing */
  int pci_collapsed; /* 0 if no collapsing, -1 if collapsed with a previous one, >1 if collapsed with several next */
  int factorized; /* 0 if no factorizing, -1 if hidden, 1 if replaced with dots */

  /* custom style */
  struct lstopo_style style;
#define LSTOPO_STYLE_BG  0x1
#define LSTOPO_STYLE_T   0x2
#define LSTOPO_STYLE_T2  0x4
  unsigned style_set; /* OR'ed LSTOPO_STYLE_* */

  /* object size (including children if they are outside of it, not including borders) */
  unsigned width;
  unsigned height;

  /* a child position is: its parent position + parent->children_*rel + child->*rel */
  /* relative position of first child with respect to top-left corner of this object */
  struct lstopo_children_position {
    unsigned kinds;
    unsigned width;
    unsigned height;
    unsigned xrel;
    unsigned yrel;
    unsigned box;
    struct lstopo_color *boxcolor;
  } children;
  /* relative position of first memory child */
  struct lstopo_children_position above_children;

  /* relative position of this object within its parent children zone */
  unsigned xrel;
  unsigned yrel;

  /* children orientation */
  enum lstopo_orient_e orient;

  /* text lines within object */
#define LSTOPO_OBJ_TEXT_MAX 4 /* current max number of lines is osdev name + 3 cuda attributes */
  struct lstopo_text_line {
    char text[128];
    unsigned width;
    unsigned xoffset;
  } text[LSTOPO_OBJ_TEXT_MAX];
  unsigned ntext;
  unsigned textwidth; /* required width for all lines of text,
		       * max of above text[].width + optional padding
		       */
};

typedef int output_method (struct lstopo_output *output, const char *filename);
extern output_method output_console, output_synthetic, output_ascii, output_fig, output_png, output_pdf, output_ps, output_nativesvg, output_cairosvg, output_x11, output_windows, output_xml, output_shmem;

extern int lstopo_shmem_adopt(const char *input, hwloc_topology_t *topologyp);

struct draw_methods {
  int (*declare_color) (struct lstopo_output *loutput, struct lstopo_color *lcolor);
  /* only called when loutput->draw_methods == LSTOPO_DRAWING_DRAW */
  void (*box) (struct lstopo_output *loutput, const struct lstopo_color *lcolor, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height, hwloc_obj_t obj, unsigned box_id);
  void (*line) (struct lstopo_output *loutput, const struct lstopo_color *lcolor, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2, hwloc_obj_t obj, unsigned line_id);
  void (*text) (struct lstopo_output *loutput, const struct lstopo_color *lcolor, int size, unsigned depth, unsigned x, unsigned y, const char *text, hwloc_obj_t obj, unsigned text_id);
  /* may be called when loutput->drawing == LSTOPO_DRAWING_PREPARE */
  void (*textsize) (struct lstopo_output *loutput, const char *text, unsigned textlength, unsigned fontsize, unsigned *width);
};

extern void output_draw(struct lstopo_output *output);

extern void lstopo_prepare_custom_styles(struct lstopo_output *loutput);
extern void declare_colors(struct lstopo_output *output);
extern void destroy_colors(void);

static __hwloc_inline int lstopo_pu_disallowed(struct lstopo_output *loutput, hwloc_obj_t l)
{
  hwloc_topology_t topology = loutput->topology;
  return !hwloc_bitmap_isset(hwloc_topology_get_allowed_cpuset(topology), l->os_index);
}

static __hwloc_inline int lstopo_pu_binding(struct lstopo_output *loutput, hwloc_obj_t l)
{
  return loutput->pid_number != -1 && hwloc_bitmap_isset(loutput->cpubind_set, l->os_index);
}

static __hwloc_inline int lstopo_numa_disallowed(struct lstopo_output *loutput, hwloc_obj_t l)
{
  hwloc_topology_t topology = loutput->topology;
  return !hwloc_bitmap_isset(hwloc_topology_get_allowed_nodeset(topology), l->os_index);
}

static __hwloc_inline int lstopo_numa_binding(struct lstopo_output *loutput, hwloc_obj_t l)
{
  return loutput->pid_number != -1 && hwloc_bitmap_isset(loutput->membind_set, l->os_index);
}

static __hwloc_inline int lstopo_busid_snprintf(struct lstopo_output *loutput, char *text, size_t textlen, hwloc_obj_t firstobj, int collapse, unsigned needdomain)
{
  hwloc_obj_t lastobj;
  char domain[10] = "";
  unsigned i;

  if (needdomain)
    snprintf(domain, sizeof(domain), "%04x:", firstobj->attr->pcidev.domain);

  /* single busid */
  if (!loutput->pci_collapse_enabled || collapse <= 1) {
      return snprintf(text, textlen, "%s%02x:%02x.%01x",
		      domain,
		      firstobj->attr->pcidev.bus,
		      firstobj->attr->pcidev.dev,
		      firstobj->attr->pcidev.func);
  }

  assert(collapse >= 0); /* should be called on the first object of a collapsed range */

  for(lastobj=firstobj, i=1; i<(unsigned)collapse; i++)
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

extern void lstopo_show_interactive_cli_options(const struct lstopo_output *loutput);
extern void lstopo_show_interactive_help(void);

#endif /* UTILS_LSTOPO_H */
