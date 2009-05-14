/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef UTILS_LSTOPO_H
#define UTILS_LSTOPO_H

#include <topology.h>

typedef void output_method (struct topo_topology *topology, FILE *output, int verbose_mode);

extern output_method output_text, output_x11, output_fig, output_png, output_pdf, output_ps, output_svg, output_windows;

struct draw_methods {
  void* (*start) (void *output, int width, int height);
  void (*declare_color) (void *output, int r, int g, int b);
  void (*box) (void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height);
  void (*text) (void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text);
};

extern void *output_draw_start(struct draw_methods *draw_methods, struct topo_topology *topology, void *output);
extern void output_draw(struct draw_methods *draw_methods, struct topo_topology *topology, void *output);

#endif /* UTILS_LSTOPO_H */
