/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <cairo-xlib.h>
#if CAIRO_HAS_XLIB_SURFACE
#include <X11/Xlib.h>
#endif /* CAIRO_HAS_XLIB_SURFACE */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lstopo.h"

#if (CAIRO_HAS_XLIB_SURFACE + CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_SVG_SURFACE)
/* Cairo methods */
static void
topo_cairo_box(void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height)
{
  cairo_t *c = output;
  cairo_rectangle(c, x, y, width, height);
  cairo_set_source_rgb(c, (float)r / 255, (float) g / 255, (float) b / 255);
  cairo_fill(c);

  cairo_rectangle(c, x, y, width, height);
  cairo_set_source_rgb(c, 0, 0, 0);
  cairo_set_line_width(c, 1);
  cairo_stroke(c);
}

static void
topo_cairo_text(void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text)
{
  cairo_t *c = output;
  cairo_move_to(c, x, y + size);
  cairo_set_source_rgb(c, (float)r / 255, (float) g / 255, (float) b / 255);
  cairo_show_text(c, text);
}

#if (CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_SVG_SURFACE)
static cairo_status_t
topo_cairo_write(void *closure, const unsigned char *data, unsigned int length)
{
  if (fwrite(data, length, 1, closure) < 1)
    return CAIRO_STATUS_WRITE_ERROR;
  return CAIRO_STATUS_SUCCESS;
}
#endif /* (CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_SVG_SURFACE) */

static void
topo_cairo_paint(struct draw_methods *methods, lt_topo_t *topology, cairo_surface_t *cs)
{
  cairo_t *c;
  c = cairo_create(cs);
  output_draw(methods, topology, c);
  cairo_show_page(c);
  cairo_destroy(c);
}

static void null(void) {};
#endif /* (CAIRO_HAS_XLIB_SURFACE + CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_SVG_SURFACE) */


#if CAIRO_HAS_XLIB_SURFACE
/* X11 back-end */

struct display {
  Display *dpy;
  cairo_surface_t *cs;
};

static void *
x11_start(void *output, int width, int height)
{
  cairo_surface_t *cs;
  Display *dpy;
  Window root, win;
  int screen;
  struct display *disp;

  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr, "couldn't connect to X\n");
    exit(EXIT_FAILURE);
  }
  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);
  win = XCreateSimpleWindow(dpy, root, 0, 0, width, height, 0, WhitePixel(dpy, screen), WhitePixel(dpy, screen));

  XSelectInput(dpy, win, ExposureMask);
  XMapWindow(dpy,win);

  cs = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy, screen), width, height);

  disp = malloc(sizeof(*disp));
  disp->dpy = dpy;
  disp->cs = cs;

  return disp;
}

static struct draw_methods x11_draw_methods = {
  .start = x11_start,
  .declare_color = (void*) null,
  .box = topo_cairo_box,
  .text = topo_cairo_text,
};

void
output_x11(lt_topo_t *topology, FILE *output, int verbose_mode)
{
  struct display *disp = output_draw_start(&x11_draw_methods, topology, output);

  while (1) {
    XEvent e;
    XNextEvent(disp->dpy, &e);
    if (e.type == Expose && e.xexpose.count < 1)
      topo_cairo_paint(&x11_draw_methods, topology, disp->cs);
  }
  cairo_surface_destroy(disp->cs);
  XCloseDisplay(disp->dpy);
  free(disp);
}
#endif /* CAIRO_HAS_XLIB_SURFACE */


#if CAIRO_HAS_PNG_FUNCTIONS
/* PNG back-end */
static void *
png_start(void *output, int width, int height)
{
  return cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
}

static struct draw_methods png_draw_methods = {
  .start = png_start,
  .declare_color = (void*) null,
  .box = topo_cairo_box,
  .text = topo_cairo_text,
};

void
output_png(lt_topo_t *topology, FILE *output, int verbose_mode)
{
  cairo_surface_t *cs = output_draw_start(&png_draw_methods, topology, output);

  topo_cairo_paint(&png_draw_methods, topology, cs);
  cairo_surface_write_to_png_stream(cs, topo_cairo_write, output);
  cairo_surface_destroy(cs);
}
#endif /* CAIRO_HAS_PNG_FUNCTIONS */


#if CAIRO_HAS_PDF_SURFACE
/* PDF back-end */
static void *
pdf_start(void *output, int width, int height)
{
  return cairo_pdf_surface_create_for_stream(topo_cairo_write, output, width, height);
}

static struct draw_methods pdf_draw_methods = {
  .start = pdf_start,
  .declare_color = (void*) null,
  .box = topo_cairo_box,
  .text = topo_cairo_text,
};

void
output_pdf(lt_topo_t *topology, FILE *output, int verbose_mode)
{
  cairo_surface_t *cs = output_draw_start(&pdf_draw_methods, topology, output);

  topo_cairo_paint(&pdf_draw_methods, topology, cs);
  cairo_surface_flush(cs);
  cairo_surface_destroy(cs);
}
#endif /* CAIRO_HAS_PDF_SURFACE */


#if CAIRO_HAS_SVG_SURFACE
/* SVG back-end */
static void *
svg_start(void *output, int width, int height)
{
  return cairo_svg_surface_create_for_stream(topo_cairo_write, output, width, height);
}

static struct draw_methods svg_draw_methods = {
  .start = svg_start,
  .declare_color = (void*) null,
  .box = topo_cairo_box,
  .text = topo_cairo_text,
};

void
output_svg(lt_topo_t *topology, FILE *output, int verbose_mode)
{
  cairo_surface_t *cs = output_draw_start(&svg_draw_methods, topology, output);

  topo_cairo_paint(&svg_draw_methods, topology, cs);
  cairo_surface_flush(cs);
  cairo_surface_destroy(cs);
}
#endif /* CAIRO_HAS_SVG_SURFACE */

#endif /* CAIRO */
