/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2010 INRIA.  All rights reserved.
 * Copyright © 2009-2010 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef UTILS_LSTOPO_H
#define UTILS_LSTOPO_H

#include <hwloc.h>

extern hwloc_obj_type_t show_only;
extern int show_cpuset;
extern int taskset;
extern hwloc_pid_t pid;

typedef void output_method (struct hwloc_topology *topology, const char *output, int logical, int legend, int verbose_mode);

FILE *open_file(const char *filename, const char *mode) __hwloc_attribute_malloc;

extern output_method output_console, output_text, output_x11, output_fig, output_png, output_pdf, output_ps, output_svg, output_windows, output_xml;

struct draw_methods {
  void* (*start) (void *output, int width, int height);
  void (*declare_color) (void *output, int r, int g, int b);
  void (*box) (void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height);
  void (*line) (void *output, int r, int g, int b, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2);
  void (*text) (void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text);
};

extern unsigned int gridsize, fontsize, force_horiz, force_vert;

extern void *output_draw_start(struct draw_methods *draw_methods, int logical, int legend, struct hwloc_topology *topology, void *output);
extern void output_draw(struct draw_methods *draw_methods, int logical, int legend, struct hwloc_topology *topology, void *output);

int rgb_to_color(int r, int g, int b) __hwloc_attribute_const;
int declare_color(int r, int g, int b);

#endif /* UTILS_LSTOPO_H */
