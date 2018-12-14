/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2018 Inria.  All rights reserved.
 * Copyright © 2009-2010, 2014, 2017 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>

#include <cairo.h>

#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif /* CAIRO_HAS_PDF_SURFACE */

#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif /* CAIRO_HAS_PS_SURFACE */

#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif /* CAIRO_HAS_SVG_SURFACE */

#ifdef LSTOPO_HAVE_X11
/* configure should enable X11 only if Cairo has XLIB SURFACE and there are X11 headers */
# ifndef HWLOC_HAVE_X11_KEYSYM
#  error LSTOPO_HAVE_X11 enabled without HWLOC_HAVE_X11_KEYSYM?!
# endif
# ifndef CAIRO_HAS_XLIB_SURFACE
#  error LSTOPO_HAVE_X11 enabled without CAIRO_HAS_XLIB_SURFACE?!
# endif
# include <cairo-xlib.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# include <X11/cursorfont.h>
/* Avoid Xwindow's definition conflict with Windows' use for fields names.  */
# undef Status
#else /* LSTOPO_HAVE_X11 */
# if (defined HWLOC_HAVE_X11_KEYSYM) && (defined CAIRO_HAS_XLIB_SURFACE)
#  error HWLOC_HAVE_X11_KEYSYM and CAIRO_HAS_XLIB_SURFACE defined but LSTOPO_HAVE_X11 not enabled?!
# endif
#endif /* LSTOPO_HAVE_X11 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "lstopo.h"

#if (defined LSTOPO_HAVE_X11) + (defined CAIRO_HAS_PNG_FUNCTIONS) + (defined CAIRO_HAS_PDF_SURFACE) + (defined CAIRO_HAS_PS_SURFACE) + (defined CAIRO_HAS_SVG_SURFACE)
struct lstopo_cairo_output {
  struct lstopo_output *loutput;
  cairo_surface_t *surface;
  cairo_t *context;
};

/* Cairo methods */
static void
topo_cairo_box(struct lstopo_output *loutput, const struct lstopo_color *lcolor, unsigned depth __hwloc_attribute_unused, unsigned x, unsigned width, unsigned y, unsigned height)
{
  struct lstopo_cairo_output *coutput = loutput->backend_data;
  cairo_t *c = coutput->context;
  int r = lcolor->r, g = lcolor->g, b = lcolor->b;

  cairo_rectangle(c, x, y, width, height);
  cairo_set_source_rgb(c, (float)r / 255, (float) g / 255, (float) b / 255);
  cairo_fill(c);

  cairo_rectangle(c, x, y, width, height);
  cairo_set_source_rgb(c, 0, 0, 0);
  cairo_set_line_width(c, 1);
  cairo_stroke(c);
}

static void
topo_cairo_line(struct lstopo_output *loutput, const struct lstopo_color *lcolor, unsigned depth __hwloc_attribute_unused, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
  struct lstopo_cairo_output *coutput = loutput->backend_data;
  cairo_t *c = coutput->context;
  int r = lcolor->r, g = lcolor->g, b = lcolor->b;

  cairo_move_to(c, x1, y1);
  cairo_set_source_rgb(c, (float) r / 255, (float) g / 255, (float) b / 255);
  cairo_set_line_width(c, 1);
  cairo_line_to(c, x2, y2);
  cairo_stroke(c);
}

static void
topo_cairo_text(struct lstopo_output *loutput, const struct lstopo_color *lcolor, int fontsize, unsigned depth __hwloc_attribute_unused, unsigned x, unsigned y, const char *text)
{
  struct lstopo_cairo_output *coutput = loutput->backend_data;
  cairo_t *c = coutput->context;
  int r = lcolor->r, g = lcolor->g, b = lcolor->b;

  cairo_move_to(c, x, y + fontsize);
  cairo_set_source_rgb(c, (float)r / 255, (float) g / 255, (float) b / 255);
  cairo_show_text(c, text);
}

static void
topo_cairo_textsize(struct lstopo_output *loutput, const char *text, unsigned textlength __hwloc_attribute_unused, unsigned fontsize __hwloc_attribute_unused, unsigned *width)
{
  struct lstopo_cairo_output *coutput = loutput->backend_data;
  cairo_t *c = coutput->context;
  cairo_text_extents_t extents;
  cairo_text_extents(c, text, &extents);
  *width = extents.width;
}

