/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <private/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif /* HAVE_SETLOCALE */

#ifdef HAVE_NL_LANGINFO
#include <langinfo.h>
#endif /* HAVE_NL_LANGINFO */

#ifdef HAVE_PUTWC
#include <wchar.h>
#endif /* HAVE_PUTWC */

#ifdef HWLOC_HAVE_LIBTERMCAP
#include <curses.h>
#include <term.h>
#endif /* HWLOC_HAVE_LIBTERMCAP */

#include "lstopo.h"

#define indent(output, i) \
  fprintf (output, "%*s", (int) i, "");

/*
 * Console fashion text output
 */

/* Recursively output topology in a console fashion */
static void
output_topology (hwloc_topology_t topology, hwloc_obj_t l, hwloc_obj_t parent, FILE *output, int i, int verbose_mode) {
  int x;
  const char * indexprefix = "#";
  char line[256];

  if (verbose_mode <= 1
      && parent && parent->arity == 1 && hwloc_cpuset_isequal(l->cpuset, parent->cpuset)) {
    /* in non-verbose mode, merge objects with their parent is they are exactly identical */
    fprintf(output, " + ");
  } else {
    if (parent)
      fprintf(output, "\n");
    indent (output, 2*i);
    i++;
  }
  hwloc_obj_snprintf (line, sizeof(line), topology, l, indexprefix, verbose_mode-1);
  fprintf(output, "%s", line);
  if (l->arity || (!i && !l->arity))
    {
      for (x=0; x<l->arity; x++)
	output_topology (topology, l->children[x], l, output, i, verbose_mode);
  }
}

void output_console(hwloc_topology_t topology, const char *filename, int verbose_mode)
{
  unsigned topodepth;
  FILE *output;

  if (!filename || !strcmp(filename, "-"))
    output = stdout;
  else {
    output = open_file(filename, "w"); 
    if (!output) {
      fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
      return;
    }
  }

  topodepth = hwloc_topology_get_depth(topology);

  /*
   * if verbose_mode == 0, only print the summary.
   * if verbose_mode == 1, only print the topology tree.
   * if verbose_mode > 1, print both.
   */

  if (verbose_mode >= 1) {
    output_topology (topology, hwloc_get_system_obj(topology), NULL, output, 0, verbose_mode);
    fprintf(output, "\n");
  }

  if (verbose_mode > 1 || !verbose_mode) {
    unsigned depth;
    for (depth = 0; depth < topodepth; depth++) {
      hwloc_obj_type_t type = hwloc_get_depth_type (topology, depth);
      unsigned nbobjs = hwloc_get_nbobjs_by_depth (topology, depth);
      indent(output, depth);
      fprintf (output, "depth %u:\t%u %s%s (type #%u)\n",
	       depth, nbobjs, hwloc_obj_type_string (type), nbobjs>1?"s":"", type);
    }
  }

  if (verbose_mode > 1)
    if (!hwloc_topology_is_thissystem(topology))
      fprintf (output, "Topology not from this system\n");

  fclose(output);
}

/*
 * Pretty text output
 */

/* Uses unicode bars if available */
#ifdef HAVE_PUTWC
typedef wchar_t character;
#define PRIchar "lc"
#define putcharacter(c,f) putwc(c,f)
#else /* HAVE_PUTWC */
typedef unsigned char character;
#define PRIchar "c"
#define putcharacter(c,f) putc(c,f)
#endif /* HAVE_PUTWC */

#ifdef HWLOC_HAVE_LIBTERMCAP
static int myputchar(int c) {
  return putcharacter(c, stdout);
}
#endif /* HWLOC_HAVE_LIBTERMCAP */

/* Off-screen rendering buffer */
struct cell {
  character c;
#ifdef HWLOC_HAVE_LIBTERMCAP
  int r, g, b;
#endif /* HWLOC_HAVE_LIBTERMCAP */
};

struct display {
  struct cell **cells;
  int width;
  int height;
  int utf8;
};

/* Allocate the off-screen buffer */
static void *
text_start(void *output, int width, int height)
{
  int j, i;
  struct display *disp = malloc(sizeof(*disp));
  /* terminals usually have narrow characters, so let's make them wider */
  width /= (gridsize/2);
  height /= gridsize;
  disp->cells = malloc(height * sizeof(*disp->cells));
  disp->width = width;
  disp->height = height;
  for (j = 0; j < height; j++) {
    disp->cells[j] = calloc(width, sizeof(**disp->cells));
    for (i = 0; i < width; i++)
      disp->cells[j][i].c = ' ';
  }
#ifdef HAVE_NL_LANGINFO
  disp->utf8 = !strcmp(nl_langinfo(CODESET), "UTF-8");
#endif /* HAVE_NL_LANGINFO */
  return disp;
}

