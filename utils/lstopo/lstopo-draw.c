/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2021 Inria.  All rights reserved.
 * Copyright © 2009-2013, 2015 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "lstopo.h"

#define EPOXY_R_COLOR 0xe7
#define EPOXY_G_COLOR 0xff
#define EPOXY_B_COLOR 0xb5

#define DARK_EPOXY_R_COLOR ((EPOXY_R_COLOR * 100) / 110)
#define DARK_EPOXY_G_COLOR ((EPOXY_G_COLOR * 100) / 110)
#define DARK_EPOXY_B_COLOR ((EPOXY_B_COLOR * 100) / 110)

#define DARKER_EPOXY_R_COLOR ((DARK_EPOXY_R_COLOR * 100) / 110)
#define DARKER_EPOXY_G_COLOR ((DARK_EPOXY_G_COLOR * 100) / 110)
#define DARKER_EPOXY_B_COLOR ((DARK_EPOXY_B_COLOR * 100) / 110)

#ifdef HWLOC_HAVE_GCC_W_MISSING_FIELD_INITIALIZERS
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/* each of these colors must be declared in declare_colors() */
struct lstopo_color BLACK_COLOR = { 0, 0, 0, 0 };
struct lstopo_color WHITE_COLOR = { 0xff, 0xff, 0xff, 0 };
struct lstopo_color PACKAGE_COLOR = { DARK_EPOXY_R_COLOR, DARK_EPOXY_G_COLOR, DARK_EPOXY_B_COLOR, 0 };
struct lstopo_color DIE_COLOR = { EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR, 0 };
struct lstopo_color MEMORY_COLOR = { 0xef, 0xdf, 0xde, 0 };
struct lstopo_color MEMORIES_COLOR = { 0xf2, 0xe8, 0xe8, 0}; /* slightly lighter than MEMORY_COLOR */
struct lstopo_color CORE_COLOR = { 0xbe, 0xbe, 0xbe, 0 };
struct lstopo_color THREAD_COLOR = { 0xff, 0xff, 0xff, 0 };
struct lstopo_color BINDING_COLOR = { 0, 0xff, 0, 0 };
struct lstopo_color DISALLOWED_COLOR = { 0xff, 0, 0, 0 };
struct lstopo_color CACHE_COLOR = { 0xff, 0xff, 0xff, 0 };
struct lstopo_color MACHINE_COLOR = { 0xff, 0xff, 0xff, 0 };
struct lstopo_color GROUP_IN_PACKAGE_COLOR = { EPOXY_R_COLOR, EPOXY_G_COLOR, EPOXY_B_COLOR, 0 };
struct lstopo_color MISC_COLOR = { 0xff, 0xff, 0xff, 0 };
struct lstopo_color PCI_DEVICE_COLOR = { DARKER_EPOXY_R_COLOR, DARKER_EPOXY_G_COLOR, DARKER_EPOXY_B_COLOR, 0 };
struct lstopo_color OS_DEVICE_COLOR = { 0xde, 0xde, 0xde, 0 };
struct lstopo_color BRIDGE_COLOR = { 0xff, 0xff, 0xff, 0 };
#ifdef HWLOC_HAVE_GCC_W_MISSING_FIELD_INITIALIZERS
#pragma GCC diagnostic warning "-Wmissing-field-initializers"
#endif

static struct lstopo_color *colors = NULL;

static struct lstopo_color *
declare_color(struct lstopo_output *loutput, struct lstopo_color *color)
{
  memset(&color->private, 0, sizeof(color->private));

  /* call the backend callback if any */
  if (loutput->methods->declare_color) {
    int ret = loutput->methods->declare_color(loutput, color);
    if (ret < 0)
      return NULL;
  }

  /* insert */
  color->next = colors;
  colors = color;

  return color;
}

void
declare_colors(struct lstopo_output *output)
{
  /* don't bother looking for duplicate colors here,
   * we want to be able to use those structs so always queue them
   */
  declare_color(output, &BLACK_COLOR);
  declare_color(output, &WHITE_COLOR);
  declare_color(output, &PACKAGE_COLOR);
  declare_color(output, &DIE_COLOR);
  declare_color(output, &MEMORY_COLOR);
  declare_color(output, &MEMORIES_COLOR);
  declare_color(output, &CORE_COLOR);
  declare_color(output, &THREAD_COLOR);
  declare_color(output, &BINDING_COLOR);
  declare_color(output, &DISALLOWED_COLOR);
  declare_color(output, &CACHE_COLOR);
  declare_color(output, &MACHINE_COLOR);
  declare_color(output, &GROUP_IN_PACKAGE_COLOR);
  declare_color(output, &MISC_COLOR);
  declare_color(output, &PCI_DEVICE_COLOR);
  declare_color(output, &OS_DEVICE_COLOR);
  declare_color(output, &BRIDGE_COLOR);
}

void
destroy_colors(struct lstopo_output *loutput)
{
  struct lstopo_color *tmp = colors;

  while (tmp) {
    struct lstopo_color *next = tmp->next;

    if (loutput->methods->destroy_color)
      loutput->methods->destroy_color(loutput, tmp);
    if (tmp->free)
      free(tmp);
    tmp = next;
  }

  colors = NULL; /* so that it works after refresh */
}

static struct lstopo_color *
find_or_declare_rgb_color(struct lstopo_output *loutput, int r, int g, int b)
{
  struct lstopo_color *color, *tmp;

  for(tmp = colors; tmp; tmp = tmp->next)
    if (tmp->r == r && tmp->g == g && tmp->b == b)
      return tmp;

  color = malloc(sizeof(*color));
  if (!color)
    return NULL;

  color->r = r & 255;
  color->g = g & 255;
  color->b = b & 255;
  color->free = 1;
  tmp = declare_color(loutput, color);
  if (!tmp)
    free(color);
  return tmp;
}

static unsigned
get_textwidth(void *output,
	      const char *text, unsigned length,
	      unsigned fontsize)
{
  struct lstopo_output *loutput = output;
  unsigned width;

#ifdef HWLOC_DEBUG
  assert(loutput->methods->textsize);
#endif
  loutput->methods->textsize(output, text, length, fontsize, &width);
  width = loutput->text_xscale * ((float)width);
  return width;
}

/*
 * foo_draw functions take a OBJ, computes which size it needs, recurse into
 * sublevels with drawing=PREPARE to recursively compute the needed size
 * without actually drawing anything, then draw things about OBJ (chip draw,
 * cache size information etc) at (X,Y), recurse into sublevels again to
 * actually draw things, and return in RETWIDTH and RETHEIGHT the amount of
 * space that the drawing took.
 *
 * For generic detailed comments, see the node_draw function.
 *
 * border is added around the objects
 * separator is added between objects
 */

typedef void (*foo_draw)(struct lstopo_output *loutput, hwloc_obj_t obj, unsigned depth, unsigned x, unsigned y);

static foo_draw get_type_fun(hwloc_obj_type_t type);

/* next child, in all children list, with memory before CPU, ignoring PU if needed.
 * similar to hwloc_get_next_child() but returns memory children first.
 */