#if (defined CAIRO_HAS_PNG_FUNCTIONS) + (defined CAIRO_HAS_PDF_SURFACE) + (defined CAIRO_HAS_PS_SURFACE) + (defined CAIRO_HAS_SVG_SURFACE)
static cairo_status_t
topo_cairo_write(void *closure, const unsigned char *data, unsigned int length)
{
  if (fwrite(data, length, 1, closure) < 1)
    return CAIRO_STATUS_WRITE_ERROR;
  return CAIRO_STATUS_SUCCESS;
}
#endif /* (CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_PS_SURFACE + CAIRO_HAS_SVG_SURFACE) */

static void
topo_cairo_paint(struct lstopo_cairo_output *coutput)
{
  unsigned fontsize = coutput->loutput->fontsize;
  cairo_surface_t *cs = coutput->surface;
  cairo_t *c = cairo_create(cs);
  coutput->context = c;
  cairo_set_font_size(c, fontsize);
  output_draw(coutput->loutput);
  cairo_show_page(c);
  cairo_destroy(c);
  coutput->context = NULL;
}



#endif /* (LSTOPO_HAVE_X11 + CAIRO_HAS_PNG_FUNCTIONS + CAIRO_HAS_PDF_SURFACE + CAIRO_HAS_PS_SURFACE + CAIRO_HAS_SVG_SURFACE) */


#ifdef LSTOPO_HAVE_X11
/* X11 back-end */

struct lstopo_x11_output {
  struct lstopo_cairo_output coutput; /* must be at the beginning */
  Display *dpy;
  int scr;
  Window top, win;
  Cursor hand;
  unsigned int orig_fontsize, orig_gridsize;
  int screen_width, screen_height;		/** visible part size */
  int last_screen_width, last_screen_height;	/** last visible part size */
  int width, height;				/** total normal display size */
  int x, y;					/** top left corner of the visible part */
  float scale, last_scale;
  int needs_redraw;
};

static void
x11_create(struct lstopo_x11_output *disp, int width, int height)
{
  disp->win = XCreateSimpleWindow(disp->dpy, disp->top, 0, 0, width, height, 0, WhitePixel(disp->dpy, disp->scr), WhitePixel(disp->dpy, disp->scr));
  disp->hand = XCreateFontCursor(disp->dpy, XC_fleur);
  XDefineCursor(disp->dpy, disp->win, disp->hand);
  XSelectInput(disp->dpy, disp->win,
      KeyPressMask |
      ButtonPressMask | ButtonReleaseMask |
      PointerMotionMask |
      ExposureMask);
  XMapWindow(disp->dpy, disp->win);
  disp->coutput.surface = cairo_xlib_surface_create(disp->dpy, disp->win, DefaultVisual(disp->dpy, disp->scr), width, height);
}

static void
x11_destroy(struct lstopo_x11_output *disp)
{
  cairo_surface_destroy(disp->coutput.surface);
  XDestroyWindow(disp->dpy, disp->win);
}

/** Clip coordinates of the visible part. */
static void
move_x11(struct lstopo_x11_output *disp)
{
  struct lstopo_cairo_output *coutput = &disp->coutput;
  struct lstopo_output *loutput = coutput->loutput;

  if (disp->scale != disp->last_scale) {
    disp->x = disp->scale / disp->last_scale * (float)disp->x;
    disp->y = disp->scale / disp->last_scale * (float)disp->y;
  }

  if (disp->needs_redraw
      || disp->screen_width != disp->last_screen_width
      || disp->screen_height != disp->last_screen_height
      || disp->scale != disp->last_scale) {
    disp->last_screen_width = disp->screen_width;
    disp->last_screen_height = disp->screen_height;
    disp->last_scale = disp->scale;
    loutput->fontsize = disp->orig_fontsize * disp->scale;
    loutput->gridsize = disp->orig_gridsize * disp->scale;

    x11_destroy(disp);

    x11_create(disp, disp->width, disp->height);
    /* recompute new sizes and positions */
    loutput->drawing = LSTOPO_DRAWING_PREPARE;
    topo_cairo_paint(coutput);
    if (loutput->width > (unsigned) disp->width || loutput->height > (unsigned) disp->height) {
      /* need to extend the window before actual redraw */
      x11_destroy(disp);
      x11_create(disp, loutput->width, loutput->height);
    }
    loutput->drawing = LSTOPO_DRAWING_DRAW;
    topo_cairo_paint(coutput);
    disp->width = loutput->width;
    disp->height = loutput->height;
    disp->needs_redraw = 0;
  }

  if (disp->width <= disp->screen_width) {
    disp->x = 0;
  } else {
    if (disp->x < 0)
      disp->x = 0;
    if (disp->x >= disp->width - disp->screen_width)
      disp->x = disp->width - disp->screen_width;
  }

  if (disp->height <= disp->screen_height) {
    disp->y = 0;
  } else {
    if (disp->y < 0)
      disp->y = 0;
    if (disp->y >= disp->height - disp->screen_height)
      disp->y = disp->height - disp->screen_height;
  }
}