#ifdef HWLOC_HAVE_LIBTERMCAP
/* Standard terminfo strings */
static char *oc, *initc = NULL, *initp = NULL, *bold, *normal, *setaf, *setab, *setf, *setb, *scp;

/* Set text color to bright white or black according to the background */
static int set_textcolor(int rr, int gg, int bb)
{
  if (!initc && !initp && rr + gg + bb < 2) {
    if (bold)
      tputs(bold, 1, myputchar);
    return 7;
  } else {
    if (normal)
      tputs(normal, 1, myputchar);
    return 0;
  }
}

static void
set_color(int r, int g, int b)
{
  char *toput;
  int color, textcolor;

  if (initc || initp) {
    /* Can set rgb color, easy */
    color = rgb_to_color(r, g, b) + 16;
    textcolor = 0;
  } else {
    /* Magic trigger: it seems to separate colors quite well */
    int rr = r >= 0xe0;
    int gg = g >= 0xe0;
    int bb = b >= 0xe0;

    if (setab)
      /* ANSI colors */
      color = (rr << 0) | (gg << 1) | (bb << 2);
    else
      /* Legacy colors */
      color = (rr << 2) | (gg << 1) | (bb << 0);
    textcolor = set_textcolor(rr, gg, bb);
  }

  /* And now output magic string to TTY */
  if (setaf) {
    /* foreground */
    if ((toput = tparm(setaf, textcolor)))
      tputs(toput, 1, myputchar);
    /* background */
    if ((toput = tparm(setab, color)))
      tputs(toput, 1, myputchar);
  } else if (setf) {
    /* foreground */
    if ((toput = tparm(setf, textcolor)))
      tputs(toput, 1, myputchar);
    /* background */
    if ((toput = tparm(setb, color)))
      tputs(toput, 1, myputchar);
  } else if (scp) {
    /* pair */
    if ((toput = tparm(scp, color)))
      tputs(toput, 1, myputchar);
  }
}
#endif /* HWLOC_HAVE_LIBTERMCAP */

/* We we can, allocate rgb colors */
static void
text_declare_color(void *output, int r, int g, int b)
{
#ifdef HWLOC_HAVE_LIBTERMCAP
  int color = declare_color(r, g, b);
  /* Yes, values seem to range from 0 to 1000 inclusive */
  int rr = (r * 1001) / 256;
  int gg = (g * 1001) / 256;
  int bb = (b * 1001) / 256;
  char *toput;

  if (initc) {
    if ((toput = tparm(initc, color + 16, rr, gg, bb)))
      tputs(toput, 1, myputchar);
  } else if (initp) {
    if ((toput = tparm(initp, color + 16, 0, 0, 0, rr, gg, bb)))
      tputs(toput, 1, myputchar);
  }
#endif /* HWLOC_HAVE_LIBTERMCAP */
}

/* output text, erasing any previous content */
static void
put(struct display *disp, int x, int y, character c, int r, int g, int b)
{
  if (x >= disp->width || y >= disp->height) {
    /* fprintf(stderr, "%"PRIchar" overflowed to (%d,%d)\n", c, x, y); */
    return;
  }
  disp->cells[y][x].c = c;
#ifdef HWLOC_HAVE_LIBTERMCAP
  if (r != -1) {
    disp->cells[y][x].r = r;
    disp->cells[y][x].g = g;
    disp->cells[y][x].b = b;
  }
#endif /* HWLOC_HAVE_LIBTERMCAP */
}

/* Where bars of a character go to */
enum {
  up = (1<<0),
  down = (1<<1),
  left = (1<<2),
  right = (1<<3),
};

/* Convert a bar character into its directions */
static int
to_directions(struct display *disp, character c)
{
#ifdef HAVE_PUTWC
  if (disp->utf8) {
    switch (c) {
      case L'\x250c': return down|right;
      case L'\x2510': return down|left;
      case L'\x2514': return up|right;
      case L'\x2518': return up|left;
      case L'\x2500': return left|right;
      case L'\x2502': return down|up;
      case L'\x2577': return down;
      case L'\x2575': return up;
      case L'\x2576': return right;
      case L'\x2574': return left;
      case L'\x251c': return down|up|right;
      case L'\x2524': return down|up|left;
      case L'\x252c': return down|left|right;
      case L'\x2534': return up|left|right;
      case L'\x253c': return down|up|left|right;
      default: return 0;
    }
  } else
#endif /* HAVE_PUTWC */
  {
    switch (c) {
      case L'-': return left|right;
      case L'|': return down|up;
      case L'/':
      case L'\\':
      case L'+': return down|up|left|right;
      default: return 0;
    }
  }
}