#define NEXT_CHILD_INIT_STATE -1
static hwloc_obj_t next_child(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned kind, hwloc_obj_t prev, int *statep)
{
  int state;
  hwloc_obj_t obj;
  if (prev) {
    obj = prev->next_sibling;
    state = *statep;
  } else {
    obj = NULL;
    state = NEXT_CHILD_INIT_STATE;
  }

 again:
  if (!obj && state <= -1 && (kind & LSTOPO_CHILD_KIND_MEMORY)) {
    obj = parent->memory_first_child;
    state = 0;
  }
  if (!obj && state <= 0 && (kind & LSTOPO_CHILD_KIND_NORMAL)) {
    obj = parent->first_child;
    state = 1;
  }
  if (!obj && state <= 1 && (kind & LSTOPO_CHILD_KIND_IO)) {
    obj = parent->io_first_child;
    state = 2;
  }
  if (!obj && state <= 2 && (kind & LSTOPO_CHILD_KIND_MISC)) {
    obj = parent->misc_first_child;
    state = 3;
  }
  if (!obj)
    return NULL;

  if (loutput->factorize_enabled && parent->arity > loutput->factorize_min[obj->type]) {
    if (((struct lstopo_obj_userdata *)obj->userdata)->factorized < 0) {
      obj = obj->next_sibling;
      goto again;
    }
  }

  if (obj->type == HWLOC_OBJ_PU && loutput->ignore_pus) {
    obj = obj->next_sibling;
    goto again;
  }
  if (obj->type == HWLOC_OBJ_NUMANODE && loutput->ignore_numanodes) {
    obj = obj->next_sibling;
    goto again;
  }
  if (loutput->pci_collapse_enabled && obj->type == HWLOC_OBJ_PCI_DEVICE) {
    struct lstopo_obj_userdata *lud = obj->userdata;
    if (lud->pci_collapsed == -1) {
      obj = obj->next_sibling;
      goto again;
    }
  }
  *statep = state;
  return obj;
}

static float pci_link_speed(hwloc_obj_t obj)
{
  if (obj->type == HWLOC_OBJ_PCI_DEVICE)
    return obj->attr->pcidev.linkspeed;
  if (obj->type == HWLOC_OBJ_BRIDGE && obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)
    return obj->attr->bridge.upstream.pci.linkspeed;
  return 0.;
}

/********************************
 * Placing children in rectangle
 */

/* preferred width/height compromise */
#define RATIO (4.f/3.f)

/* returns a score <= 1. close to 1 is better */
static __hwloc_inline
float rectangle_score(unsigned width, unsigned height, float ratio)
{
  float score = ((float) width)/height/ratio;
  if (score > 1)
    score = 1/score;
  return score;
}

static void find_children_rectangle(struct lstopo_output *loutput, hwloc_obj_t parent,
				    unsigned kind, unsigned separator,
				    unsigned *rowsp, unsigned *columnsp,
				    float ratio)
{
  unsigned rows = 0, columns = 0, n;
  unsigned numsubobjs = 0, obj_totwidth = 0, obj_totheight = 0;
  unsigned obj_avgwidth, obj_avgheight;
  unsigned area = 0;
  float idealtotheight, under_score, over_score, best_score = 0.f;
  hwloc_obj_t child;
  int ncstate;

  /* Total area for subobjects */
  child = NULL;
  while ((child=next_child(loutput, parent, kind, child, &ncstate)) != NULL) {
    struct lstopo_obj_userdata *clud = child->userdata;
    numsubobjs++;
    obj_totwidth += clud->width + separator;
    obj_totheight += clud->height + separator;
    area += (clud->width + separator) * (clud->height + separator);
  }

  /* Average object size */
  obj_avgwidth = obj_totwidth / numsubobjs;
  obj_avgheight = obj_totheight / numsubobjs;

  /* Try to find a rectangle fitting exactly */
  for (n = (unsigned) (float) floor(sqrt(numsubobjs));
       rows >= (unsigned) (float) ceil(pow(numsubobjs, 0.33)) && n > 1;
       n--) {
    float old_best_score = best_score;
    unsigned p = numsubobjs / n;
    float np_score; /* n rows x p columns */
    float pn_score; /* p rows x n columns */
    if (p <= 1 || p * n != numsubobjs)
      continue;
    /* Try both n*p and n*p rectangles */
    np_score = rectangle_score(p * obj_avgwidth, obj_avgheight * n, ratio);
    pn_score = rectangle_score(n * obj_avgwidth, obj_avgheight * p, ratio);
    if (np_score > pn_score) {
      if (np_score > best_score) {
	rows = n;
	columns = p;
	best_score = np_score;
      }
    } else {
      if (pn_score > best_score) {
	rows = p;
	columns = n;
	best_score = pn_score;
      }
    }
    if (old_best_score == best_score)
      /* Score didn't improve, no need to try further.
       * The graph score(n) is an inverted U, with np_score on the left and pn_score on the right.
       * Se start somewhere in the middle and walk to both ends simultaneously,
       * Only one side can improve towards the maximum, we stop once once it stops improving.
       * The other side can just decrease for ever.
       */
      goto done;
  }
  if (best_score)
    /* We got a rectangle fitting exactly, it may not be perfect but it's good looking */
    goto done;

  /* Try to find a rectangle with an incomplete last row */

  /* Ideal total height for spreading that area with ratio */
  idealtotheight = (float) sqrt(area/ratio);
  /* approximation of number of rows */
  rows = (unsigned) (idealtotheight / obj_avgheight);
  columns = rows ? (numsubobjs + rows - 1) / rows : 1;
  /* Score obtained by underestimation */
  under_score = rectangle_score(columns * obj_avgwidth, rows * obj_avgheight, ratio);
  /* try to overestimate too */
  rows++;
  columns = (numsubobjs + rows - 1) / rows;
  /* Score obtained by overestimation */
  over_score = rectangle_score(columns * obj_avgwidth, rows * obj_avgheight, ratio);
  /* Revert back to under estimation if it was better */
  if (rows > 1 && under_score > over_score) {
    rows--;
    columns = (numsubobjs + rows - 1) / rows;
  }

 done:
  *rowsp = rows;
  *columnsp = columns;
}

/**************************
 * Placing children
 */

static void
place_children_horiz(struct lstopo_output *loutput, hwloc_obj_t parent,
		     unsigned kind, unsigned border, unsigned separator,
		     unsigned *width, unsigned *height)
{
  unsigned curx = 0;
  unsigned maxh = 0;
  hwloc_obj_t child;
  int ncstate;
  for(child = next_child(loutput, parent, kind, NULL, &ncstate);
      child;
      child = next_child(loutput, parent, kind, child, &ncstate)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    clud->xrel = curx + border;
    clud->yrel = border;
    if (clud->height > maxh)
      maxh = clud->height;
    curx += separator + clud->width;
  }
  *width = curx - separator + 2*border;
  *height = maxh + 2*border;
}

/* bridge object height: small empty box */
#define BRIDGE_HEIGHT (gridsize)

static void
place_children_vert(struct lstopo_output *loutput, hwloc_obj_t parent,
		    unsigned kind, unsigned border, unsigned separator,
		    unsigned *width, unsigned *height)
{
  unsigned cury = 0;
  unsigned maxw = 0;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  int bridge_parent_with_pcilinkspeed = parent->type == HWLOC_OBJ_BRIDGE
    && loutput->show_text_enabled && loutput->show_text[HWLOC_OBJ_BRIDGE];
  hwloc_obj_t child;
  int ncstate;
  for(child = next_child(loutput, parent, kind, NULL, &ncstate);
      child;
      child = next_child(loutput, parent, kind, child, &ncstate)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    unsigned child_height = clud->height;
    clud->xrel = border;
    clud->yrel = cury + border;
    if (clud->width > maxw)
      maxw = clud->width;

    if (bridge_parent_with_pcilinkspeed && pci_link_speed(child) != 0.)
      /* make sure padding between children is enough to display pci link speed */
      if (child_height <= BRIDGE_HEIGHT + fontsize)
	child_height = BRIDGE_HEIGHT + fontsize;

    cury += separator + child_height;
  }
  *width = maxw + 2*border;
  *height = cury - separator + 2*border;
}