static struct draw_methods x11_draw_methods = {
  NULL,
  topo_cairo_box,
  topo_cairo_line,
  topo_cairo_text,
  topo_cairo_textsize,
};

int
output_x11(struct lstopo_output *loutput, const char *dummy __hwloc_attribute_unused)
{
  struct lstopo_x11_output _disp, *disp = &_disp;
  struct lstopo_cairo_output *coutput;
  Display *dpy;
  Window root, top;
  int scr;
  Screen *screen;
  int screen_width, screen_height;
  unsigned int dpi_x, dpi_y, dpi;
  int finish = 0;
  int state = 0;
  int x = 0, y = 0; /* shut warning down */
  int lastx, lasty;
  unsigned i;

  coutput = &disp->coutput;
  memset(coutput, 0, sizeof(*coutput));
  coutput->loutput = loutput;
  loutput->backend_data = coutput;
  loutput->methods = &x11_draw_methods;

  /* create the toplevel window */
  if (!(dpy = XOpenDisplay(NULL))) {
    fprintf(stderr, "couldn't connect to X\n");
    return -1;
  }

  disp->dpy = dpy;
  disp->scr = scr = DefaultScreen(dpy);
  screen = ScreenOfDisplay(dpy, scr);

  /* 25.4mm per inch */
  dpi_x = ((double) DisplayWidth(dpy, scr) * 25.4) / DisplayWidthMM(dpy, scr);
  dpi_y = ((double) DisplayHeight(dpy, scr) * 25.4) / DisplayHeightMM(dpy, scr);
  dpi = (dpi_x + dpi_y) / 2;

  /* Original values for fontsize/gridsize were tuned for 96dpi */
  coutput->loutput->fontsize = (coutput->loutput->fontsize * dpi) / 96;
  coutput->loutput->gridsize = (coutput->loutput->gridsize * dpi) / 96;

  /* recurse once for preparing sizes and positions using the root window surface */
  root = RootWindow(dpy, scr);
  disp->top = root;
  loutput->drawing = LSTOPO_DRAWING_PREPARE;
  x11_create(disp, 1, 1);
  topo_cairo_paint(coutput);
  x11_destroy(disp);
  loutput->drawing = LSTOPO_DRAWING_DRAW;

  /* now create the actual window with the computed max size */
  screen_width = loutput->width;
  screen_height = loutput->height;

  disp->top = top = XCreateSimpleWindow(dpy, root, 0, 0, screen_width, screen_height, 0, WhitePixel(dpy, scr), WhitePixel(dpy, scr));
  XStoreName(dpy, top, "lstopo");
  XSetIconName(dpy, top, "lstopo");
  XSelectInput(dpy,top, StructureNotifyMask | KeyPressMask);

  if (screen_width >= screen->width)
    screen_width = screen->width;
  if (screen_height >= screen->height)
    screen_height = screen->height;
  disp->last_screen_width = 0;
  disp->last_screen_height = 0;
  disp->screen_width = screen_width;
  disp->screen_height = screen_height;
  disp->width = loutput->width;
  disp->height = loutput->height;
  disp->orig_fontsize = loutput->fontsize;
  disp->orig_gridsize = loutput->gridsize;
  disp->x = 0;
  disp->y = 0;
  disp->scale = disp->last_scale = 1.0f;
  /* TODO: if window got truncated, scale down? */
  disp->needs_redraw = 0;

  x11_create(disp, loutput->width, loutput->height);

  XMapWindow(dpy, top);

  printf("\n");
  printf("Keyboard shortcuts:\n");
  printf(" Zoom-in or out ...................... + -\n");
  printf(" Try to fit scale to window .......... f F\n");
  printf(" Reset scale to default .............. 1\n");
  printf(" Scroll vertically ................... Up Down PageUp PageDown\n");
  printf(" Scroll horizontally ................. Left Right Ctrl+PageUp/Down\n");
  printf(" Scroll to the top-left corner ....... Home\n");
  printf(" Scroll to the bottom-right corner ... End\n");
  printf(" Exit ................................ q Q Esc\n");
  printf("Configuration tweaks:\n");
  printf(" Toggle color for disallowed objects . d\n");
  printf(" Toggle color for binding objects .... b\n");
  printf(" Show/Hide Attributes/Indexes/Text ... A/I/T\n");
  printf(" Show Physical/Logical/Both indexes .. P/L/B\n");
  printf(" Command-line options for tweaks ..... c\n");
  printf("\n\n");

  /* ready */
  declare_colors(loutput);
  lstopo_prepare_custom_styles(loutput);

  lastx = disp->x;
  lasty = disp->y;

  topo_cairo_paint(coutput);

  while (!finish) {
    XEvent e;
    if (!XEventsQueued(disp->dpy, QueuedAfterFlush)) {
      /* No pending event, flush moving windows before waiting for next event */
      if (disp->x != lastx || disp->y != lasty) {
	XMoveWindow(disp->dpy, disp->win, -disp->x, -disp->y);
	lastx = disp->x;
	lasty = disp->y;
      }
    }
    XNextEvent(disp->dpy, &e);
    switch (e.type) {
      case Expose:
	if (e.xexpose.count < 1)
	  topo_cairo_paint(coutput);
	break;
      case MotionNotify:
	if (state) {
	  disp->x -= e.xmotion.x_root - x;
	  disp->y -= e.xmotion.y_root - y;
	  x = e.xmotion.x_root;
	  y = e.xmotion.y_root;
	  move_x11(disp);
	}
	break;
      case ConfigureNotify: {
	float wscale, hscale;
	disp->screen_width = e.xconfigure.width;
	disp->screen_height = e.xconfigure.height;
	if (disp->screen_width != disp->last_screen_width
	    || disp->screen_height != disp->last_screen_height) {
	  wscale = disp->screen_width / (float)disp->width;
	  hscale = disp->screen_height / (float)disp->height;
	  disp->scale *= wscale > hscale ? hscale : wscale;
	  if (disp->scale < 1.0f)
	    disp->scale = 1.0f;
	  move_x11(disp);
	}
	if (disp->x != lastx || disp->y != lasty)
	  XMoveWindow(disp->dpy, disp->win, -disp->x, -disp->y);
	break;
      }
      case ButtonPress:
	  if (e.xbutton.button == Button1) {
	  state = 1;
	  x = e.xbutton.x_root;
	  y = e.xbutton.y_root;
	}
	break;
      case ButtonRelease:
	if (e.xbutton.button == Button1)
	  state = 0;
	break;
      case MappingNotify:
	XRefreshKeyboardMapping(&e.xmapping);
	break;
      case KeyPress: {
	KeySym keysym;
	XLookupString(&e.xkey, NULL, 0, &keysym, NULL);
        switch (keysym) {
          case XK_q:
          case XK_Q:
          case XK_Escape:
            finish = 1;
            break;
          case XK_Left:
            disp->x -= disp->screen_width/10;
            move_x11(disp);
            break;
          case XK_Right:
            disp->x += disp->screen_width/10;
            move_x11(disp);
            break;
          case XK_Up:
            disp->y -= disp->screen_height/10;
            move_x11(disp);
            break;
          case XK_Down:
            disp->y += disp->screen_height/10;
            move_x11(disp);
            break;
          case XK_Page_Up:
            if (e.xkey.state & ControlMask) {
              disp->x -= disp->screen_width;
              move_x11(disp);
            } else {
              disp->y -= disp->screen_height;
              move_x11(disp);
            }
            break;
          case XK_Page_Down:
            if (e.xkey.state & ControlMask) {
              disp->x += disp->screen_width;
              move_x11(disp);
            } else {
              disp->y += disp->screen_height;
              move_x11(disp);
            }
            break;
          case XK_Home:
            disp->x = 0;
            disp->y = 0;
            move_x11(disp);
            break;
          case XK_End:
            disp->x = INT_MAX;
            disp->y = INT_MAX;
            move_x11(disp);
            break;
	case XK_f:
	case XK_F: {
	  float wscale = disp->screen_width / (float)disp->width;
	  float hscale = disp->screen_height / (float)disp->height;
	  disp->scale *= wscale > hscale ? hscale : wscale;
	  move_x11(disp);
	  break;
	}
	case XK_plus:
	case XK_KP_Add:
	  disp->scale *= 1.2f;
	  move_x11(disp);
	  break;
	case XK_minus:
	case XK_KP_Subtract:
	  disp->scale /= 1.2f;
	  move_x11(disp);
	  break;
	case XK_1:
	case XK_KP_1:
	  disp->scale = 1.0f;
	  move_x11(disp);
	  break;
	case XK_A: {
	  int v = !loutput->show_attrs[0]; /* if show_attrs[] contains different values, assume it's all like the first type */
	  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	    loutput->show_attrs[i] = v;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	}
	case XK_I: {
	  int v = !loutput->show_indexes[0]; /* if show_indexes[] contains different values, assume it's all like the first type */
	  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	    loutput->show_indexes[i] = v;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	}
	case XK_T: {
	  int v = !loutput->show_text[0]; /* if show_text[] contains different values, assume it's all like the first type */
	  for(i=HWLOC_OBJ_TYPE_MIN; i<HWLOC_OBJ_TYPE_MAX; i++)
	    loutput->show_text[i] = v;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	}
	case XK_L:
	  loutput->index_type = LSTOPO_INDEX_TYPE_LOGICAL;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	case XK_P:
	  loutput->index_type = LSTOPO_INDEX_TYPE_PHYSICAL;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	case XK_B:
	  loutput->index_type = LSTOPO_INDEX_TYPE_DEFAULT;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	case XK_b:
	  loutput->show_binding ^= 1;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	case XK_d:
	  loutput->show_disallowed ^= 1;
	  disp->needs_redraw = 1;
	  move_x11(disp);
	  break;
	case XK_c:
	  lstopo_show_interactive_cli_options(loutput);
	  break;
	}
      }
    }
  }
  x11_destroy(disp);
  XDestroyWindow(disp->dpy, disp->top);
  XFreeCursor(disp->dpy, disp->hand);
  XCloseDisplay(disp->dpy);

  destroy_colors();
  return 0;
}
#endif /* LSTOPO_HAVE_X11 */