/* Produce a bar character given the wanted directions */
static character
from_directions(struct display *disp, int direction)
{
#ifdef HAVE_PUTWC
  if (disp->utf8) {
    static const wchar_t chars[] = {
      [down|right]	= L'\x250c',
      [down|left]	= L'\x2510',
      [up|right]	= L'\x2514',
      [up|left]		= L'\x2518',
      [left|right]	= L'\x2500',
      [down|up]		= L'\x2502',
      [down]		= L'\x2577',
      [up]		= L'\x2575',
      [right]		= L'\x2576',
      [left]		= L'\x2574',
      [down|up|right]	= L'\x251c',
      [down|up|left]	= L'\x2524',
      [down|left|right]	= L'\x252c',
      [up|left|right]	= L'\x2534',
      [down|up|left|right]	= L'\x253c',
      [0]		= L' ',
    };
    return chars[direction];
  } else
#endif /* HAVE_PUTWC */
  {
    static const char chars[] = {
      [down|right]	= '/',
      [down|left]	= '\\',
      [up|right]	= '\\',
      [up|left]		= '/',
      [left|right]	= '-',
      [down|up]		= '|',
      [down]		= '|',
      [up]		= '|',
      [right]		= '-',
      [left]		= '-',
      [down|up|right]	= '+',
      [down|up|left]	= '+',
      [down|left|right]	= '+',
      [up|left|right]	= '+',
      [down|up|left|right]	= '+',
      [0]		= ' ',
    };
    return chars[direction];
  }
}

/* output bars, merging with existing bars: `andnot' are removed, `or' are added */
static void
merge(struct display *disp, int x, int y, int or, int andnot, int r, int g, int b)
{
  character current = disp->cells[y][x].c;
  int directions = (to_directions(disp, current) & ~andnot) | or;
  put(disp, x, y, from_directions(disp, directions), r, g, b);
}

/* Now we can implement the standard drawing helpers */
static void
text_box(void *output, int r, int g, int b, unsigned depth, unsigned x1, unsigned width, unsigned y1, unsigned height)
{
  struct display *disp = output;
  int x2, y2, i, j;
  x1 /= (gridsize/2);
  width /= (gridsize/2);
  y1 /= gridsize;
  height /= gridsize;
  x2 = x1 + width - 1;
  y2 = y1 + height - 1;

  /* Corners */
  merge(disp, x1, y1, down|right, 0, r, g, b);
  merge(disp, x2, y1, down|left, 0, r, g, b);
  merge(disp, x1, y2, up|right, 0, r, g, b);
  merge(disp, x2, y2, up|left, 0, r, g, b);

  for (i = 1; i < width - 1; i++) {
    /* upper line */
    merge(disp, x1 + i, y1, left|right, down, r, g, b);
    /* lower line */
    merge(disp, x1 + i, y2, left|right, up, r, g, b);
  }
  for (j = 1; j < height - 1; j++) {
    /* left line */
    merge(disp, x1, y1 + j, up|down, right, r, g, b);
    /* right line */
    merge(disp, x2, y1 + j, up|down, left, r, g, b);
  }
  for (j = y1 + 1; j < y2; j++) {
    for (i = x1 + 1; i < x2; i++) {
      put(disp, i, j, ' ', r, g, b);
    }
  }
}

