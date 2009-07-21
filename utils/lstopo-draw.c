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

#define MACHINE_R_COLOR EPOXY_R_COLOR
#define MACHINE_G_COLOR EPOXY_G_COLOR
#define MACHINE_B_COLOR EPOXY_B_COLOR

#define NODE_R_COLOR ((EPOXY_R_COLOR * 100) / 110)
#define NODE_G_COLOR ((EPOXY_G_COLOR * 100) / 110)
#define NODE_B_COLOR ((EPOXY_B_COLOR * 100) / 110)

#define SYSTEM_R_COLOR 0xff
#define SYSTEM_G_COLOR 0xff
#define SYSTEM_B_COLOR 0xff

#define MISC_R_COLOR 0xff
#define MISC_G_COLOR 0xff
#define MISC_B_COLOR 0xff

/* preferred width/height compromise */
#define RATIO (4./3.)

static void null(void) {}

static struct draw_methods null_draw_methods = {
  .start = (void*) null,
  .declare_color = (void*) null,
  .box = (void*) null,
  .line = (void*) null,
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

typedef void (*foo_draw)(topo_topology_t topology, struct draw_methods *methods, topo_obj_t obj, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight);

static foo_draw get_type_fun(topo_obj_type_t type);

/*
 * Helper to recurse into sublevels, either horizontally or vertically
 * Updates caller's totwidth/myheight and maxwidth/maxheight
 * Needs textwidth, border, topology, output, depth, x and y
 */

#define RECURSE_BEGIN(obj, border) do { \
  topo_obj_t *sublevels = obj->children; \
  unsigned numsublevels = obj->arity; \
  unsigned width, height; \
  unsigned maxwidth, maxheight; \
  maxwidth = maxheight = 0; \
  totwidth = (border) + mywidth; \
  totheight = (border) + myheight; \
  foo_draw fun; \
  if (obj->arity) { \
    fun = get_type_fun(sublevels[0]->type); \
    int i; \
    /* Iterate over subobjects */ \
    for (i = 0; i < numsublevels; i++) { \

      /* Recursive call */
#define RECURSE_CALL_FUN(obj, methods) \
      fun(topology, methods, sublevels[i], output, depth-1, x + totwidth, &width, y + totheight, &height) \

#define RECURSE_END_HORIZ(obj, separator, border) \
      /* Add the subobject's width and separator */ \
      totwidth += width + (separator); \
      /* Update maximum height */ \
      if (height > maxheight) \
	maxheight = height; \
    } \
    \
    /* Remove spurious separator on the right */ \
    totwidth -= (separator); \
    \
    /* Make sure there is width for the heading text */ \
    if (totwidth < textwidth) \
      totwidth = textwidth; \
    /* Add border on the right */ \
    totwidth += (border); \
    /* Add subobjects height */ \
    totheight += maxheight; \
    /* And add border below if there are subobjects */ \
    if (maxheight) \
      totheight += (border); \
    /* Update returned values */ \
    *retwidth = totwidth; \
    *retheight = totheight; \
  } \
} while(0)

#define RECURSE_END_VERT(obj, separator, border) \
      /* Add the subobject's height and separator */ \
      totheight += height + (separator); \
      if (width > maxwidth) \
      /* Update maximum width */ \
	maxwidth = width; \
    } \
    /* Remove spurious separator at the bottom */ \
    totheight -= (separator); \
    \
    /* Make sure there is width for the heading text */ \
    if (maxwidth < textwidth) \
      maxwidth = textwidth; \
    /* Add subobjects width */ \
    totwidth += maxwidth; \
    /* And add border below if there are subobjects */ \
    if (maxwidth) \
      totwidth += (border); \
    /* Add border at the bottom */ \
    totheight = totheight + (border); \
    /* Update returned values */ \
    *retwidth = totwidth; \
    *retheight = totheight; \
  } \
} while(0)

#define RECURSE_HORIZ(obj, methods, separator, border) \
	RECURSE_BEGIN(obj, border) \
	  RECURSE_CALL_FUN(obj, methods); \
	RECURSE_END_HORIZ(obj, separator, border)

#define RECURSE_VERT(obj, methods, separator, border) \
	RECURSE_BEGIN(obj, border) \
	  RECURSE_CALL_FUN(obj, methods); \
	RECURSE_END_VERT(obj, separator, border)

static int
prefer_vert(topo_topology_t topology, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned y, int separator)
{
  float horiz_ratio, vert_ratio;
  unsigned textwidth = 0;
  unsigned mywidth = 0, myheight = 0;
  unsigned totwidth, *retwidth = &totwidth, totheight, *retheight = &totheight;
  RECURSE_HORIZ(level, &null_draw_methods, separator, 0);
  horiz_ratio = (float)totwidth / totheight;
  RECURSE_VERT(level, &null_draw_methods, separator, 0);
  vert_ratio = (float)totwidth / totheight;
  return ((RATIO / vert_ratio) < (horiz_ratio / RATIO));
}