#ifdef CAIRO_HAS_PNG_FUNCTIONS
/* PNG back-end */

static struct draw_methods png_draw_methods = {
  NULL,
  topo_cairo_box,
  topo_cairo_line,
  topo_cairo_text,
  topo_cairo_textsize,
};

int
output_png(struct lstopo_output *loutput, const char *filename)
{
  struct lstopo_cairo_output coutput;
  FILE *output;
  cairo_surface_t *fakecs, *cs;

  output = open_output(filename, loutput->overwrite);
  if (!output) {
    fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
    return -1;
  }

  memset(&coutput, 0, sizeof(coutput));
  coutput.loutput = loutput;
  loutput->backend_data = &coutput;
  loutput->methods = &png_draw_methods;
  loutput->file = output;

  /* create a fake surface */
  fakecs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  coutput.surface = fakecs;

  /* recurse once for preparing sizes and positions using the fake surface */
  loutput->drawing = LSTOPO_DRAWING_PREPARE;
  topo_cairo_paint(&coutput);
  loutput->drawing = LSTOPO_DRAWING_DRAW;
  cairo_surface_destroy(fakecs);

  /* create the actual surface with the right size */
  cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, loutput->width, loutput->height);
  coutput.surface = cs;

  /* ready */
  declare_colors(loutput);
  lstopo_prepare_custom_styles(loutput);

  topo_cairo_paint(&coutput);
  cairo_surface_write_to_png_stream(coutput.surface, topo_cairo_write, output);
  cairo_surface_destroy(coutput.surface);

  if (output != stdout)
    fclose(output);

  destroy_colors();
  return 0;
}
#endif /* CAIRO_HAS_PNG_FUNCTIONS */