static void
text_line(void *output, int r, int g, int b, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
  struct display *disp = output;
  int i, j;
  int z;
  x1 /= (gridsize/2);
  y1 /= gridsize;
  x2 /= (gridsize/2);
  y2 /= gridsize;

  /* Canonicalize coordinates */
  if (x1 > x2) {
    z = x1;
    x1 = x2;
    x2 = z;
  }
  if (y1 > y2) {
    z = y1;
    y1 = y2;
    y2 = z;
  }

  /* vertical/horizontal should be enough, but should mix with existing
   * characters for better output ! */

  if (x1 == x2) {
    /* Vertical */
    if (y1 == y2) {
      /* Hu ?! That's a point, let's do nothing... */
    } else {
      /* top */
      merge(disp, x1, y1, down, 0, -1, -1, -1);
      /* bottom */
      merge(disp, x1, y2, up, 0, -1, -1, -1);
    }
    for (j = y1 + 1; j < y2; j++)
      merge(disp, x1, j, down|up, 0, -1, -1, -1);
  } else if (y1 == y2) {
    /* Horizontal */
    /* left */
    merge(disp, x1, y1, right, 0, -1, -1, -1);
    /* right */
    merge(disp, x2, y1, left, 0, -1, -1, -1);
    for (i = x1 + 1; i < x2; i++)
      merge(disp, i, y1, left|right, 0, -1, -1, -1);
  } else {
    /* Unsupported, sorry */
  }
}

static void
text_text(void *output, int r, int g, int b, int size, unsigned depth, unsigned x, unsigned y, const char *text)
{
  struct display *disp = output;
  x /= (gridsize/2);
  y /= gridsize;
  for ( ; *text; text++)
    put(disp, x++, y, *text, -1, -1, -1);
}

static struct draw_methods text_draw_methods = {
  .start = text_start,
  .declare_color = text_declare_color,
  .box = text_box,
  .line = text_line,
  .text = text_text,
};

void output_text(hwloc_topology_t topology, const char *filename, int verbose_mode)
{
  FILE *output;
  struct display *disp;
  int i, j;
  int lr, lg, lb;
#ifdef HWLOC_HAVE_LIBTERMCAP
  int term = 0;
#endif

  if (!filename || !strcmp(filename, "-"))
    output = stdout;
  else {
    output = open_file(filename, "w"); 
    if (!output) {
      fprintf(stderr, "Failed to open %s for writing (%s)\n", filename, strerror(errno));
      return;
    }
  }

  /* Try to use utf-8 characters */
#ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif /* HAVE_SETLOCALE */

#ifdef HWLOC_HAVE_LIBTERMCAP
  /* If we are outputing to a tty, use colors */
  if (output == stdout && isatty(STDOUT_FILENO)) {
    term = !setupterm(NULL, STDOUT_FILENO, NULL);

    if (term) {
      int colors, pairs;

      /* reset colors */
      if ((oc = tgetstr("oc", NULL)))
	tputs(oc, 1, myputchar);

      /* Get terminfo(5) strings */
      pairs = tgetnum("pa");
      initp = tgetstr("Ip", NULL);
      scp = tgetstr("sp", NULL);
      if (pairs <= 16 || !initp || !scp) {
	/* Can't use pairs to define our own colors */
	initp = NULL;
	colors = tgetnum("Co");
	if (colors > 16) {
	  if (tgetflag("cc"))
	    initc = tgetstr("Ic", NULL);
	}
	setaf = tgetstr("AF", NULL);
	setab = tgetstr("AB", NULL);
	setf = tgetstr("Sf", NULL);
	setb = tgetstr("Sb", NULL);
      }
      if (tgetflag("lhs"))
	/* Sorry, I'm lazy to convert colors and I don't know any terminal
	 * using LHS anyway */
	initc = initp = 0;
      bold = tgetstr("md", NULL);
      normal = tgetstr("me", NULL);
    }
  }
#endif /* HWLOC_HAVE_LIBTERMCAP */

  disp = output_draw_start(&text_draw_methods, topology, output);
  output_draw(&text_draw_methods, topology, disp);

  lr = lg = lb = -1;
  for (j = 0; j < disp->height; j++) {
    for (i = 0; i < disp->width; i++) {
#ifdef HWLOC_HAVE_LIBTERMCAP
      if (term) {
	/* TTY output, use colors */
	int r = disp->cells[j][i].r;
	int g = disp->cells[j][i].g;
	int b = disp->cells[j][i].b;

	/* Avoid too much work for the TTY */
	if (r != lr || g != lg || b != lb) {
	  set_color(r, g, b);
	  lr = r;
	  lg = g;
	  lb = b;
	}
      }
#endif /* HWLOC_HAVE_LIBTERMCAP */
      putcharacter(disp->cells[j][i].c, output);
    }
#ifdef HWLOC_HAVE_LIBTERMCAP
    /* Keep the rest of the line black */
    if (term) {
      lr = lg = lb = 0;
      set_color(lr, lg, lb);
    }
#endif /* HWLOC_HAVE_LIBTERMCAP */
    putcharacter('\n', output);
  }
}