static void
place_children_rect(struct lstopo_output *loutput, hwloc_obj_t parent,
		    unsigned kind, unsigned border, unsigned separator,
		    unsigned *width, unsigned *height)
{
  unsigned rows, columns;
  unsigned totwidth, totheight; /* total children array size, without borders */
  unsigned rowwidth; /* current row width */
  unsigned maxheight; /* max height for current row */
  hwloc_obj_t child;
  int ncstate;
  float ratio;
  int i;

  if (parent->type == HWLOC_OBJ_CORE)
    ratio = 1/RATIO;
  else
    ratio = RATIO;
  find_children_rectangle(loutput, parent, kind, separator, &rows, &columns, ratio);

  rowwidth = 0;
  maxheight = 0;
  totwidth = 0;
  totheight = 0;
  for(i = 0, child = next_child(loutput, parent, kind, NULL, &ncstate);
      child;
      i++, child = next_child(loutput, parent, kind, child, &ncstate)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    /* Newline? */
    if (i && i%columns == 0) {
      /* Update total width using new row */
      if (rowwidth > totwidth)
	totwidth = rowwidth;
      rowwidth = 0;
      /* Update total height */
      totheight += maxheight + separator;
      maxheight = 0;
    }
    /* Add new child */
    clud->xrel = rowwidth + border;
    clud->yrel = totheight + border;
    rowwidth += clud->width + separator;
    if (clud->height > maxheight)
      maxheight = clud->height;
  }
  /* Update total width using last row */
  if (rowwidth > totwidth)
    totwidth = rowwidth;
  /* Remove spurious separator on the right */
  totwidth -= separator;
  /* Update total height using last row */
  totheight += maxheight; /* no separator */

  *width = totwidth + 2*border;
  *height = totheight + 2*border;
}

static void
place__children(struct lstopo_output *loutput, hwloc_obj_t parent,
		unsigned kind,
		enum lstopo_orient_e *orientp,
		unsigned border, unsigned separator,
		unsigned *widthp, unsigned *heightp)
{
  if (*orientp == LSTOPO_ORIENT_HORIZ) {
    /* force horizontal */
    place_children_horiz(loutput, parent, kind, border, separator, widthp, heightp);

  } else if (*orientp == LSTOPO_ORIENT_VERT) {
    /* force vertical */
    place_children_vert(loutput, parent, kind, border, separator, widthp, heightp);

  } else {
    /* NONE or forced RECT, do a rectangular placement */
    place_children_rect(loutput, parent, kind, border, separator, widthp, heightp);
  }
}

/* Recurse into children to get their size.
 * Place them.
 * Save their position and the parent total size for later.
 */
static void
place_children(struct lstopo_output *loutput, hwloc_obj_t parent,
	       unsigned xrel, unsigned yrel /* position of children within parent */)
{
  struct lstopo_obj_userdata *plud = parent->userdata;
  enum lstopo_orient_e orient = loutput->force_orient[parent->type];
  unsigned border = loutput->gridsize;
  unsigned separator = loutput->gridsize;
  unsigned separator_below_cache = loutput->gridsize;
  unsigned normal_children_separator = loutput->gridsize;
  unsigned totwidth = plud->width, totheight = plud->height;
  unsigned children_width = 0, children_height = 0;
  unsigned above_children_width, above_children_height;
  unsigned existing_kinds;
  int normal_children_are_PUs;
  hwloc_obj_t child;
  int ncstate;
  unsigned i;

  /* defaults */
  plud->children.box = 0;
  plud->above_children.box = 0;

  /* list the kinds of children that exist in that parent */
  existing_kinds = (parent->arity ? LSTOPO_CHILD_KIND_NORMAL : 0)
    | (parent->memory_arity ? LSTOPO_CHILD_KIND_MEMORY : 0)
    | (parent->io_arity ? LSTOPO_CHILD_KIND_IO : 0)
    | (parent->misc_arity ? LSTOPO_CHILD_KIND_MISC : 0);
  /* now assign them below or above the parent */
  if (loutput->plain_children_order || hwloc_obj_type_is_memory(parent->type)) {
    plud->children.kinds = existing_kinds;
    plud->above_children.kinds = 0;
  } else {
    plud->children.kinds = existing_kinds & ~LSTOPO_CHILD_KIND_MEMORY;
    plud->above_children.kinds = existing_kinds & LSTOPO_CHILD_KIND_MEMORY;
  }

  /* bridge children always vertical */
  if (parent->type == HWLOC_OBJ_BRIDGE)
    orient = LSTOPO_ORIENT_VERT;

  /* if factorizing children, use horizontal by default */
  if (orient == LSTOPO_ORIENT_NONE
      && parent->symmetric_subtree
      && parent->first_child
      && loutput->factorize_enabled
      && parent->arity > loutput->factorize_min[parent->first_child->type]) {
    orient = LSTOPO_ORIENT_HORIZ;
  }

  /* if there are memory children and using plain children layout, use horizontal by default */
  if (orient == LSTOPO_ORIENT_NONE
      && parent->memory_arity
      && loutput->plain_children_order)
    orient = LSTOPO_ORIENT_HORIZ;

  /* recurse into children to prepare their sizes,
   * and check whether all normal children are PUs. */
  normal_children_are_PUs = (parent->arity > 0);
  for(i = 0, child = next_child(loutput, parent, LSTOPO_CHILD_KIND_ALL, NULL, &ncstate);
      child;
      i++, child = next_child(loutput, parent, LSTOPO_CHILD_KIND_ALL, child, &ncstate)) {
    get_type_fun(child->type)(loutput, child, 0, 0, 0);
    if (hwloc_obj_type_is_normal(child->type) && child->type != HWLOC_OBJ_PU)
      normal_children_are_PUs = 0;
  }
  if (!i)
    return;


  /* no separator between PUs */
  if (normal_children_are_PUs)
    normal_children_separator = 0;

  /* add separator between a cache parent and its children */
  if (hwloc_obj_type_is_cache(parent->type) || parent->type == HWLOC_OBJ_MEMCACHE) {
    if (normal_children_are_PUs || parent->type == HWLOC_OBJ_MEMCACHE)
      /* except between cache parent and PU children */
      separator_below_cache = 0;
    /* update children placement */
    yrel += separator_below_cache;
  }

  /* place children below the parent */
  if (plud->children.kinds)
    place__children(loutput, parent, plud->children.kinds, &orient, 0, normal_children_separator, &children_width, &children_height);

  /* place children above the parent, if any*/
  if (plud->above_children.kinds) {
    enum lstopo_orient_e morient = LSTOPO_ORIENT_HORIZ;
    int need_box;

    assert(plud->above_children.kinds == LSTOPO_CHILD_KIND_MEMORY);

    /* we need a memory children box if parent isn't a memory object
     * and if there are multiple objects in the box
     */
    need_box = !hwloc_obj_type_is_memory(parent->type)
      && (parent->memory_arity + parent->memory_first_child->memory_arity > 1);

    place__children(loutput, parent, plud->above_children.kinds, &morient, need_box ? border : 0, separator, &above_children_width, &above_children_height);
    if (parent->type == HWLOC_OBJ_MEMCACHE)
      above_children_height -= separator;

    if (need_box) {
      /* if there are multiple memory children, add a box, as large as the parent */
      if (above_children_width < children_width) {
	above_children_width = children_width;
      }
      plud->above_children.boxcolor = &MEMORIES_COLOR;
      plud->above_children.box = 1;

    } else {
      /* if there's a single memory child without wide memory box, enlarge that child */
      struct lstopo_obj_userdata *clud = parent->memory_first_child->userdata;
      if (clud->width < children_width) {
	clud->width = children_width;
	above_children_width = children_width;
      }
    }
  }

  /* adjust parent size */
  if (hwloc_obj_type_is_cache(parent->type) || parent->type == HWLOC_OBJ_MEMCACHE) {
    /* cache children are below */
    if (children_width > totwidth)
      totwidth = children_width;
    if (children_height)
      totheight += children_height + separator_below_cache;
    if (plud->above_children.kinds) {
      totheight += above_children_height + separator;
      if (above_children_width > totwidth)
	totwidth = above_children_width;
    }
  } else if (parent->type == HWLOC_OBJ_BRIDGE) {
    /* bridge children are on the right, within any space between bridge and children */
    if (children_width)
      totwidth += children_width;
    if (children_height > totheight)
      totheight = children_height;
  } else {
    /* normal objects have children inside their box, with space around them */
    if (children_width + 2*border > totwidth)
      totwidth = children_width + 2*border;
    if (children_height)
      totheight += children_height + border;
    if (plud->above_children.kinds) {
      totheight += above_children_height + separator;
      if (above_children_width + 2*border > totwidth)
	totwidth = above_children_width + 2*border;
    }
  }

  /* save config for draw_children() later */
  plud->orient = orient;
  plud->width = totwidth;
  plud->height = totheight;
  plud->children.width = children_width;
  plud->children.height = children_height;
  plud->children.xrel = xrel;
  plud->children.yrel = yrel;
  if (plud->above_children.kinds) {
    plud->above_children.width = above_children_width;
    plud->above_children.height = above_children_height;
    plud->above_children.xrel = xrel;
    plud->above_children.yrel = yrel;
    plud->children.yrel += above_children_height + separator;
  }
}