#ifdef CAIRO_HAS_PDF_SURFACE
/* PDF back-end */

static struct draw_methods pdf_draw_methods = {
  NULL,
  topo_cairo_box,
  topo_cairo_line,
  topo_cairo_text,
  topo_cairo_textsize,
};

int
output_pdf(struct lstopo_output *loutput, const char *filename)
{
  struct lstopo_cairo_output coutput;
  FILE *output;
  cairo_surface_t *fakecs, *cs;

  output = open_output(filename, loutput->overwrite);
  if (!output) {
    fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
    return -1;
  }

  memset(&coutput, 0, sizeof(coutput));
  coutput.loutput = loutput;
  loutput->backend_data = &coutput;
  loutput->methods = &pdf_draw_methods;
  loutput->file = output;

  /* create a fake surface */
  fakecs = cairo_pdf_surface_create_for_stream(NULL, NULL, 1, 1);
  coutput.surface = fakecs;

  /* recurse once for preparing sizes and positions using the fake surface */
  loutput->drawing = LSTOPO_DRAWING_PREPARE;
  topo_cairo_paint(&coutput);
  loutput->drawing = LSTOPO_DRAWING_DRAW;
  cairo_surface_destroy(fakecs);

  /* create the actual surface with the right size */
  cs = cairo_pdf_surface_create_for_stream(topo_cairo_write, loutput->file, loutput->width, loutput->height);
  coutput.surface = cs;

  /* ready */
  declare_colors(loutput);
  lstopo_prepare_custom_styles(loutput);

  topo_cairo_paint(&coutput);
  cairo_surface_flush(coutput.surface);
  cairo_surface_destroy(coutput.surface);

  if (output != stdout)
    fclose(output);

  destroy_colors();
  return 0;
}
#endif /* CAIRO_HAS_PDF_SURFACE */


