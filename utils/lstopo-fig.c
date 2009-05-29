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

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

/* .fig back-end.  */

#define FIG_FACTOR 20

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
  x *= FIG_FACTOR;
  y *= FIG_FACTOR;
  width *= FIG_FACTOR;
  height *= FIG_FACTOR;
  fprintf(output, "2 2 0 1 0 %d %u -1 20 0.0 0 0 -1 0 0 5\n\t", rgb_to_fig(r, g, b), depth);
  fprintf(output, " %u %u", x, y);
  fprintf(output, " %u %u", x + width, y);
  fprintf(output, " %u %u", x + width, y + height);
  fprintf(output, " %u %u", x, y + height);
  fprintf(output, " %u %u", x, y);
  fprintf(output, "\n");
}

static void
fig_line(void *output_, int r, int g, int b, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
  FILE *output = output_;
  x1 *= FIG_FACTOR;
  y1 *= FIG_FACTOR;
  x2 *= FIG_FACTOR;
  y2 *= FIG_FACTOR;
  fprintf(output, "2 2 0 1 0 %d %u -1 20 0.0 0 0 -1 0 0 4\n\t", rgb_to_fig(r, g, b), depth);
  fprintf(output, " %u %u", x1, y1);
  fprintf(output, " %u %u", x2, y2);
  fprintf(output, "\n");
}

static void
fig_text(void *output_, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text)
{
  FILE *output = output_;
  x *= FIG_FACTOR;
  y *= FIG_FACTOR;
  size = (size * 14) / 10;
  fprintf(output, "4 0 %d %u -1 0 %d 0.0 4 %d %u %u %u %s\\001\n", rgb_to_fig(r, g, b), depth, size, size * 10, (unsigned) strlen(text) * size * 10, x, y + size * 10, text);
}

static struct draw_methods fig_draw_methods = {
  .start = fig_start,
  .declare_color = fig_declare_color,
  .box = fig_box,
  .line = fig_line,
  .text = fig_text,
};

void
output_fig (topo_topology_t topology, FILE *output, int verbose_mode)
{
  output = output_draw_start(&fig_draw_methods, topology, output);
  output_draw(&fig_draw_methods, topology, output);
}