/***********************
 * Drawing children
 */

static void
draw__children(struct lstopo_output *loutput, hwloc_obj_t parent,
	       struct lstopo_children_position *children,
	       unsigned depth,
	       unsigned x, unsigned y)
{
  hwloc_obj_t child;
  int ncstate;

  if (children->box)
    loutput->methods->box(loutput, children->boxcolor, depth, x, children->width, y, children->height, parent, 1);

  for(child = next_child(loutput, parent, children->kinds, NULL, &ncstate);
      child;
      child = next_child(loutput, parent, children->kinds, child, &ncstate)) {
      struct lstopo_obj_userdata *clud = child->userdata;
      get_type_fun(child->type)(loutput, child, depth-1, x + clud->xrel, y + clud->yrel);
    }
}

static void
draw_children(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned depth,
	      unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *plud = parent->userdata;

  if (plud->children.kinds)
    draw__children(loutput, parent, &plud->children, depth, x + plud->children.xrel, y + plud->children.yrel);

  if (plud->above_children.kinds)
    draw__children(loutput, parent, &plud->above_children, depth, x + plud->above_children.xrel, y + plud->above_children.yrel);
}

/*******
 * Misc
 */

static int
lstopo_obj_snprintf(struct lstopo_output *loutput, char *text, size_t textlen, hwloc_obj_t obj)
{
  enum lstopo_index_type_e index_type = loutput->index_type;
  unsigned idx;
  const char *indexprefix;
  char typestr[32];
  char indexstr[32]= "";
  char index2str[32] = "";
  char attrstr[256];
  char totmemstr[64] = "";
  int attrlen;

  /* For Misc and Group, name replaces type+index+attrs */
  if (obj->name && (obj->type == HWLOC_OBJ_MISC || obj->type == HWLOC_OBJ_GROUP)) {
    return snprintf(text, textlen, "%s", obj->name);
  }
  /* For OSDev, OSDev-type+name replaces type+index+attrs */
  if (obj->type == HWLOC_OBJ_OS_DEVICE) {
    /* consider the name as an index and remove it if LSTOPO_INDEX_TYPE_NONE */
    if (index_type != LSTOPO_INDEX_TYPE_NONE) {
      hwloc_obj_type_snprintf(typestr, sizeof(typestr), obj, 0);
      return snprintf(text, textlen, "%s %s", typestr, obj->name);
    } else {
      return hwloc_obj_type_snprintf(text, textlen, obj, 0);
    }
  }

  /* subtype replaces the basic type name */
  if (obj->subtype) {
    snprintf(typestr, sizeof(typestr), "%s", obj->subtype);
  } else {
    hwloc_obj_type_snprintf(typestr, sizeof(typestr), obj, 0);
  }

  if (index_type == LSTOPO_INDEX_TYPE_DEFAULT) {
    if (obj->type == HWLOC_OBJ_PU || obj->type == HWLOC_OBJ_NUMANODE) {
      /* by default we show logical+physical for PU/NUMA */
      idx = obj->logical_index;
      indexprefix = " L#";
    } else if (obj->type == HWLOC_OBJ_PACKAGE || obj->type == HWLOC_OBJ_DIE || obj->type == HWLOC_OBJ_CORE) {
      /* logical only for package+core (so that we see easily how many packages/cores there are */
      idx = obj->logical_index;
      indexprefix = " L#";
    } else {
      /* nothing for others */
      idx = HWLOC_UNKNOWN_INDEX;
      indexprefix = "";
    }
  } else if (index_type == LSTOPO_INDEX_TYPE_LOGICAL) {
    idx = obj->logical_index;
    indexprefix = " L#";
  } else if (index_type == LSTOPO_INDEX_TYPE_PHYSICAL) {
    idx = obj->os_index;
    indexprefix = " P#";
  } else {
    /* shutup the compiler */
    idx = 0;
    indexprefix = "";
  }

  if (loutput->show_indexes[obj->type]
      && index_type != LSTOPO_INDEX_TYPE_NONE
      && idx != (unsigned)-1 && obj->depth != 0
      && obj->type != HWLOC_OBJ_PCI_DEVICE
      && (obj->type != HWLOC_OBJ_BRIDGE || obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_HOST))
    snprintf(indexstr, sizeof(indexstr), "%s%u", indexprefix, idx);

  if (index_type == LSTOPO_INDEX_TYPE_DEFAULT && obj->type == HWLOC_OBJ_NUMANODE && loutput->show_indexes[obj->type])
    /* NUMA have both P# and L# on the same line (PU is split on 2 lines) */
    snprintf(index2str, sizeof(index2str), " P#%u", obj->os_index);

  if (loutput->show_attrs_enabled && loutput->show_attrs[obj->type]) {
    attrlen = hwloc_obj_attr_snprintf(attrstr, sizeof(attrstr), obj, " ", 0);
    /* display the root total_memory (cannot be local_memory since root cannot be a NUMA node) */
    if (!obj->parent && obj->total_memory)
      snprintf(totmemstr, sizeof(totmemstr), " (%lu%s total)",
	       (unsigned long) hwloc_memory_size_printf_value(obj->total_memory, 0),
	       hwloc_memory_size_printf_unit(obj->total_memory, 0));
  } else
    attrlen = 0;

  if (attrlen > 0)
    return snprintf(text, textlen, "%s%s%s (%s)%s", typestr, indexstr, index2str, attrstr, totmemstr);
  else
    return snprintf(text, textlen, "%s%s%s%s", typestr, indexstr, index2str, totmemstr);
}

