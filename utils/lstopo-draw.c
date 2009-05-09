/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/private.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

#define EPOXY_R_COLOR 0xe7
#define EPOXY_G_COLOR 0xff
#define EPOXY_B_COLOR 0xb5

#define DIE_R_COLOR 0xde
#define DIE_G_COLOR 0xde
#define DIE_B_COLOR 0xde

#define MEMORY_R_COLOR 0xef
#define MEMORY_G_COLOR 0xdf
#define MEMORY_B_COLOR 0xde

#define CORE_R_COLOR 0xbe
#define CORE_G_COLOR 0xbe
#define CORE_B_COLOR 0xbe

#define THREAD_R_COLOR 0xff
#define THREAD_G_COLOR 0xff
#define THREAD_B_COLOR 0xff

#define CACHE_R_COLOR 0xff
#define CACHE_G_COLOR 0xff
#define CACHE_B_COLOR 0xff

/* grid unit: 10 pixels */
#define UNIT 10
#define FONT_SIZE 10

static void null(void) {}

static struct draw_methods null_draw_methods = {
  .start = (void*) null,
  .declare_color = (void*) null,
  .box = (void*) null,
  .text = (void*) null,
};

/*
 * given LEVELS[0..NUMLEVELS-1], return all their direct sublevels in
 * SUBLEVELS[0..NUMSUBLEVELS-1], allocated by malloc.
 */

static void
get_sublevels(topo_level_t *levels, unsigned numlevels, topo_level_t **sublevels, unsigned *numsublevels)
{
  unsigned alloced = numlevels * levels[0]->arity;
  topo_level_t *l = malloc(alloced * sizeof(*l));
  unsigned i, j, n;

  n = 0;
  for (i = 0; i < numlevels; i++) {
    for (j = 0; j < levels[i]->arity; j++) {
      if (n >= alloced) {
	alloced *= 2;
	l = realloc(l, alloced * sizeof(*l));
      }
      l[n++] = levels[i]->children[j];
    }
  }
  *sublevels = l;
  *numsublevels = n;
}

/*
 * foo_draw functions take a LEVEL, computes which size it needs, recurse into
 * sublevels with null_draw_methods to recursively compute the needed size
 * without actually drawing anything, then draw things about LEVEL (chip draw,
 * cache size information etc) at (X,Y), recurse into sublevels again to
 * actually draw things, and return in RETWIDTH and RETHEIGHT the amount of
 * space that the drawing took.
 */

typedef void (*foo_draw)(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight);

static int get_sublevels_type(topo_level_t level, topo_level_t **levels, unsigned *numsublevels, enum topo_level_type_e *type, foo_draw *fun);

/*
 * Helper to recurse into sublevels
 */

#define RECURSE(methods, sep) { \
  topo_level_t *sublevels;		\
  unsigned numsublevels; \
  unsigned width, height; \
  foo_draw fun; \
  if (get_sublevels_type(level, &sublevels, &numsublevels, &type, &fun)) { \
    int i; \
    for (i = 0; i < numsublevels; i++) { \
      fun(methods, sublevels[i], type, output, depth-1, x + totwidth, &width, y + myheight, &height); \
      totwidth += width + (sep); \
      if (height > maxheight) \
	maxheight = height; \
    } \
    totwidth -= (sep); \
    free(sublevels); \
  } \
}

#define size_value(size) (size < 4*1024 && size % 1024 ? size : size < 4*1024*1024 && (size / 1024) % 1024 ? size / 1024 : size / (1024*1024))
#define size_unit(size) (size < 4*1024 && size % 1024 ? "KB" : size < 4*1024*1024 && (size / 1024) % 1024 ? "MB" : "GB")

