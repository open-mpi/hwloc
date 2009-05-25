/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <topology.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lstopo.h"

#define EPOXY_R_COLOR 0xe7
#define EPOXY_G_COLOR 0xff
#define EPOXY_B_COLOR 0xb5

#define SOCKET_R_COLOR 0xde
#define SOCKET_G_COLOR 0xde
#define SOCKET_B_COLOR 0xde

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

#define MACHINE_R_COLOR 0xff
#define MACHINE_G_COLOR 0xff
#define MACHINE_B_COLOR 0xff

#define FAKE_R_COLOR 0xff
#define FAKE_G_COLOR 0xff
#define FAKE_B_COLOR 0xff

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
 * foo_draw functions take a OBJ, computes which size it needs, recurse into
 * sublevels with null_draw_methods to recursively compute the needed size
 * without actually drawing anything, then draw things about OBJ (chip draw,
 * cache size information etc) at (X,Y), recurse into sublevels again to
 * actually draw things, and return in RETWIDTH and RETHEIGHT the amount of
 * space that the drawing took.
 *
 * For generic detailed comments, see the node_draw function.
 */

typedef void (*foo_draw)(struct draw_methods *methods, topo_obj_t obj, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight);

static foo_draw get_type_fun(topo_obj_type_t type);

/*
 * Helper to recurse into sublevels
 */

#define RECURSE(obj, methods, separator) { \
  topo_obj_t *sublevels = obj->children; \
  unsigned numsublevels = obj->arity; \
  unsigned width, height; \
  foo_draw fun; \
  if (obj->arity) { \
    fun = get_type_fun(sublevels[0]->type); \
    int i; \
    for (i = 0; i < numsublevels; i++) { \
      fun(methods, sublevels[i], type, output, depth-1, x + totwidth, &width, y + myheight, &height); \
      totwidth += width + (separator); \
      if (height > maxheight) \
	maxheight = height; \
    } \
    totwidth -= (separator); \
  } \
}

#define size_value(size) (size < 4*1024 && size % 1024 ? size : size < 4*1024*1024 && (size / 1024) % 1024 ? size / 1024 : size / (1024*1024))
#define size_unit(size) (size < 4*1024 && size % 1024 ? "KB" : size < 4*1024*1024 && (size / 1024) % 1024 ? "MB" : "GB")

static void
proc_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  char text[64];

  *retwidth = 6*FONT_SIZE;
  *retheight = UNIT + FONT_SIZE + UNIT;

  methods->box(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "CPU#%d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);
}

static void
cache_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT + FONT_SIZE + UNIT + UNIT;
  unsigned totwidth = 0, maxheight = 0;
  char text[64];
  int textwidth;

  /* Do not separate objects when in L1 (SMT) */
  RECURSE(level, &null_draw_methods, level->cache_depth > 1 ? UNIT : 0);

  if (level->physical_index == -1)
    textwidth = 7*FONT_SIZE;
  else
    textwidth = 8*FONT_SIZE;

  if (totwidth < textwidth)
    totwidth = textwidth;

  *retwidth = totwidth;
  *retheight = myheight + maxheight;
  if (!maxheight)
    /* No sublevels, remove the separator */
    *retheight -= UNIT;

  methods->box(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR, depth, x, *retwidth, y, myheight - UNIT);

  if (level->physical_index == -1)
    snprintf(text, sizeof(text), "L%u (%lu%s)", level->cache_depth,
		    size_value(level->memory_kB),
		    size_unit(level->memory_kB));
  else
    snprintf(text, sizeof(text), "L%u#%d (%lu%s)", level->cache_depth, level->physical_index,
		    size_value(level->memory_kB),
		    size_unit(level->memory_kB));
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = 0;
  RECURSE(level, methods, level->cache_depth > 1 ? UNIT : 0);
}

static void
core_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT + FONT_SIZE + UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  RECURSE(level, &null_draw_methods, 0);

  if (totwidth < 6*FONT_SIZE)
    totwidth = 6*FONT_SIZE;
  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  if (maxheight)
    *retheight += UNIT;

  methods->box(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Core#%d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(level, methods, 0);
}

static void
socket_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT + FONT_SIZE + UNIT;;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  RECURSE(level, &null_draw_methods, UNIT);

  if (totwidth < 6*FONT_SIZE)
    totwidth = 6*FONT_SIZE;
  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  if (maxheight)
    *retheight += UNIT;

  methods->box(output, SOCKET_R_COLOR, SOCKET_G_COLOR, SOCKET_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Socket#%d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(level, methods, UNIT);
}