static void
lstopo__prepare_custom_styles(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  struct lstopo_obj_userdata *lud = obj->userdata;
  struct lstopo_style *s = &lud->style;
  hwloc_obj_t child;
  unsigned forcer, forceg, forceb;
  const char *stylestr;
  struct lstopo_color *lcolor;

  lud->style_set = 0;

  stylestr = hwloc_obj_get_info_by_name(obj, "lstopoStyle");
  if (stylestr) {
    while (*stylestr != '\0') {
      if (sscanf(stylestr, "%02x%02x%02x", &forcer, &forceg, &forceb) == 3
	  || sscanf(stylestr, "Background=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	lcolor = find_or_declare_rgb_color(loutput, forcer, forceg, forceb);
	if (lcolor) {
	  s->bg = lcolor;
	  lud->style_set |= LSTOPO_STYLE_BG;
	  if (!(lud->style_set & LSTOPO_STYLE_T)) {
	    s->t = (lcolor->r + lcolor->g + lcolor->b < 0xff) ? &WHITE_COLOR : &BLACK_COLOR;
	    lud->style_set |= LSTOPO_STYLE_T;
	  }
	}
      } else if (sscanf(stylestr, "Text=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	lcolor = find_or_declare_rgb_color(loutput, forcer, forceg, forceb);
	if (lcolor) {
	  s->t = lcolor;
	  lud->style_set |= LSTOPO_STYLE_T;
	}
      } else if (sscanf(stylestr, "Text2=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	lcolor = find_or_declare_rgb_color(loutput, forcer, forceg, forceb);
	if (lcolor) {
	  s->t2 = lcolor;
	  lud->style_set |= LSTOPO_STYLE_T2;
	}
      }
      stylestr = strchr(stylestr, ';');
      if (!stylestr)
	break;
      stylestr++;
    }
  }

  for_each_child(child, obj)
    lstopo__prepare_custom_styles(loutput, child);
  for_each_memory_child(child, obj)
    lstopo__prepare_custom_styles(loutput, child);
  for_each_io_child(child, obj)
    lstopo__prepare_custom_styles(loutput, child);
  for_each_misc_child(child, obj)
    lstopo__prepare_custom_styles(loutput, child);
}

void
lstopo_prepare_custom_styles(struct lstopo_output *loutput)
{
  lstopo__prepare_custom_styles(loutput, hwloc_get_root_obj(loutput->topology));
}

static void
lstopo_set_object_color(struct lstopo_output *loutput,
			hwloc_obj_t obj,
			struct lstopo_style *s)
{
  struct lstopo_obj_userdata *lud = obj->userdata;

  s->bg = &BLACK_COLOR;
  s->t = &BLACK_COLOR;
  s->t2 = &BLACK_COLOR;

  switch (obj->type) {

  case HWLOC_OBJ_MACHINE:
    s->bg = &MACHINE_COLOR;
    break;

  case HWLOC_OBJ_GROUP: {
    hwloc_obj_t parent;
    s->bg = &MISC_COLOR;
    parent = obj->parent;
    while (parent) {
      if (parent->type == HWLOC_OBJ_PACKAGE) {
	s->bg = &GROUP_IN_PACKAGE_COLOR;
	break;
      }
      parent = parent->parent;
    }
    break;
  }

  case HWLOC_OBJ_MISC:
    s->bg = &MISC_COLOR;
    break;

  case HWLOC_OBJ_NUMANODE:
    if (loutput->show_disallowed && lstopo_numa_disallowed(loutput, obj)) {
      s->bg = &DISALLOWED_COLOR;
    } else if (loutput->show_binding && lstopo_numa_binding(loutput, obj)) {
      s->bg = &BINDING_COLOR;
    } else {
      s->bg = &MEMORY_COLOR;
    }
    break;

  case HWLOC_OBJ_PACKAGE:
    s->bg = &PACKAGE_COLOR;
    break;

  case HWLOC_OBJ_DIE:
    s->bg = &DIE_COLOR;
    break;

  case HWLOC_OBJ_CORE:
    s->bg = &CORE_COLOR;
    break;

  case HWLOC_OBJ_L1CACHE:
  case HWLOC_OBJ_L2CACHE:
  case HWLOC_OBJ_L3CACHE:
  case HWLOC_OBJ_L4CACHE:
  case HWLOC_OBJ_L5CACHE:
  case HWLOC_OBJ_L1ICACHE:
  case HWLOC_OBJ_L2ICACHE:
  case HWLOC_OBJ_L3ICACHE:
  case HWLOC_OBJ_MEMCACHE:
    s->bg = &CACHE_COLOR;
    break;

  case HWLOC_OBJ_PU:
    if (loutput->show_disallowed && lstopo_pu_disallowed(loutput, obj)) {
      s->bg = &DISALLOWED_COLOR;
    } else if (loutput->show_binding && lstopo_pu_binding(loutput, obj)) {
      s->bg = &BINDING_COLOR;
    } else {
      s->bg = &THREAD_COLOR;
    }
    break;

  case HWLOC_OBJ_BRIDGE:
    s->bg = &BRIDGE_COLOR;
    break;

  case HWLOC_OBJ_PCI_DEVICE:
    s->bg = &PCI_DEVICE_COLOR;
    break;

  case HWLOC_OBJ_OS_DEVICE:
    s->bg = &OS_DEVICE_COLOR;
    break;

  default:
    assert(0);
  }

  if (lud->style_set & LSTOPO_STYLE_BG)
    s->bg = lud->style.bg;
  if (lud->style_set & LSTOPO_STYLE_T)
    s->t = lud->style.t;
  if (lud->style_set & LSTOPO_STYLE_T2)
    s->t2 = lud->style.t2;
}

static void
prepare_text(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  struct lstopo_obj_userdata *lud = obj->userdata;
  unsigned fontsize = loutput->fontsize;
  unsigned i;
  int n;

  /* sane defaults */
  lud->ntext = 0;
  lud->textwidth = 0;

  if (!loutput->show_text_enabled || !loutput->show_text[obj->type])
    return;

  /* main object identifier line */
  if (obj->type == HWLOC_OBJ_PCI_DEVICE && loutput->show_attrs_enabled && loutput->show_attrs[HWLOC_OBJ_PCI_DEVICE]) {
    /* PCI text collapsing */
    char busid[32];
    char _text[64];
    lstopo_obj_snprintf(loutput, _text, sizeof(_text), obj);
    lstopo_busid_snprintf(loutput, busid, sizeof(busid), obj, lud->pci_collapsed, loutput->need_pci_domain);
    if (loutput->pci_collapse_enabled && lud->pci_collapsed > 1) {
      n = snprintf(lud->text[0].text, sizeof(lud->text[0].text), "%d x { %s %s }", lud->pci_collapsed, _text, busid);
    } else {
      n = snprintf(lud->text[0].text, sizeof(lud->text[0].text), "%s %s", _text, busid);
    }
  } else {
    /* normal object text */
    n = lstopo_obj_snprintf(loutput, lud->text[0].text, sizeof(lud->text[0].text), obj);
  }
  lud->ntext = 1;

  /* additional lines of text */

  if (HWLOC_OBJ_PU == obj->type && loutput->index_type == LSTOPO_INDEX_TYPE_DEFAULT && loutput->show_indexes[obj->type]) {
    /* PU P# is on second line */
    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text), "P#%u", obj->os_index);
  }

  if (loutput->show_attrs_enabled && loutput->show_attrs[obj->type]) {
    if (HWLOC_OBJ_OS_DEVICE == obj->type) {
      if (HWLOC_OBJ_OSDEV_COPROC == obj->attr->osdev.type && obj->subtype) {
	/* Coprocessor */
	if (!strcmp(obj->subtype, "CUDA")) {
	  /* CUDA */
	  const char *value, *value2, *value3;
	  value = hwloc_obj_get_info_by_name(obj, "CUDAGlobalMemorySize");
	  if (value) {
	    unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		     mb >= 10240 ? "%llu GB" : "%llu MB",
		     mb >= 10240 ? mb/1024 : mb);
	  }
	  value = hwloc_obj_get_info_by_name(obj, "CUDAL2CacheSize");
	  if (value) {
	    unsigned long long kb = strtoull(value, NULL, 10);
	    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		     kb >= 10240 ? "L2 (%llu MB)" : "L2 (%llu kB)",
		     kb >= 10240 ? kb/1024 : kb);
	  }
	  value = hwloc_obj_get_info_by_name(obj, "CUDAMultiProcessors");
	  value2 = hwloc_obj_get_info_by_name(obj, "CUDACoresPerMP");
	  value3 = hwloc_obj_get_info_by_name(obj, "CUDASharedMemorySizePerMP");
	  if (value && value2 && value3) {
	    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		     "%s MP x (%s cores + %s kB)", value, value2, value3);
	  }

	} else if (!strcmp(obj->subtype, "OpenCL")) {
	  /* OpenCL */
	  const char *value;
	  value = hwloc_obj_get_info_by_name(obj, "OpenCLComputeUnits");
	  if (value) {
	    unsigned long long cu = strtoull(value, NULL, 10);
	    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		     "%llu compute units", cu);
	  }
	  value = hwloc_obj_get_info_by_name(obj, "OpenCLGlobalMemorySize");
	  if (value) {
	    unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	    snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		     mb >= 10240 ? "%llu GB" : "%llu MB",
		     mb >= 10240 ? mb/1024 : mb);
	  }
	} else if (!strcmp(obj->subtype, "VectorEngine")) {
          /* NEC Vector Engine */
          const char *value;
          value = hwloc_obj_get_info_by_name(obj, "VectorEngineCores");
          if (value) {
            unsigned long long cu = strtoull(value, NULL, 10);
            snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
                     "%llu cores", cu);
          }
          value = hwloc_obj_get_info_by_name(obj, "VectorEngineMemorySize");
          if (value) {
            unsigned long long mb = strtoull(value, NULL, 10) / 1024;
            snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
                     mb >= 10240 ? "%llu GB" : "%llu MB",
                     mb >= 10240 ? mb/1024 : mb);
          }
        }

      } else if (HWLOC_OBJ_OSDEV_BLOCK == obj->attr->osdev.type) {
	/* Block */
	const char *value;
	value = hwloc_obj_get_info_by_name(obj, "Size");
	if (value) {
	  unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	  snprintf(lud->text[lud->ntext++].text, sizeof(lud->text[0].text),
		   mb >= 10485760 ? "%llu TB" : mb >= 10240 ? "%llu GB" : "%llu MB",
		   mb >= 10485760 ? mb/1048576 : mb >= 10240 ? mb/1024 : mb);
	}
      }
    }
  }

  lud->textwidth = 0;
  for(i=0; i<lud->ntext; i++) {
    unsigned textwidth;
    if (i) /* already computed above for n=0 */
      n = (unsigned)strlen(lud->text[i].text);
    textwidth = get_textwidth(loutput, lud->text[i].text, n, fontsize);
    lud->text[i].width = textwidth;
    lud->text[i].xoffset = 0; /* only used for PU in output_compute_pu_min_textwidth() */
    if (textwidth > lud->textwidth)
      lud->textwidth = textwidth;
  }
}

