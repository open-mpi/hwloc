/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

/* .fig back-end.  */

static struct color {
  int r, g, b;
} *colors;

static int numcolors;

static int
rgb_to_fig(int r, int g, int b)
{
  int i;

  if (r == 0xff && g == 0xff && b == 0xff)
    return 7;

  if (!r && !g && !b)
    return 0;

  for (i = 0; i < numcolors; i++)
    if (colors[i].r == r && colors[i].g == g && colors[i].b == b)
      return 32+i;

  fprintf(stderr, "color #%02x%02x%02x not declared\n", r, g, b);
  exit(EXIT_FAILURE);
}

static void *
fig_start(void *output_, int width, int height)
{
  FILE *output = output_;
  fprintf(output, "#FIG 3.2  Produced by libtopology's lstopo\n");
  fprintf(output, "Landscape\n");
  fprintf(output, "Center\n");
  fprintf(output, "Inches\n");
  fprintf(output, "letter\n");
  fprintf(output, "100.00\n");	/* magnification */
  fprintf(output, "Single\n");	/* single page */
  fprintf(output, "-2\n");	/* no transparent color */
  fprintf(output, "1200 2\n");	/* 1200 ppi resolution, upper left origin */
  return output;
}

static void
fig_declare_color(void *output_, int r, int g, int b)
{
  FILE *output = output_;

  if (r == 0xff && g == 0xff && b == 0xff)
    return;

  if (!r && !g && !b)
    return;

  colors = realloc(colors, sizeof(*colors) * (numcolors + 1));
  fprintf(output, "0 %d #%02x%02x%02x\n", 32 + numcolors, r, g, b);
  colors[numcolors].r = r;
  colors[numcolors].g = g;
  colors[numcolors].b = b;
  numcolors++;
}

static void
fig_box(void *output_, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height)
{
  FILE *output = output_;
  x *= 20;
  y *= 20;
  width *= 20;
  height *= 20;
  fprintf(output, "2 2 0 1 0 %d %u -1 20 0.0 0 0 -1 0 0 5\n\t", rgb_to_fig(r, g, b), depth);
  fprintf(output, " %u %u", x, y);
  fprintf(output, " %u %u", x + width, y);
  fprintf(output, " %u %u", x + width, y + height);
  fprintf(output, " %u %u", x, y + height);
  fprintf(output, " %u %u", x, y);
  fprintf(output, "\n");
}

static void
fig_text(void *output_, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text)
{
  FILE *output = output_;
  x *= 20;
  y *= 20;
  size = (size * 14) / 10;
  fprintf(output, "4 0 %d %u -1 0 %d 0.0 4 %d %u %u %u %s\\001\n", rgb_to_fig(r, g, b), depth, size, size * 10, (unsigned) strlen(text) * size * 10, x, y + size * 10, text);
}

static struct draw_methods fig_draw_methods = {
  .start = fig_start,
  .declare_color = fig_declare_color,
  .box = fig_box,
  .text = fig_text,
};

void
output_fig (topo_topology_t topology, FILE *output, int verbose_mode)
{
  output = output_draw_start(&fig_draw_methods, topology, output);
  output_draw(&fig_draw_methods, topology, output);
}