#ifdef CAIRO_HAS_PS_SURFACE
/* PS back-end */

static struct draw_methods ps_draw_methods = {
  NULL,
  topo_cairo_box,
  topo_cairo_line,
  topo_cairo_text,
  topo_cairo_textsize,
};

int
output_ps(struct lstopo_output *loutput, const char *filename)
{
  struct lstopo_cairo_output coutput;
  FILE *output;
  cairo_surface_t *fakecs, *cs;

  output = open_output(filename, loutput->overwrite);
  if (!output) {
    fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
    return -1;
  }

  memset(&coutput, 0, sizeof(coutput));
  coutput.loutput = loutput;
  loutput->backend_data = &coutput;
  loutput->methods = &ps_draw_methods;
  loutput->file = output;

  /* create a fake surface */
  fakecs = cairo_ps_surface_create_for_stream(NULL, NULL, 1, 1);
  coutput.surface = fakecs;

  /* recurse once for preparing sizes and positions using the fake surface */
  loutput->drawing = LSTOPO_DRAWING_PREPARE;
  topo_cairo_paint(&coutput);
  loutput->drawing = LSTOPO_DRAWING_DRAW;
  cairo_surface_destroy(fakecs);

  /* create the actual surface with the right size */
  cs = cairo_ps_surface_create_for_stream(topo_cairo_write, loutput->file, loutput->width, loutput->height);
  coutput.surface = cs;

  /* ready */
  declare_colors(loutput);
  lstopo_prepare_custom_styles(loutput);

  topo_cairo_paint(&coutput);
  cairo_surface_flush(coutput.surface);
  cairo_surface_destroy(coutput.surface);

  if (output != stdout)
    fclose(output);

  destroy_colors();
  return 0;
}
#endif /* CAIRO_HAS_PS_SURFACE */


#ifdef CAIRO_HAS_SVG_SURFACE
/* SVG back-end */

static struct draw_methods svg_draw_methods = {
  NULL,
  topo_cairo_box,
  topo_cairo_line,
  topo_cairo_text,
  topo_cairo_textsize,
};

int
output_svg(struct lstopo_output *loutput, const char *filename)
{
  struct lstopo_cairo_output coutput;
  FILE *output;
  cairo_surface_t *fakecs, *cs;

  output = open_output(filename, loutput->overwrite);
  if (!output) {
    fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
    return -1;
  }

  memset(&coutput, 0, sizeof(coutput));
  coutput.loutput = loutput;
  loutput->backend_data = &coutput;
  loutput->methods = &svg_draw_methods;
  loutput->file = output;

  /* create a fake surface */
  fakecs = cairo_svg_surface_create_for_stream(NULL, NULL, 1, 1);
  coutput.surface = fakecs;

  /* recurse once for preparing sizes and positions using the fake surface */
  loutput->drawing = LSTOPO_DRAWING_PREPARE;
  topo_cairo_paint(&coutput);
  loutput->drawing = LSTOPO_DRAWING_DRAW;
  cairo_surface_destroy(fakecs);

  /* create the actual surface with the right size */
  cs = cairo_svg_surface_create_for_stream(topo_cairo_write, loutput->file, loutput->width, loutput->height);
  coutput.surface = cs;

  /* ready */
  declare_colors(loutput);
  lstopo_prepare_custom_styles(loutput);

  topo_cairo_paint(&coutput);
  cairo_surface_flush(coutput.surface);
  cairo_surface_destroy(coutput.surface);

  if (output != stdout)
    fclose(output);

  destroy_colors();
  return 0;
}
#endif /* CAIRO_HAS_SVG_SURFACE */