static void
draw_text(struct lstopo_output *loutput, hwloc_obj_t obj, struct lstopo_color *lcolor, unsigned depth, unsigned x, unsigned y)
{
  struct draw_methods *methods = loutput->methods;
  struct lstopo_obj_userdata *lud = obj->userdata;
  unsigned fontsize = loutput->fontsize;
  unsigned linespacing = loutput->linespacing;
  unsigned i;

  if (!loutput->show_text_enabled || !loutput->show_text[obj->type])
    return;

  for(i=0; i<lud->ntext; i++)
    methods->text(loutput, lcolor, fontsize, depth, x + lud->text[i].xoffset, y + i*(linespacing + fontsize), lud->text[i].text, obj, i);
}

static void
pci_device_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned overlaidoffset = 0;

  if (loutput->pci_collapse_enabled && lud->pci_collapsed > 1) {
    /* additional depths and height for overlaid boxes */
    depth -= 2;
    if (lud->pci_collapsed > 2) {
      overlaidoffset = gridsize;
    } else {
      overlaidoffset = gridsize/2;
    }
    if (loutput->no_half_lines)
      overlaidoffset *= 2;
  }

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    lud->width = gridsize + overlaidoffset;
    lud->height = gridsize + overlaidoffset;
    if (lud->ntext > 0) {
      lud->width += lud->textwidth + gridsize;
      lud->height += fontsize + gridsize;
    }
    place_children(loutput, level,
		   gridsize, lud->height);

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct lstopo_style style;
    unsigned totwidth, totheight;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);

    if (loutput->pci_collapse_enabled && lud->pci_collapsed > 1) {
      methods->box(loutput, style.bg, depth+2, x + overlaidoffset, totwidth - overlaidoffset, y + overlaidoffset, totheight - overlaidoffset, level, 2);
      if (lud->pci_collapsed > 2)
	methods->box(loutput, style.bg, depth+1, x + overlaidoffset/2, totwidth - overlaidoffset, y + overlaidoffset/2, totheight - overlaidoffset, level, 1);
      methods->box(loutput, style.bg, depth, x, totwidth - overlaidoffset, y, totheight - overlaidoffset, level, 0);
    } else {
      methods->box(loutput, style.bg, depth, x, totwidth, y, totheight, level, 0);
    }

    draw_text(loutput, level, style.t, depth-1, x + gridsize, y + gridsize);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

static void
bridge_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  /* align all children even if only some of them have a linkspeed */
  unsigned speedwidth = (loutput->show_text_enabled && loutput->show_text[HWLOC_OBJ_BRIDGE]) ? fontsize + gridsize : 0;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    lud->width = 2*gridsize + gridsize + speedwidth;
    lud->height = BRIDGE_HEIGHT;
    place_children(loutput, level,
		   3*gridsize + speedwidth, 0);

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct lstopo_style style;

    /* Square and left link */
    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg, depth, x, gridsize, y + BRIDGE_HEIGHT/2 - gridsize/2, gridsize, level, 0);
    methods->line(loutput, &BLACK_COLOR, depth, x + gridsize, y + BRIDGE_HEIGHT/2, x + 2*gridsize, y + BRIDGE_HEIGHT/2, level, 0);

    if (level->io_arity > 0) {
      hwloc_obj_t child = NULL;
      unsigned ymax = -1;
      unsigned ymin = (unsigned) -1;
      int ncstate;
      int i = 0;
      while ((child=next_child(loutput, level, LSTOPO_CHILD_KIND_ALL, child, &ncstate)) != NULL) {
	struct lstopo_obj_userdata *clud = child->userdata;
	unsigned ymid = y + clud->yrel + BRIDGE_HEIGHT/2;
	/* Line to PCI device */
	methods->line(loutput, &BLACK_COLOR, depth-1, x+2*gridsize, ymid, x+3*gridsize+speedwidth, ymid, level, i+2);
	if (ymin == (unsigned) -1)
	  ymin = ymid;
	ymax = ymid;
	/* Negotiated link speed */
	if (loutput->show_text_enabled && loutput->show_text[HWLOC_OBJ_BRIDGE]) {
	  float speed = pci_link_speed(child);
	  if (loutput->show_attrs_enabled && loutput->show_attrs[HWLOC_OBJ_BRIDGE] && speed != 0.) {
	    char text[4];
	    if (speed >= 10.)
	      snprintf(text, sizeof(text), "%.0f", child->attr->pcidev.linkspeed);
	    else
	      snprintf(text, sizeof(text), "%0.1f", child->attr->pcidev.linkspeed);
	    methods->text(loutput, style.t2, fontsize, depth-1, x + 2.5*gridsize, ymid + BRIDGE_HEIGHT/2, text, level, i+2);
	  }
	}
	i++;
      }
      methods->line(loutput, &BLACK_COLOR, depth-1, x+2*gridsize, ymin, x+2*gridsize, ymax, level, 1);

      /* Draw sublevels for real */
      draw_children(loutput, level, depth-1, x, y);
    }
  }
}