static void
node_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  /* Reserve room for the separator, heading memory box and separator */
  unsigned myheight = UNIT + UNIT + FONT_SIZE + UNIT + UNIT;;
  /* Put a vertical separator on the left */
  unsigned totwidth = UNIT;
  unsigned maxheight = 0;
  char text[64];

  /* Compute the size needed by sublevels */
  RECURSE(level, &null_draw_methods, UNIT);

  /* Make sure we have room for text */
  if (totwidth < 10*FONT_SIZE)
    totwidth = 10*FONT_SIZE;

  /* Add vertical separator on the right */
  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  /* There were objects, add separator at the bottom */
  if (maxheight)
    *retheight += UNIT;

  /* Draw the epoxy box */
  methods->box(output, EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR, depth, x, *retwidth, y, *retheight);
  /* Draw the memory box */
  methods->box(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR, depth-1, x + UNIT, *retwidth - 2 * UNIT, y + UNIT, myheight - 2 * UNIT);

  /* Output text */
  snprintf(text, sizeof(text), "Node#%d (%lu%s)", level->physical_index,
		  size_value(level->memory_kB),
		  size_unit(level->memory_kB));
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-2, x + 2 * UNIT, y + 2 * UNIT, text);

  /* Restart, now really drawing sublevels */
  totwidth = UNIT;
  RECURSE(level, methods, UNIT);
}

static void
machine_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT + FONT_SIZE + UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  RECURSE(level, &null_draw_methods, UNIT);

  if (totwidth < 10*FONT_SIZE)
    totwidth = 10*FONT_SIZE;

  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;
  if (maxheight)
    *retheight += UNIT;

  methods->box(output, MACHINE_R_COLOR, MACHINE_G_COLOR, MACHINE_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Machine (%lu%s)",
		  size_value(level->memory_kB),
		  size_unit(level->memory_kB));
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(level, methods, UNIT);
}

static void
fake_draw(struct draw_methods *methods, topo_obj_t level, topo_obj_type_t type, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = UNIT + FONT_SIZE + UNIT;
  unsigned totwidth = UNIT, maxheight = 0;
  char text[64];

  RECURSE(level, &null_draw_methods, UNIT);

  maxheight += UNIT;
  if (totwidth < 6*FONT_SIZE)
    totwidth = 6*FONT_SIZE;
  *retwidth = totwidth + UNIT;
  *retheight = myheight + maxheight;

  methods->box(output, FAKE_R_COLOR, FAKE_G_COLOR, FAKE_B_COLOR, depth, x, *retwidth, y, *retheight);

  snprintf(text, sizeof(text), "Fake#%d", level->physical_index);
  methods->text(output, 0, 0, 0, FONT_SIZE, depth-1, x + UNIT, y + UNIT, text);

  totwidth = UNIT;
  RECURSE(level, methods, UNIT);
}

static void
fig(struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned y)
{
  unsigned totwidth = 0, maxheight = 0;
  topo_obj_type_t type = level->type;

  machine_draw(methods, level, type, output, depth, x, &totwidth, y, &maxheight);
}

/*
 * given a type, return a pointer FUN to the function that draws it.
 */
static foo_draw
get_type_fun(topo_obj_type_t type)
{
  switch (type) {
    case TOPO_OBJ_MACHINE: return machine_draw;
    case TOPO_OBJ_NODE: return node_draw;
    case TOPO_OBJ_SOCKET: return socket_draw;
    case TOPO_OBJ_CACHE: return cache_draw;
    case TOPO_OBJ_CORE: return core_draw;
    case TOPO_OBJ_PROC: return proc_draw;
    case TOPO_OBJ_FAKE: return fake_draw;
  }
  return NULL;
}

/*
 * Dummy drawing methods to get the bounding box.
 */

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
  fig(&getmax_draw_methods, topo_get_machine_object(topology), &coords, 100, 0, 0);
  output = methods->start(output, coords.x, coords.y);
  methods->declare_color(output, EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR);
  methods->declare_color(output, SOCKET_R_COLOR, SOCKET_G_COLOR, SOCKET_B_COLOR);
  methods->declare_color(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR);
  methods->declare_color(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR);
  methods->declare_color(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR);
  methods->declare_color(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR);
  methods->declare_color(output, MACHINE_R_COLOR, MACHINE_G_COLOR, MACHINE_B_COLOR);
  methods->declare_color(output, FAKE_R_COLOR, FAKE_G_COLOR, FAKE_B_COLOR);
  return output;
}

void
output_draw(struct draw_methods *methods, topo_topology_t topology, void *output)
{
  fig(methods, topo_get_machine_object(topology), output, 100, 0, 0);
}