static void
proc_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  *retwidth = fontsize ? 4*fontsize : gridsize;
  *retheight = gridsize + (fontsize ? (fontsize + gridsize) : 0);

  methods->box(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR, depth, x, *retwidth, y, *retheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }
}

static void
cache_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = gridsize + (fontsize ? (fontsize + gridsize) : 0) + gridsize, totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = fontsize ? (level->os_index == -1 ? 7*fontsize : 9*fontsize) : 0;
  unsigned separator = level->attr->cache.depth > 1 ? gridsize : 0;

  /* Do not separate objects when in L1 (SMT) */
  RECURSE_HORIZ(level, &null_draw_methods, separator, 0);

  methods->box(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR, depth, x, totwidth, y, myheight - gridsize);

  if (fontsize) {
    char text[64];

    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  RECURSE_HORIZ(level, methods, separator, 0);
}

static void
core_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = (fontsize ? (fontsize + gridsize) : 0), totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = 5*fontsize;

  RECURSE_HORIZ(level, &null_draw_methods, 0, gridsize);

  methods->box(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR, depth, x, totwidth, y, totheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  RECURSE_HORIZ(level, methods, 0, gridsize);
}

static void
socket_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = (fontsize ? (fontsize + gridsize) : 0), totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = 6*fontsize;
  int vert = prefer_vert(topology, level, output, depth, x, y, gridsize);

  if (vert)
    RECURSE_VERT(level, &null_draw_methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, &null_draw_methods, gridsize, gridsize);

  methods->box(output, SOCKET_R_COLOR, SOCKET_G_COLOR, SOCKET_B_COLOR, depth, x, totwidth, y, totheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  if (vert)
    RECURSE_VERT(level, methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, methods, gridsize, gridsize);
}

static void
node_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  /* Reserve room for the heading memory box and separator */
  unsigned myheight = (fontsize ? (gridsize + fontsize) : 0) + gridsize + gridsize;
  /* Currently filled height */
  unsigned totheight;
  /* Nothing on the left */
  unsigned mywidth = 0;
  /* Currently filled width */
  unsigned totwidth;
  /* Width of the heading text, thus minimal width */
  unsigned textwidth = 11*fontsize;

  /* Compute the size needed by sublevels */
  RECURSE_HORIZ(level, &null_draw_methods, gridsize, gridsize);

  /* Draw the epoxy box */
  methods->box(output, NODE_R_COLOR, NODE_G_COLOR, NODE_B_COLOR, depth, x, totwidth, y, totheight);
  /* Draw the memory box */
  methods->box(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR, depth-1, x + gridsize, totwidth - 2 * gridsize, y + gridsize, myheight - gridsize);

  if (fontsize) {
    char text[64];
    /* Output text */
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-2, x + 2 * gridsize, y + 2 * gridsize, text);
  }

  /* Restart, now really drawing sublevels */
  RECURSE_HORIZ(level, methods, gridsize, gridsize);
}

static void
machine_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = (fontsize ? (fontsize + gridsize) : 0), totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = 11*fontsize;
  int vert = prefer_vert(topology, level, output, depth, x, y, gridsize);

  if (vert)
    RECURSE_VERT(level, &null_draw_methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, &null_draw_methods, gridsize, gridsize);

  methods->box(output, MACHINE_R_COLOR, MACHINE_G_COLOR, MACHINE_B_COLOR, depth, x, totwidth, y, totheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  if (vert)
    RECURSE_VERT(level, methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, methods, gridsize, gridsize);
}

static void
system_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = (fontsize ? (fontsize + gridsize) : 0), totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = 10*fontsize;
  int vert = prefer_vert(topology, level, output, depth, x, y, gridsize);

  if (vert) {
    if (level->arity > 1 && level->children[0]->type == TOPO_OBJ_MACHINE) {
      mywidth += gridsize;
      totwidth = mywidth;
    }
    RECURSE_VERT(level, &null_draw_methods, gridsize, gridsize);
  } else
    RECURSE_HORIZ(level, &null_draw_methods, gridsize, gridsize);

  methods->box(output, SYSTEM_R_COLOR, SYSTEM_G_COLOR, SYSTEM_B_COLOR, depth, x, totwidth, y, totheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  if (vert) {
    unsigned top = 0, bottom = 0;
    RECURSE_BEGIN(level, gridsize)
      RECURSE_CALL_FUN(level, methods);
      if (level->arity > 1 && level->children[0]->type == TOPO_OBJ_MACHINE) {
	unsigned center = y + totheight + height / 2;
	if (!top)
	  top = center;
	bottom = center;
	methods->line(output, 0, 0, 0, depth, x + mywidth - gridsize, center, x + mywidth, center);
      }
    RECURSE_END_VERT(level, gridsize, gridsize);

    if (level->arity > 1 && level->children[0]->type == TOPO_OBJ_MACHINE)
      methods->line(output, 0, 0, 0, depth, x + mywidth - gridsize, top, x + mywidth - gridsize, bottom);
  } else {
    unsigned left = 0, right = 0;
    RECURSE_BEGIN(level, gridsize)
      RECURSE_CALL_FUN(level, methods);
      if (level->arity > 1 && level->children[0]->type == TOPO_OBJ_MACHINE) {
	unsigned center = x + totwidth + width / 2;
	if (!left)
	  left = center;
	right = center;
	methods->line(output, 0, 0, 0, depth, center, y + myheight - gridsize, center, y + myheight);
      }
    RECURSE_END_HORIZ(level, gridsize, gridsize);

    if (level->arity > 1 && level->children[0]->type == TOPO_OBJ_MACHINE)
      methods->line(output, 0, 0, 0, depth, left, y + myheight - gridsize, right, y + myheight - gridsize);
  }
}