static void
factorized_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned linespacing = loutput->linespacing;
  unsigned missingseparator;

  /* we need a separator even between PUs */
  if ((unsigned)level->depth == loutput->depth-1)
    missingseparator = gridsize;
  else
    missingseparator = 0;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    unsigned n, textwidth;
    lud->width = gridsize*5; /* space, box, space, box, space */
    lud->height = gridsize*2 + linespacing + fontsize + gridsize; /* space, box, linespace, text, gridsize */
    sprintf(lud->text[0].text, "%ux total", level->parent->arity);
    n = (unsigned)strlen(lud->text[0].text);
    textwidth = get_textwidth(loutput, lud->text[0].text, n, fontsize);
    lud->text[0].width = textwidth;
    if (textwidth > lud->width) {
      lud->width = textwidth;
      lud->text[0].xoffset = 0;
    } else {
      lud->text[0].xoffset = (lud->width - textwidth)/2;
    }
    lud->ntext = 1;
    lud->width += 2*missingseparator;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct lstopo_style boxstyle, textstyle;
    unsigned boxoffset = (lud->width - 5*gridsize - 2*missingseparator) / 2;
    /* boxes use object style, but the text outside uses the parent style */
    lstopo_set_object_color(loutput, level, &boxstyle);
    lstopo_set_object_color(loutput, level->parent, &textstyle);
    methods->box(loutput, boxstyle.bg, depth, x + missingseparator + boxoffset, gridsize, y + gridsize, gridsize, level, 0);
    methods->box(loutput, boxstyle.bg, depth, x + missingseparator + boxoffset + 2*gridsize, gridsize, y + gridsize, gridsize, level, 0);
    methods->box(loutput, boxstyle.bg, depth, x + missingseparator + boxoffset + 4*gridsize, gridsize, y + gridsize, gridsize, level, 0);
    methods->text(loutput, textstyle.t, fontsize, depth, x + missingseparator + lud->text[0].xoffset, y + 2 * gridsize + linespacing, lud->text[0].text, level, 0);
  }
}

static void
cache_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;

  if (loutput->factorize_enabled
      && lud->factorized == 1
      && level->parent->arity > loutput->factorize_min[level->type]) {
    factorized_draw(loutput, level, depth, x, y);
    return;
  }

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    lud->width = gridsize;
    lud->height = gridsize;
    if (lud->ntext > 0) {
      lud->width += lud->textwidth + gridsize;
      lud->height += fontsize + gridsize;
    }
    place_children(loutput, level,
		   0, lud->height /* the callee with add vertical space if needed */);

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct lstopo_style style;
    unsigned totwidth;
    unsigned myoff = 0;
    unsigned myheight;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;

    /* totheight also contains children outside of this actual cache box,
     * recompute our height without outside children (just like above)
     */
    myheight = gridsize;
    if (lud->ntext > 0)
      myheight += fontsize + gridsize;

    if (lud->above_children.kinds) {
      /* display above_children even above the cache itself */
      myoff = lud->above_children.height + gridsize;
      lud->above_children.yrel = 0;
    }

    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg, depth, x, totwidth, y + myoff, myheight, level, 0);

    draw_text(loutput, level, style.t, depth-1, x + gridsize, y + gridsize + myoff);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

static void
normal_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned linespacing = loutput->linespacing;

  if (loutput->factorize_enabled
      && lud->factorized == 1
      && level->parent->arity > loutput->factorize_min[level->type]) {
    factorized_draw(loutput, level, depth, x, y);
    return;
  }

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    if (level->type != HWLOC_OBJ_PU) /* PU already computed in output_compute_pu_min_textwidth() earlier */
      prepare_text(loutput, level);
    lud->width = gridsize;
    lud->height = gridsize;
    if (lud->ntext > 0) {
      lud->width += lud->textwidth + gridsize;
      lud->height += fontsize + (fontsize + linespacing) * (lud->ntext - 1) + gridsize;
    }
    place_children(loutput, level,
		   gridsize, lud->height);

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct lstopo_style style;
    unsigned totwidth, totheight;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg, depth, x, totwidth, y, totheight, level, 0);
    draw_text(loutput, level, style.t, depth-1, x + gridsize, y + gridsize);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

static void
output_align_PU_textwidth(struct lstopo_output *loutput)
{
  hwloc_topology_t topology = loutput->topology;
  unsigned textwidth_max = 0;
  hwloc_obj_t pu;
  unsigned i;

  /* compute the max PU textwidth */
  pu = NULL;
  while ((pu = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu)) != NULL) {
    struct lstopo_obj_userdata *lud = pu->userdata;
    prepare_text(loutput, pu);
    if (lud->textwidth > textwidth_max)
      textwidth_max = lud->textwidth;
  }

  /* update text placement to match the max textwidth */
  pu = NULL;
  while ((pu = hwloc_get_next_obj_by_type(topology, HWLOC_OBJ_PU, pu)) != NULL) {
    struct lstopo_obj_userdata *lud = pu->userdata;
    for(i=0; i<lud->ntext; i++)
      if (lud->text[i].width < textwidth_max) {
	lud->text[i].xoffset = (textwidth_max-lud->text[i].width)/2;
	lud->text[i].width = textwidth_max;
      }
    lud->textwidth = textwidth_max;
  }
}