static void
proc_draw(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  char text[64];

  *retwidth = 5*UNIT;
  *retheight = UNIT + FONT_SIZE + UNIT;
  methods->box(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "CPU%d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);
}

static void
cache_draw(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;
  char text[64];

  myheight += UNIT + FONT_SIZE + UNIT + UNIT;

  RECURSE(&null_draw_methods, level->cache_depth > 1 ? UNIT : 0);

  if (totwidth < 8*UNIT)
    totwidth = 8*UNIT;
  *retwidth = totwidth;
  *retheight = myheight + maxheight;

  if (!maxheight)
    *retheight -= UNIT;
  methods->box(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR, depth, x, *retwidth, y, myheight - UNIT);

  snprintf(text, sizeof(text), "L%u %d - %lu%s", level->cache_depth, level->physical_index,
		  size_value(level->memory_kB),
		  size_unit(level->memory_kB));
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = 0;
  RECURSE(methods, level->cache_depth > 1 ? UNIT : 0);
}

static void
core_draw(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  myheight += FONT_SIZE + UNIT;

  RECURSE(&null_draw_methods, 0);

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight + (maxheight?UNIT:0);
  methods->box(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Core %d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(methods, 0);
}

static void
die_draw(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  myheight += FONT_SIZE + UNIT;

  RECURSE(&null_draw_methods, UNIT);
  maxheight += UNIT;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  methods->box(output, DIE_R_COLOR, DIE_G_COLOR, DIE_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Die %d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(methods, UNIT);
}

static void
node_draw(struct draw_methods *methods, topo_level_t level, enum topo_level_type_e type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  myheight += UNIT + FONT_SIZE + UNIT + UNIT;

  RECURSE(&null_draw_methods, UNIT);
  maxheight += UNIT;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;

  methods->box(output, EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR, depth, x, *retwidth, y, *retheight);
  methods->box(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR, depth-1, x + UNIT, *retwidth - 2 * UNIT, y + UNIT, myheight - 2 * UNIT);
  snprintf(text, sizeof(text), "Node %d - %lu%s", level->physical_index,
		  size_value(level->memory_kB),
		  size_unit(level->memory_kB));
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-2, x + 2 * UNIT, y + 2 * UNIT, text);

  totwidth = UNIT;
  RECURSE(methods, UNIT);
}

static void
fig(struct draw_methods *methods, topo_level_t level, void *output, unsigned depth, unsigned x, unsigned y)
{
  unsigned myheight = 0;
  unsigned totwidth = 0, maxheight = 0;
  enum topo_level_type_e type = level->type;

  RECURSE(methods, UNIT);
}

/*
 * given a LEVEL and a mask of accepted level types, return all its (possibly
 * non-direct) sublevels in SUBLEVELS[0..NUMSUBLEVELS-1], allocated by malloc,
 * and a pointer FUN to the function that draws it.
 */
static int
get_sublevels_type(topo_level_t level, topo_level_t **levels, unsigned *numsublevels, enum topo_level_type_e *type, foo_draw *fun)
{
  unsigned n = 1, n2;
  topo_level_t *l = malloc(sizeof(*l)), *l2;
  l[0] = level;

  while (1) {
#define DO(thetype, _fun) \
    if (*type == TOPO_LEVEL_##thetype) { \
      *type = 0; \
      *fun = _fun; \
      *levels = l; \
      *numsublevels = n; \
      return 1; \
    }
    DO(NODE, node_draw)
    DO(DIE, die_draw)
    DO(CACHE, cache_draw)
    DO(CORE, core_draw)
    DO(PROC, proc_draw)
    if ((*type) != TOPO_LEVEL_MACHINE && (*type) != TOPO_LEVEL_FAKE)
      fprintf(stderr,"urgl, type %d?! Skipping\n", *type);
    if (!l[0]->children)
      return 0;
    get_sublevels(l, n, &l2, &n2);
    free(l);
    l = l2;
    n = n2;
    *type = l[0]->type;
  }
}

struct coords {
  int x;
  int y;
};

static void
getmax_box(void *output, int r, int g, int b, unsigned depth, unsigned x, unsigned width, unsigned y, unsigned height)
{
  struct coords *coords = output;

  if (x + width > coords->x)
    coords->x = x + width;
  if (y + height > coords->y)
    coords->y = y + height;
}

static struct draw_methods getmax_draw_methods = {
  .start = (void*) null,
  .declare_color = (void*) null,
  .box = getmax_box,
  .text = (void*) null,
};

void *
output_draw_start(struct draw_methods *methods, topo_topology_t topology, void *output)
{
  struct coords coords = { .x = 0, .y = 0};
  fig(&getmax_draw_methods, topo_get_machine_level(topology), &coords, 100, 0, 0);
  output = methods->start(output, coords.x, coords.y);
  methods->declare_color(output, EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR);
  methods->declare_color(output, DIE_R_COLOR, DIE_G_COLOR, DIE_B_COLOR);
  methods->declare_color(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR);
  methods->declare_color(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR);
  methods->declare_color(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR);
  methods->declare_color(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR);
  return output;
}

void
output_draw(struct draw_methods *methods, topo_topology_t topology, void *output)
{
  fig(methods, topo_get_machine_level(topology), output, 100, 0, 0);
}