static void
misc_draw(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned *retwidth, unsigned y, unsigned *retheight)
{
  unsigned myheight = (fontsize ? (fontsize + gridsize) : 0), totheight;
  unsigned mywidth = 0, totwidth;
  unsigned textwidth = 6*fontsize;
  int vert = prefer_vert(topology, level, output, depth, x, y, gridsize);

  if (vert)
    RECURSE_VERT(level, &null_draw_methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, &null_draw_methods, gridsize, gridsize);

  methods->box(output, MISC_R_COLOR, MISC_G_COLOR, MISC_B_COLOR, depth, x, totwidth, y, totheight);

  if (fontsize) {
    char text[64];
    topo_obj_snprintf(text, sizeof(text), topology, level, "#", 0);
    methods->text(output, 0, 0, 0, fontsize, depth-1, x + gridsize, y + gridsize, text);
  }

  if (vert)
    RECURSE_VERT(level, methods, gridsize, gridsize);
  else
    RECURSE_HORIZ(level, methods, gridsize, gridsize);
}

static void
fig(topo_topology_t topology, struct draw_methods *methods, topo_obj_t level, void *output, unsigned depth, unsigned x, unsigned y)
{
  unsigned totwidth, totheight;

  system_draw(topology, methods, level, output, depth, x, &totwidth, y, &totheight);
}

/*
 * given a type, return a pointer FUN to the function that draws it.
 */
static foo_draw
get_type_fun(topo_obj_type_t type)
{
  switch (type) {
    case TOPO_OBJ_SYSTEM: return system_draw;
    case TOPO_OBJ_MACHINE: return machine_draw;
    case TOPO_OBJ_NODE: return node_draw;
    case TOPO_OBJ_SOCKET: return socket_draw;
    case TOPO_OBJ_CACHE: return cache_draw;
    case TOPO_OBJ_CORE: return core_draw;
    case TOPO_OBJ_PROC: return proc_draw;
    case TOPO_OBJ_MISC: return misc_draw;
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

static void
getmax_line(void *output, int r, int g, int b, unsigned depth, unsigned x1, unsigned y1, unsigned x2, unsigned y2)
{
  struct coords *coords = output;

  if (x2 > coords->x)
    coords->x = x2;
  if (y2 > coords->y)
    coords->y = y2;
}

static struct draw_methods getmax_draw_methods = {
  .start = (void*) null,
  .declare_color = (void*) null,
  .box = getmax_box,
  .line = getmax_line,
  .text = (void*) null,
};

void *
output_draw_start(struct draw_methods *methods, topo_topology_t topology, void *output)
{
  struct coords coords = { .x = 0, .y = 0};
  fig(topology, &getmax_draw_methods, topo_get_system_obj(topology), &coords, 100, 0, 0);
  output = methods->start(output, coords.x, coords.y);
  methods->declare_color(output, 0, 0, 0);
  methods->declare_color(output, NODE_R_COLOR, NODE_G_COLOR, NODE_B_COLOR);
  methods->declare_color(output, SOCKET_R_COLOR, SOCKET_G_COLOR, SOCKET_B_COLOR);
  methods->declare_color(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR);
  methods->declare_color(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR);
  methods->declare_color(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR);
  methods->declare_color(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR);
  methods->declare_color(output, MACHINE_R_COLOR, MACHINE_G_COLOR, MACHINE_B_COLOR);
  methods->declare_color(output, SYSTEM_R_COLOR, SYSTEM_G_COLOR, SYSTEM_B_COLOR);
  methods->declare_color(output, MISC_R_COLOR, MISC_G_COLOR, MISC_B_COLOR);
  return output;
}

void
output_draw(struct draw_methods *methods, topo_topology_t topology, void *output)
{
	fig(topology, methods, topo_get_system_obj(topology), output, 100, 0, 0);
}