void
output_draw(struct lstopo_output *loutput)
{
  hwloc_topology_t topology = loutput->topology;
  struct draw_methods *methods = loutput->methods;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned linespacing = loutput->linespacing;
  hwloc_obj_t root = hwloc_get_root_obj(topology);
  struct lstopo_obj_userdata *rlud = root->userdata;
  unsigned depth = 100;
  unsigned totwidth, totheight, offset, i, j;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    unsigned maxtextwidth = 0, textwidth;
    unsigned ndl = 0;
    char hostname[122] = "";
    unsigned long hostname_size = sizeof(hostname);
    unsigned infocount = 0;

    /* prepare legend lines and compute the width */
    if (loutput->show_legend == LSTOPO_SHOW_LEGEND_ALL) {
      time_t t;
      const char *forcedhostname = NULL;

      /* build the default legend lines */
      forcedhostname = hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "HostName");
      if (!forcedhostname && hwloc_topology_is_thissystem(topology)) {
#if defined(HWLOC_WIN_SYS) && !defined(__CYGWIN__)
        GetComputerName(hostname, &hostname_size);
#else
        gethostname(hostname, hostname_size);
#endif
      }
      if (forcedhostname || *hostname) {
        snprintf(loutput->legend_default_lines[ndl],
                 sizeof(loutput->legend_default_lines[ndl]),
                 "Host: %s",
                 forcedhostname ? forcedhostname : hostname);
        textwidth = get_textwidth(loutput,
                                  loutput->legend_default_lines[ndl],
                                  (unsigned) strlen(loutput->legend_default_lines[ndl]),
                                  fontsize);
        if (textwidth > maxtextwidth)
          maxtextwidth = textwidth;
        ndl++;
      }

      /* Display whether we're showing physical or logical IDs */
      if (loutput->index_type != LSTOPO_INDEX_TYPE_DEFAULT) {
        snprintf(loutput->legend_default_lines[ndl],
                 sizeof(loutput->legend_default_lines[ndl]),
                 "Indexes: %s",
                 (loutput->index_type == LSTOPO_INDEX_TYPE_LOGICAL ? "logical" : "physical"));
        textwidth = get_textwidth(loutput,
                                  loutput->legend_default_lines[ndl],
                                  (unsigned) strlen(loutput->legend_default_lines[ndl]),
                                  fontsize);
        if (textwidth > maxtextwidth)
          maxtextwidth = textwidth;
        ndl++;
      }

      /* Display timestamp */
      t = time(NULL);
#ifdef HAVE_STRFTIME
      {
        struct tm *tmp;
        tmp = localtime(&t);
        strftime(loutput->legend_default_lines[ndl],
                 sizeof(loutput->legend_default_lines[ndl]),
                 "Date: %c",
                 tmp);
      }
#else /* HAVE_STRFTIME */
      {
        char *date;
        unsigned n;
        date = ctime(&t);
        n = (unsigned) strlen(date);
        if (n && date[n-1] == '\n') {
          date[n-1] = 0;
        }
        snprintf(loutput->legend_default_lines[ndl],
                 sizeof(loutput->legend_default_lines[ndl]),
                 "Date: %s",
                 date);
      }
#endif /* HAVE_STRFTIME */
      textwidth = get_textwidth(loutput,
                                loutput->legend_default_lines[ndl],
                                (unsigned) strlen(loutput->legend_default_lines[ndl]),
                                fontsize);
      if (textwidth > maxtextwidth)
        maxtextwidth = textwidth;
      ndl++;
    }

    if (loutput->show_legend != LSTOPO_SHOW_LEGEND_NONE) {
      /* look at custom legend lines in root info attr and --append-legend */
      for(i=0; i<root->infos_count; i++) {
        if (!strcmp(root->infos[i].name, "lstopoLegend")) {
          infocount++;
          textwidth = get_textwidth(loutput, root->infos[i].value, (unsigned) strlen(root->infos[i].value), fontsize);
          if (textwidth > maxtextwidth)
            maxtextwidth = textwidth;
        }
      }
      for(i=0; i<loutput->legend_append_nr; i++) {
        textwidth = get_textwidth(loutput, loutput->legend_append[i], (unsigned) strlen(loutput->legend_append[i]), fontsize);
        if (textwidth > maxtextwidth)
          maxtextwidth = textwidth;
      }
    }

    /* save legend info for later */
    loutput->legend_maxtextwidth = maxtextwidth;
    loutput->legend_default_lines_nr = ndl;
    loutput->legend_info_lines_nr = infocount;

    /* compute root size, our size, and save it */

    output_align_PU_textwidth(loutput);

    get_type_fun(root->type)(loutput, root, depth, 0, 0);

    /* loutput width is max(root, legend) */
    totwidth = rlud->width;
    if (loutput->legend_maxtextwidth + 2*gridsize > totwidth)
      totwidth = loutput->legend_maxtextwidth + 2*gridsize;
    loutput->width = totwidth;

    /* loutput height is sum(root, legend) */
    totheight = rlud->height;
    if (loutput->show_legend != LSTOPO_SHOW_LEGEND_NONE
        && (ndl + infocount + loutput->legend_append_nr))
      totheight += gridsize
        + (ndl + infocount + loutput->legend_append_nr - 1) * (linespacing + fontsize)
        + fontsize + gridsize;
    loutput->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    /* restore our size that was computed during prepare */
    totwidth = rlud->width;
    totheight = rlud->height;

    /* Draw root for real */
    get_type_fun(root->type)(loutput, root, depth, 0, 0);

    /* Draw legend */
    if (loutput->show_legend != LSTOPO_SHOW_LEGEND_NONE
        && (loutput->legend_default_lines_nr + loutput->legend_info_lines_nr + loutput->legend_append_nr)) {
      offset = rlud->height + gridsize;
      methods->box(loutput, &WHITE_COLOR, depth,
                   0,
                   loutput->width,
                   totheight,
                   gridsize
                   + (loutput->legend_default_lines_nr + loutput->legend_info_lines_nr + loutput->legend_append_nr - 1) * (linespacing + fontsize)
                   + fontsize + gridsize,
                   NULL, 0);
      for(i=0; i<loutput->legend_default_lines_nr; i++, offset += linespacing + fontsize)
	methods->text(loutput, &BLACK_COLOR, fontsize, depth, gridsize, offset, loutput->legend_default_lines[i], NULL, i);
      for(i=0, j=0; i<root->infos_count; i++) {
        if (!strcmp(root->infos[i].name, "lstopoLegend")) {
          methods->text(loutput, &BLACK_COLOR, fontsize, depth, gridsize, offset, root->infos[i].value, NULL, j+loutput->legend_default_lines_nr);
          j++;
          offset += linespacing + fontsize;
        }
      }
      for(i=0; i<loutput->legend_append_nr; i++, offset += linespacing + fontsize)
	methods->text(loutput, &BLACK_COLOR, fontsize, depth, gridsize, offset, loutput->legend_append[i], NULL, i+loutput->legend_default_lines_nr+loutput->legend_info_lines_nr);
    }
  }
}

/*
 * given a type, return a pointer FUN to the function that draws it.
 */
static foo_draw
get_type_fun(hwloc_obj_type_t type)
{
  switch (type) {
    case HWLOC_OBJ_MACHINE:
    case HWLOC_OBJ_NUMANODE:
    case HWLOC_OBJ_PACKAGE:
    case HWLOC_OBJ_DIE:
    case HWLOC_OBJ_CORE:
    case HWLOC_OBJ_PU:
    case HWLOC_OBJ_GROUP:
    case HWLOC_OBJ_OS_DEVICE:
    case HWLOC_OBJ_MISC: return normal_draw;
    case HWLOC_OBJ_L1CACHE: return cache_draw;
    case HWLOC_OBJ_L2CACHE: return cache_draw;
    case HWLOC_OBJ_L3CACHE: return cache_draw;
    case HWLOC_OBJ_L4CACHE: return cache_draw;
    case HWLOC_OBJ_L5CACHE: return cache_draw;
    case HWLOC_OBJ_L1ICACHE: return cache_draw;
    case HWLOC_OBJ_L2ICACHE: return cache_draw;
    case HWLOC_OBJ_L3ICACHE: return cache_draw;
    case HWLOC_OBJ_MEMCACHE: return cache_draw;
    case HWLOC_OBJ_PCI_DEVICE: return pci_device_draw;
    case HWLOC_OBJ_BRIDGE: return bridge_draw;
    default:
    case HWLOC_OBJ_TYPE_MAX: assert(0);
  }
  /* for dumb compilers */
  return normal_draw;
}
