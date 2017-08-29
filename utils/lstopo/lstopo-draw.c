/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2017 Inria.  All rights reserved.
 * Copyright © 2009-2013, 2015 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <private/misc.h>
#include <private/private.h> /* for topology->pci_nonzero_domains */
#include <hwloc.h>

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

#define PACKAGE_R_COLOR 0xde
#define PACKAGE_G_COLOR 0xde
#define PACKAGE_B_COLOR 0xde

#define MEMORY_R_COLOR 0xef
#define MEMORY_G_COLOR 0xdf
#define MEMORY_B_COLOR 0xde

#define CORE_R_COLOR 0xbe
#define CORE_G_COLOR 0xbe
#define CORE_B_COLOR 0xbe

#define THREAD_R_COLOR 0xff
#define THREAD_G_COLOR 0xff
#define THREAD_B_COLOR 0xff

#define RUNNING_R_COLOR 0
#define RUNNING_G_COLOR 0xff
#define RUNNING_B_COLOR 0

#define FORBIDDEN_R_COLOR 0xff
#define FORBIDDEN_G_COLOR 0
#define FORBIDDEN_B_COLOR 0

#define CACHE_R_COLOR 0xff
#define CACHE_G_COLOR 0xff
#define CACHE_B_COLOR 0xff

#define MACHINE_R_COLOR EPOXY_R_COLOR
#define MACHINE_G_COLOR EPOXY_G_COLOR
#define MACHINE_B_COLOR EPOXY_B_COLOR

#define NODE_R_COLOR DARK_EPOXY_R_COLOR
#define NODE_G_COLOR DARK_EPOXY_G_COLOR
#define NODE_B_COLOR DARK_EPOXY_B_COLOR

#define SYSTEM_R_COLOR 0xff
#define SYSTEM_G_COLOR 0xff
#define SYSTEM_B_COLOR 0xff

#define MISC_R_COLOR 0xff
#define MISC_G_COLOR 0xff
#define MISC_B_COLOR 0xff

#define PCI_DEVICE_R_COLOR DARKER_EPOXY_R_COLOR
#define PCI_DEVICE_G_COLOR DARKER_EPOXY_G_COLOR
#define PCI_DEVICE_B_COLOR DARKER_EPOXY_B_COLOR

#define OS_DEVICE_R_COLOR 0xde
#define OS_DEVICE_G_COLOR 0xde
#define OS_DEVICE_B_COLOR 0xde

#define BRIDGE_R_COLOR 0xff
#define BRIDGE_G_COLOR 0xff
#define BRIDGE_B_COLOR 0xff

unsigned get_textwidth(void *output,
		       const char *text, unsigned length,
		       unsigned fontsize)
{
  struct lstopo_output *loutput = output;
  if (loutput->methods->textsize) {
    unsigned width;
    loutput->methods->textsize(output, text, length, fontsize, &width);
    return width;
  }
  return (length * fontsize * 3) / 4;
}

/* preferred width/height compromise */
#define RATIO (4.f/3.f)

/* do we prefer ratio1 over ratio2? */
static int prefer_ratio(float ratio1, float ratio2) {
  float _ratio1 = (ratio1) / RATIO;
  float _ratio2 = (ratio2) / RATIO;
  if (_ratio1 < 1)
    _ratio1 = 1/_ratio1;
  if (_ratio2 < 1)
    _ratio2 = 1/_ratio2;
  return _ratio1 < _ratio2;
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

/* next child, in all children list, ignoring PU if needed */
static hwloc_obj_t next_child(struct lstopo_output *loutput, hwloc_obj_t parent, hwloc_obj_t prev)
{
  hwloc_topology_t topology = loutput->topology;
  hwloc_obj_t obj = prev;
again:
  obj = hwloc_get_next_child(topology, parent, obj);
  if (!obj)
    return NULL;
  if (obj->type == HWLOC_OBJ_PU && loutput->ignore_pus)
    goto again;
  if (loutput->collapse && obj->type == HWLOC_OBJ_PCI_DEVICE) {
    struct lstopo_obj_userdata *lud = obj->userdata;
    if (lud->pci_collapsed == -1)
      goto again;
  }
  return obj;
}

/**************************
 * Placing children
 */

static void
place_children_horiz(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned separator,
		     unsigned *width, unsigned *height)
{
  unsigned curx = 0;
  unsigned maxh = 0;
  hwloc_obj_t child;
  for(child = next_child(loutput, parent, NULL);
      child;
      child = next_child(loutput, parent, child)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    clud->xrel = curx;
    clud->yrel = 0;
    if (clud->height > maxh)
      maxh = clud->height;
    curx += separator + clud->width;
  }
  *width = curx - separator;
  *height = maxh;
}

static void
place_children_vert(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned separator,
		    unsigned *width, unsigned *height)
{
  unsigned cury = 0;
  unsigned maxw = 0;
  hwloc_obj_t child;
  for(child = next_child(loutput, parent, NULL);
      child;
      child = next_child(loutput, parent, child)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    clud->xrel = 0;
    clud->yrel = cury;
    if (clud->width > maxw)
      maxw = clud->width;
    cury += separator + clud->height;
  }
  *width = maxw;
  *height = cury - separator;
}

static void
place_children_rect(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned separator,
		    unsigned *width, unsigned *height)
{
  unsigned numsubobjs = 0, obj_totwidth = 0, obj_totheight = 0;
  unsigned area = 0;
  unsigned rows, columns;
  unsigned totwidth, totheight; /* total children array size, without borders */
  unsigned rowwidth; /* current row width */
  unsigned maxheight; /* max height for current row */
  int found, i;
  hwloc_obj_t child = NULL;

  /* Total area for subobjects */
  while ((child=next_child(loutput, parent, child)) != NULL) {
    struct lstopo_obj_userdata *clud = child->userdata;
    numsubobjs++;
    obj_totwidth += clud->width + separator;
    obj_totheight += clud->height + separator;
    area += (clud->width + separator) * (clud->height + separator);
  }

  /* Try to find a fitting rectangle */
  found = 0;
  for (rows = (unsigned) (float) floor(sqrt(numsubobjs));
       rows >= (unsigned) (float) ceil(pow(numsubobjs, 0.33)) && rows > 1;
       rows--) {
    columns = numsubobjs / rows;
    if (columns > 1 && columns * rows == numsubobjs) {
      found = 1;
      break;
    }
  }

  if (!found) {
    /* Average object size */
    unsigned obj_avgwidth = obj_totwidth / numsubobjs;
    unsigned obj_avgheight = obj_totheight / numsubobjs;
    /* Ideal total height for spreading that area with RATIO */
    float idealtotheight = (float) sqrt(area/RATIO);
    float under_ratio, over_ratio;
    /* approximation of number of rows */
    rows = (unsigned) (idealtotheight / obj_avgheight);
    columns = rows ? (numsubobjs + rows - 1) / rows : 1;
    /* Ratio obtained by underestimation */
    under_ratio = (float) (columns * obj_avgwidth) / (rows * obj_avgheight);
    /* try to overestimate too */
    rows++;
    columns = (numsubobjs + rows - 1) / rows;
    /* Ratio obtained by overestimation */
    over_ratio = (float) (columns * obj_avgwidth) / (rows * obj_avgheight);
    /* Did we actually preferred underestimation? (good row/column fit or good ratio) */
    if (rows > 1 && prefer_ratio(under_ratio, over_ratio)) {
      rows--;
      columns = (numsubobjs + rows - 1) / rows;
    }
  }

  rowwidth = 0;
  maxheight = 0;
  totwidth = 0;
  totheight = 0;
  for(i = 0, child = next_child(loutput, parent, NULL);
      child;
      i++, child = next_child(loutput, parent, child)) {
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
    clud->xrel = rowwidth;
    clud->yrel = totheight;
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

  *width = totwidth;
  *height = totheight;
}

/* Recurse into children to get their size.
 * Place them.
 * Save their position and the parent total size for later.
 */
static void
place_children(struct lstopo_output *loutput, hwloc_obj_t parent,
	       unsigned *totwidth, unsigned *totheight, /* total object size */
	       unsigned xrel, unsigned yrel /* position of children within parent */)
{
  struct lstopo_obj_userdata *plud = parent->userdata;
  enum lstopo_orient_e orient = loutput->force_orient[parent->type];
  unsigned border = loutput->gridsize;
  unsigned separator = loutput->gridsize;
  unsigned childwidth, childheight;
  int network;
  unsigned nxoff = 0, nyoff = 0;
  hwloc_obj_t child;
  unsigned i;

  /* system containing machines is drawn as network */
  /* FIXME: we only use the network code for drawing systems of machines.
   * Systems of groups (of groups ...) of machines still use boxes.
   */
  plud->network = network = (parent->type == HWLOC_OBJ_SYSTEM && parent->arity > 1 && parent->children[0]->type == HWLOC_OBJ_MACHINE);
  /* must be initialized before returning below so that draw_children() works */

  /* bridge children always vertical */
  if (parent->type == HWLOC_OBJ_BRIDGE)
    orient = LSTOPO_ORIENT_VERT;

  /* recurse into children to prepare their sizes */
  for(i = 0, child = next_child(loutput, parent, NULL);
      child;
      i++, child = next_child(loutput, parent, child)) {
    get_type_fun(child->type)(loutput, child, 0, 0, 0);
  }
  if (!i)
    return;


  /* no separator between core or L1 children */
  if (parent->type == HWLOC_OBJ_CORE
      || (hwloc_obj_type_is_cache(parent->type) && parent->attr->cache.depth == 1))
    separator = 0;


  /* actually place children */
  if (orient == LSTOPO_ORIENT_HORIZ) {
    /* force horizontal */
    place_children_horiz(loutput, parent, separator, &childwidth, &childheight);

  } else if (orient == LSTOPO_ORIENT_VERT) {
    /* force vertical */
    place_children_vert(loutput, parent, separator, &childwidth, &childheight);

  } else if (network) {
    /* NONE or forced RECT, but network only supports horiz or vert, use the best one */
    unsigned vwidth, vheight, hwidth, hheight;
    float horiz_ratio, vert_ratio;
    place_children_horiz(loutput, parent, separator, &hwidth, &hheight);
    horiz_ratio = (float)hwidth / hheight;
    place_children_vert(loutput, parent, separator, &vwidth, &vheight);
    vert_ratio = (float)vwidth / vheight;
    if (prefer_ratio(vert_ratio, horiz_ratio)) {
      /* children still contain vertical placement */
      orient = LSTOPO_ORIENT_VERT;
      childwidth = vwidth + separator;
      childheight = vheight;
      nxoff = separator;
    } else {
      /* must place horizontally again */
      orient = LSTOPO_ORIENT_HORIZ;
      place_children_horiz(loutput, parent, separator, &hwidth, &hheight);
      childwidth = hwidth;
      childheight = hheight + separator;
      nyoff = separator;
    }

  } else {
    /* NONE or forced RECT, do a rectangular placement */
    place_children_rect(loutput, parent, separator, &childwidth, &childheight);
  }

  /* adjust parent size */
  if (hwloc_obj_type_is_cache(parent->type)) {
    /* cache children are below */
    if (childwidth > *totwidth)
      *totwidth = childwidth;
    if (childheight)
      *totheight += childheight + border;
  } else if (parent->type == HWLOC_OBJ_BRIDGE) {
    /* bridge children are on the right, within any space between bridge and children */
    if (childwidth)
      *totwidth += childwidth;
    if (childheight > *totheight)
      *totheight = childheight;
  } else {
    /* normal objects have children inside their box, with space around them */
    if (childwidth + 2*border > *totwidth)
      *totwidth = childwidth + 2*border;
    if (childheight)
      *totheight += childheight + border;
  }

  /* save config for draw_children() later */
  plud->orient = orient;
  plud->network = network;
  plud->children_xrel = xrel + nxoff;
  plud->children_yrel = yrel + nyoff;
}

/***********************
 * Drawing children
 */

static void
draw_children_network(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned depth,
		      unsigned x, unsigned y)
{
  unsigned separator = loutput->gridsize;
  struct lstopo_obj_userdata *plud = parent->userdata;
  enum lstopo_orient_e orient = plud->orient;

  if (orient == LSTOPO_ORIENT_HORIZ) {
    hwloc_obj_t child;
    unsigned xmax;
    unsigned xmin = (unsigned) -1;
    for(child = next_child(loutput, parent, NULL);
	child;
	child = next_child(loutput, parent, child)) {
      struct lstopo_obj_userdata *clud = child->userdata;
      unsigned xmid = clud->xrel + clud->width / 2;
      loutput->methods->line(loutput, 0, 0, 0, depth, x+xmid, y-separator, x+xmid, y);
      if (xmin == (unsigned) -1)
	xmin = xmid;
      xmax = xmid;
    }
    assert(xmax != xmin);
    loutput->methods->line(loutput, 0, 0, 0, depth, x+xmin, y-separator, x+xmax, y-separator);

  } else if (orient == LSTOPO_ORIENT_VERT) {
    hwloc_obj_t child;
    unsigned ymax;
    unsigned ymin = (unsigned) -1;
    for(child = next_child(loutput, parent, NULL);
	child;
	child = next_child(loutput, parent, child)) {
      struct lstopo_obj_userdata *clud = child->userdata;
      unsigned ymid = clud->yrel + clud->height / 2;
      loutput->methods->line(loutput, 0, 0, 0, depth, x-separator, y+ymid, x, y+ymid);
      if (ymin == (unsigned) -1)
	ymin = ymid;
      ymax = ymid;
    }
    assert(ymax != ymin);
    loutput->methods->line(loutput, 0, 0, 0, depth, x-separator, y+ymin, x-separator, y+ymax);

  } else {
    assert(0);
  }
}

static void
draw_children(struct lstopo_output *loutput, hwloc_obj_t parent, unsigned depth,
	      unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *plud = parent->userdata;
  hwloc_obj_t child;

  /* add children zone offset to the parent top-left corner */
  x += plud->children_xrel;
  y += plud->children_yrel;

  for(child = next_child(loutput, parent, NULL);
      child;
      child = next_child(loutput, parent, child)) {
    struct lstopo_obj_userdata *clud = child->userdata;
    get_type_fun(child->type)(loutput, child, depth, x + clud->xrel, y + clud->yrel);
  }

  if (plud->network)
    draw_children_network(loutput, parent, depth, x, y);
}

/*******
 * Misc
 */

static int
lstopo_obj_snprintf(struct lstopo_output *loutput, char *text, size_t textlen, hwloc_obj_t obj)
{
  int logical = loutput->logical;
  unsigned idx = logical ? obj->logical_index : obj->os_index;
  const char *indexprefix = logical ? " L#" : " P#";
  char typestr[32];
  char indexstr[32]= "";
  char attrstr[256];
  char totmemstr[64] = "";
  int attrlen;

  /* For OSDev, Misc and Group, name replaces type+index+attrs */
  if (obj->name && (obj->type == HWLOC_OBJ_OS_DEVICE || obj->type == HWLOC_OBJ_MISC || obj->type == HWLOC_OBJ_GROUP)) {
    return snprintf(text, textlen, "%s", obj->name);
  }

  /* subtype replaces the basic type name */
  if (obj->subtype) {
    snprintf(typestr, sizeof(typestr), "%s", obj->subtype);
  } else {
    hwloc_obj_type_snprintf(typestr, sizeof(typestr), obj, 0);
  }

  if (loutput->show_indexes[obj->type]
      && idx != (unsigned)-1 && obj->depth != 0
      && obj->type != HWLOC_OBJ_PCI_DEVICE
      && (obj->type != HWLOC_OBJ_BRIDGE || obj->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_HOST))
    snprintf(indexstr, sizeof(indexstr), "%s%u", indexprefix, idx);

  if (loutput->show_attrs[obj->type]) {
    attrlen = hwloc_obj_attr_snprintf(attrstr, sizeof(attrstr), obj, " ", 0);
    /* display the root total_memory if different from the local_memory (already shown) */
    if (!obj->parent && obj->memory.total_memory > obj->memory.local_memory)
      snprintf(totmemstr, sizeof(totmemstr), " (%lu%s total)",
	       (unsigned long) hwloc_memory_size_printf_value(obj->memory.total_memory, 0),
	       hwloc_memory_size_printf_unit(obj->memory.total_memory, 0));
  } else
    attrlen = 0;

  if (attrlen > 0)
    return snprintf(text, textlen, "%s%s (%s)%s", typestr, indexstr, attrstr, totmemstr);
  else
    return snprintf(text, textlen, "%s%s%s", typestr, indexstr, totmemstr);
}

static void
lstopo_prepare_custom_styles(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  struct lstopo_obj_userdata *lud = obj->userdata;
  struct style *s = &lud->style;
  hwloc_obj_t child;
  unsigned forcer, forceg, forceb;
  const char *stylestr;

  lud->style_set = 0;

  stylestr = hwloc_obj_get_info_by_name(obj, "lstopoStyle");
  if (stylestr) {
    while (*stylestr != '\0') {
      if (sscanf(stylestr, "%02x%02x%02x", &forcer, &forceg, &forceb) == 3
	  || sscanf(stylestr, "Background=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	s->bg.r = forcer & 255;
	s->bg.g = forceg & 255;
	s->bg.b = forceb & 255;
	lud->style_set |= 0x1;
	loutput->methods->declare_color(loutput, s->bg.r, s->bg.g, s->bg.b);
	s->t.r = s->t.g = s->t.b = (s->bg.r + s->bg.g + s->bg.b < 0xff) ? 0xff : 0;
    } else if (sscanf(stylestr, "Background2=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	s->bg2.r = forcer & 255;
	s->bg2.g = forceg & 255;
	s->bg2.b = forceb & 255;
	lud->style_set |= 0x2;
	loutput->methods->declare_color(loutput, s->bg2.r, s->bg2.g, s->bg2.b);
	s->t2.r = s->t2.g = s->t2.b = (s->bg2.r + s->bg2.g + s->bg2.b < 0xff) ? 0xff : 0;
      } else if (sscanf(stylestr, "Text=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	s->t.r = forcer & 255;
	s->t.g = forceg & 255;
	s->t.b = forceb & 255;
	lud->style_set |= 0x4;
	loutput->methods->declare_color(loutput, s->t.r, s->t.g, s->t.b);
      } else if (sscanf(stylestr, "Text2=#%02x%02x%02x", &forcer, &forceg, &forceb) == 3) {
	s->t2.r = forcer & 255;
	s->t2.g = forceg & 255;
	s->t2.b = forceb & 255;
	lud->style_set |= 0x8;
	loutput->methods->declare_color(loutput, s->t2.r, s->t2.g, s->t2.b);
      }
      stylestr = strchr(stylestr, ';');
      if (!stylestr)
	break;
      stylestr++;
    }
  }

  for_each_child(child, obj)
    lstopo_prepare_custom_styles(loutput, child);
  for_each_io_child(child, obj)
    lstopo_prepare_custom_styles(loutput, child);
  for_each_misc_child(child, obj)
    lstopo_prepare_custom_styles(loutput, child);
}

static void
lstopo_set_object_color(struct lstopo_output *loutput,
			hwloc_obj_t obj,
			struct style *s)
{
  struct lstopo_obj_userdata *lud = obj->userdata;

  memset(s, 0, sizeof(*s));

  switch (obj->type) {

  case HWLOC_OBJ_MACHINE:
    if (obj->depth) {
      s->bg.r = MACHINE_R_COLOR;
      s->bg.g = MACHINE_G_COLOR;
      s->bg.b = MACHINE_B_COLOR;
      break;
    }
    /* Machine root printed as a System */
    /* FALLTHRU */
  case HWLOC_OBJ_SYSTEM:
    s->bg.r = SYSTEM_R_COLOR;
    s->bg.g = SYSTEM_G_COLOR;
    s->bg.b = SYSTEM_B_COLOR;
    break;

  case HWLOC_OBJ_GROUP:
    s->bg.r = MISC_R_COLOR;
    s->bg.g = MISC_G_COLOR;
    s->bg.b = MISC_B_COLOR;
    break;

  case HWLOC_OBJ_MISC:
    s->bg.r = MISC_R_COLOR;
    s->bg.g = MISC_G_COLOR;
    s->bg.b = MISC_B_COLOR;
    break;

  case HWLOC_OBJ_NUMANODE:
    s->bg.r = NODE_R_COLOR;
    s->bg.g = NODE_G_COLOR;
    s->bg.b = NODE_B_COLOR;
    s->bg2.r = MEMORY_R_COLOR;
    s->bg2.g = MEMORY_G_COLOR;
    s->bg2.b = MEMORY_B_COLOR;
    break;

  case HWLOC_OBJ_PACKAGE:
    s->bg.r = PACKAGE_R_COLOR;
    s->bg.g = PACKAGE_G_COLOR;
    s->bg.b = PACKAGE_B_COLOR;
    break;

  case HWLOC_OBJ_CORE:
    s->bg.r = CORE_R_COLOR;
    s->bg.g = CORE_G_COLOR;
    s->bg.b = CORE_B_COLOR;
    break;

  case HWLOC_OBJ_L1CACHE:
  case HWLOC_OBJ_L2CACHE:
  case HWLOC_OBJ_L3CACHE:
  case HWLOC_OBJ_L4CACHE:
  case HWLOC_OBJ_L5CACHE:
  case HWLOC_OBJ_L1ICACHE:
  case HWLOC_OBJ_L2ICACHE:
  case HWLOC_OBJ_L3ICACHE:
    s->bg.r = CACHE_R_COLOR;
    s->bg.g = CACHE_G_COLOR;
    s->bg.b = CACHE_B_COLOR;
    break;

  case HWLOC_OBJ_PU:
    if (lstopo_pu_forbidden(obj)) {
      s->bg.r = FORBIDDEN_R_COLOR;
      s->bg.g = FORBIDDEN_G_COLOR;
      s->bg.b = FORBIDDEN_B_COLOR;
    } else if (lstopo_pu_running(loutput, obj)) {
      s->bg.r = RUNNING_R_COLOR;
      s->bg.g = RUNNING_G_COLOR;
      s->bg.b = RUNNING_B_COLOR;
    } else {
      s->bg.r = THREAD_R_COLOR;
      s->bg.g = THREAD_G_COLOR;
      s->bg.b = THREAD_B_COLOR;
    }
    break;

  case HWLOC_OBJ_BRIDGE:
    s->bg.r = BRIDGE_R_COLOR;
    s->bg.g = BRIDGE_G_COLOR;
    s->bg.b = BRIDGE_B_COLOR;
    break;

  case HWLOC_OBJ_PCI_DEVICE:
    s->bg.r = PCI_DEVICE_R_COLOR;
    s->bg.g = PCI_DEVICE_G_COLOR;
    s->bg.b = PCI_DEVICE_B_COLOR;
    break;

  case HWLOC_OBJ_OS_DEVICE:
    s->bg.r = OS_DEVICE_R_COLOR;
    s->bg.g = OS_DEVICE_G_COLOR;
    s->bg.b = OS_DEVICE_B_COLOR;
    break;

  default:
    assert(0);
  }

  if (lud->style_set & 0x1)
    memcpy(&s->bg, &lud->style.bg, sizeof(struct stylecolor));
  if (lud->style_set & 0x2)
    memcpy(&s->t, &lud->style.t, sizeof(struct stylecolor));
  if (lud->style_set & 0x4)
    memcpy(&s->bg2, &lud->style.bg2, sizeof(struct stylecolor));
  if (lud->style_set & 0x8)
    memcpy(&s->t2, &lud->style.t2, sizeof(struct stylecolor));
}

static void
prepare_more_text(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  struct lstopo_obj_userdata *lud = obj->userdata;
  unsigned i;

  if (!loutput->show_attrs[obj->type])
    return;

  if (HWLOC_OBJ_OS_DEVICE == obj->type) {
    if (HWLOC_OBJ_OSDEV_COPROC == obj->attr->osdev.type && obj->subtype) {
      /* Coprocessor */
      if (!strcmp(obj->subtype, "CUDA")) {
	/* CUDA */
	const char *value, *value2, *value3;
	value = hwloc_obj_get_info_by_name(obj, "CUDAGlobalMemorySize");
	if (value) {
	  unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   mb >= 10240 ? "%llu GB" : "%llu MB",
		   mb >= 10240 ? mb/1024 : mb);
	}
	value = hwloc_obj_get_info_by_name(obj, "CUDAL2CacheSize");
	if (value) {
	  unsigned long long kb = strtoull(value, NULL, 10);
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   kb >= 10240 ? "L2 (%llu MB)" : "L2 (%llu kB)",
		   kb >= 10240 ? kb/1024 : kb);
	}
	value = hwloc_obj_get_info_by_name(obj, "CUDAMultiProcessors");
	value2 = hwloc_obj_get_info_by_name(obj, "CUDACoresPerMP");
	value3 = hwloc_obj_get_info_by_name(obj, "CUDASharedMemorySizePerMP");
	if (value && value2 && value3) {
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   "%s MP x (%s cores + %s kB)", value, value2, value3);
	}

      } else if (!strcmp(obj->subtype, "MIC")) {
	/* MIC */
	const char *value;
	value = hwloc_obj_get_info_by_name(obj, "MICActiveCores");
	if (value) {
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   "%s cores", value);
	}
	value = hwloc_obj_get_info_by_name(obj, "MICMemorySize");
	if (value) {
	  unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   mb >= 10240 ? "%llu GB" : "%llu MB",
		   mb >= 10240 ? mb/1024 : mb);
	}

      } else if (!strcmp(obj->subtype, "OpenCL")) {
	/* OpenCL */
	const char *value;
	value = hwloc_obj_get_info_by_name(obj, "OpenCLComputeUnits");
	if (value) {
	  unsigned long long cu = strtoull(value, NULL, 10);
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		   "%llu compute units", cu);
	}
	value = hwloc_obj_get_info_by_name(obj, "OpenCLGlobalMemorySize");
	if (value) {
	  unsigned long long mb = strtoull(value, NULL, 10) / 1024;
	  snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
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
	snprintf(lud->text[lud->ntext++], sizeof(lud->text[0]),
		 mb >= 10485760 ? "%llu TB" : mb >= 10240 ? "%llu GB" : "%llu MB",
		 mb >= 10485760 ? mb/1048576 : mb >= 10240 ? mb/1024 : mb);
      }
    }
  }

  for(i=1; i<lud->ntext; i++) {
    unsigned nn = (unsigned)strlen(lud->text[i]);
    unsigned ntextwidth = get_textwidth(loutput, lud->text[i], nn, loutput->fontsize);
    if (ntextwidth > lud->textwidth)
      lud->textwidth = ntextwidth;
  }
}

static void
prepare_text(struct lstopo_output *loutput, hwloc_obj_t obj)
{
  struct lstopo_obj_userdata *lud = obj->userdata;
  unsigned fontsize = loutput->fontsize;
  int n;

  /* sane defaults */
  lud->ntext = 0;
  lud->textwidth = 0;
  lud->textxoffset = 0;

  if (!fontsize)
    return;

  /* main object identifier line */
  if (obj->type == HWLOC_OBJ_PCI_DEVICE && loutput->show_attrs[HWLOC_OBJ_PCI_DEVICE]) {
    /* PCI text collapsing */
    char busid[32];
    char _text[64];
    lstopo_obj_snprintf(loutput, _text, sizeof(_text), obj);
    lstopo_busid_snprintf(busid, sizeof(busid), obj, lud->pci_collapsed, loutput->topology->pci_nonzero_domains);
    if (lud->pci_collapsed > 1) {
      n = snprintf(lud->text[0], sizeof(lud->text[0]), "%d x { %s %s }", lud->pci_collapsed, _text, busid);
    } else {
      n = snprintf(lud->text[0], sizeof(lud->text[0]), "%s %s", _text, busid);
    }
  } else {
    /* normal object text */
    n = lstopo_obj_snprintf(loutput, lud->text[0], sizeof(lud->text[0]), obj);
  }
  lud->textwidth = get_textwidth(loutput, lud->text[0], n, fontsize);
  lud->ntext = 1;

  if (obj->type == HWLOC_OBJ_PU) {
    /* if smaller than other PU, artificially extend/shift it
     * to make PU boxes nicer when vertically stacked.
     */
    if (lud->textwidth < loutput->min_pu_textwidth) {
      lud->textxoffset = (loutput->min_pu_textwidth - lud->textwidth) / 2;
      lud->textwidth = loutput->min_pu_textwidth;
    }
  }

  /* additional text */
  prepare_more_text(loutput, obj);
}

/* additional gridsize when fontsize > 0 */
#define FONTGRIDSIZE (fontsize ? gridsize : 0)

static void
draw_text(struct lstopo_output *loutput, hwloc_obj_t obj, struct stylecolor *color, unsigned depth, unsigned x, unsigned y)
{
  struct draw_methods *methods = loutput->methods;
  struct lstopo_obj_userdata *lud = obj->userdata;
  unsigned fontsize = loutput->fontsize;
  unsigned gridsize = loutput->gridsize;
  unsigned i;

  if (!fontsize)
    return;

  methods->text(loutput, color->r, color->g, color->b, fontsize, depth, x + lud->textxoffset, y, lud->text[0]);
  for(i=1; i<lud->ntext; i++)
    methods->text(loutput, color->r, color->g, color->b, fontsize, depth, x, y + i*gridsize + i*fontsize, lud->text[i]);
}

static void
pci_device_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned totwidth, totheight;
  unsigned overlaidoffset = 0;

  if (lud->pci_collapsed > 1) {
    /* additional depths and height for overlaid boxes */
    depth -= 2;
    if (lud->pci_collapsed > 2) {
      overlaidoffset = gridsize;
    } else {
      overlaidoffset = gridsize/2;
    }
  }

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    totwidth = lud->textwidth + gridsize + overlaidoffset + FONTGRIDSIZE;
    totheight = fontsize + gridsize + overlaidoffset + FONTGRIDSIZE;
    place_children(loutput, level, &totwidth, &totheight,
		   gridsize, fontsize + gridsize + FONTGRIDSIZE);
    lud->width = totwidth;
    lud->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct style style;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);

    if (lud->pci_collapsed > 1) {
      methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth+2, x + overlaidoffset, totwidth - overlaidoffset, y + overlaidoffset, totheight - overlaidoffset);
      if (lud->pci_collapsed > 2)
	methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth+1, x + overlaidoffset/2, totwidth - overlaidoffset, y + overlaidoffset/2, totheight - overlaidoffset);
      methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, totwidth - overlaidoffset, y, totheight - overlaidoffset);
    } else {
      methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, totwidth, y, totheight);
    }

    draw_text(loutput, level, &style.t, depth-1, x + gridsize, y + gridsize);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

/* bridge object height: linkspeed + a small empty box */
#define BRIDGE_HEIGHT (gridsize + fontsize + FONTGRIDSIZE)

static void
bridge_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned totwidth, totheight;
  unsigned speedwidth = fontsize ? fontsize + gridsize : 0;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    totwidth = 2*gridsize + gridsize + speedwidth;
    totheight = gridsize + FONTGRIDSIZE;
    place_children(loutput, level, &totwidth, &totheight,
		   3*gridsize + speedwidth, 0);
    lud->width = totwidth;
    lud->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct style style;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    /* Square and left link */
    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, gridsize, y + BRIDGE_HEIGHT/2 - gridsize/2, gridsize);
    methods->line(loutput, 0, 0, 0, depth, x + gridsize, y + BRIDGE_HEIGHT/2, x + 2*gridsize, y + BRIDGE_HEIGHT/2);

    if (level->io_arity > 0) {
      hwloc_obj_t child = NULL;
      unsigned ymax = -1;
      unsigned ymin = (unsigned) -1;
      while ((child=next_child(loutput, level, child)) != NULL) {
	struct lstopo_obj_userdata *clud = child->userdata;
	unsigned ymid = y + clud->yrel + BRIDGE_HEIGHT/2;
	/* Line to PCI device */
	methods->line(loutput, 0, 0, 0, depth-1, x+2*gridsize, ymid, x+3*gridsize+speedwidth, ymid);
	if (ymin == (unsigned) -1)
	  ymin = ymid;
	ymax = ymid;
	/* Negotiated link speed */
	if (fontsize) {
	  float speed = 0.;
	  if (child->type == HWLOC_OBJ_PCI_DEVICE)
	    speed = child->attr->pcidev.linkspeed;
	  if (child->type == HWLOC_OBJ_BRIDGE && child->attr->bridge.upstream_type == HWLOC_OBJ_BRIDGE_PCI)
	    speed = child->attr->bridge.upstream.pci.linkspeed;
	  if (loutput->show_attrs[HWLOC_OBJ_BRIDGE] && speed != 0.) {
	    char text[4];
	    if (speed >= 10.)
	      snprintf(text, sizeof(text), "%.0f", child->attr->pcidev.linkspeed);
	    else
	      snprintf(text, sizeof(text), "%0.1f", child->attr->pcidev.linkspeed);
	    methods->text(loutput, style.t2.r, style.t2.g, style.t2.b, fontsize, depth-1, x + 3*gridsize, ymid - BRIDGE_HEIGHT/2, text);
	  }
	}
      }
      methods->line(loutput, 0, 0, 0, depth-1, x+2*gridsize, ymin, x+2*gridsize, ymax);

      /* Draw sublevels for real */
      draw_children(loutput, level, depth-1, x, y);
    }
  }
}

static void
cache_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned totwidth, totheight;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    totwidth = lud->textwidth + gridsize + FONTGRIDSIZE;
    totheight = fontsize + gridsize + FONTGRIDSIZE;
    place_children(loutput, level, &totwidth, &totheight,
		   0, fontsize + 2*gridsize + FONTGRIDSIZE);
    lud->width = totwidth;
    lud->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct style style;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, totwidth, y, fontsize + gridsize + FONTGRIDSIZE /* totheight also contains children below this box */);

    draw_text(loutput, level, &style.t, depth-1, x + gridsize, y + gridsize);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

static void
node_draw(struct lstopo_output *loutput, hwloc_obj_t level, unsigned depth, unsigned x, unsigned y)
{
  struct lstopo_obj_userdata *lud = level->userdata;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  unsigned totwidth, totheight;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    totwidth = lud->textwidth + 3*gridsize + FONTGRIDSIZE;
    totheight = fontsize + 3*gridsize + FONTGRIDSIZE;
    place_children(loutput, level, &totwidth, &totheight,
		   gridsize, fontsize + 3*gridsize + FONTGRIDSIZE);
    lud->width = totwidth;
    lud->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct style style;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);
    /* Draw the epoxy box */
    methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, totwidth, y, totheight);
    /* Draw the memory box */
    methods->box(loutput, style.bg2.r, style.bg2.g, style.bg2.b, depth-1, x + gridsize, totwidth - 2*gridsize, y + gridsize, fontsize + gridsize + FONTGRIDSIZE);
    draw_text(loutput, level, &style.t2, depth-2, x + 2*gridsize, y + 2*gridsize);

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
  unsigned totwidth, totheight;

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute children size and position, our size, and save it */
    prepare_text(loutput, level);
    totwidth = lud->textwidth + gridsize + FONTGRIDSIZE;
    totheight = gridsize + (fontsize + FONTGRIDSIZE) * lud->ntext;
    place_children(loutput, level, &totwidth, &totheight,
		   gridsize, gridsize + (fontsize + FONTGRIDSIZE) * lud->ntext);
    lud->width = totwidth;
    lud->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    struct draw_methods *methods = loutput->methods;
    struct style style;

    /* restore our size that was computed during prepare */
    totwidth = lud->width;
    totheight = lud->height;

    lstopo_set_object_color(loutput, level, &style);
    methods->box(loutput, style.bg.r, style.bg.g, style.bg.b, depth, x, totwidth, y, totheight);
    draw_text(loutput, level, &style.t, depth-1, x + gridsize, y + gridsize);

    /* Draw sublevels for real */
    draw_children(loutput, level, depth-1, x, y);
  }
}

static void
output_compute_pu_min_textwidth(struct lstopo_output *output)
{
  unsigned fontsize = output->fontsize;
  char text[64];
  int n;
  hwloc_topology_t topology = output->topology;
  hwloc_obj_t lastpu;

  if (!output->methods->textsize) {
    output->min_pu_textwidth = 0;
    return;
  }

  if (output->logical) {
    unsigned depth = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
    lastpu = hwloc_get_obj_by_depth(topology, depth, hwloc_get_nbobjs_by_depth(topology, depth)-1);
  } else {
    unsigned lastidx = hwloc_bitmap_last(hwloc_topology_get_topology_cpuset(topology));
    lastpu = hwloc_get_pu_obj_by_os_index(topology, lastidx);
  }

  n = lstopo_obj_snprintf(output, text, sizeof(text), lastpu);
  output->min_pu_textwidth = get_textwidth(output, text, n, fontsize);
}

void
output_draw(struct lstopo_output *loutput)
{
  hwloc_topology_t topology = loutput->topology;
  struct draw_methods *methods = loutput->methods;
  int legend = loutput->legend;
  unsigned gridsize = loutput->gridsize;
  unsigned fontsize = loutput->fontsize;
  hwloc_obj_t root = hwloc_get_root_obj(topology);
  struct lstopo_obj_userdata *rlud = root->userdata;
  unsigned depth = 100;
  unsigned totwidth, totheight, offset, i;
  time_t t;
  char text[3][128];
  unsigned ntext = 0;
  char hostname[128] = "";
  const char *forcedhostname = NULL;
  unsigned long hostname_size = sizeof(hostname);
  unsigned maxtextwidth = 0, textwidth;

  if (legend && fontsize) {
    forcedhostname = hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "HostName");
    if (!forcedhostname && hwloc_topology_is_thissystem(topology)) {
#if defined(HWLOC_WIN_SYS) && !defined(__CYGWIN__)
      GetComputerName(hostname, &hostname_size);
#else
      gethostname(hostname, hostname_size);
#endif
    }
    if (forcedhostname || *hostname) {
      if (forcedhostname)
	snprintf(text[ntext], sizeof(text[ntext]), "Host: %s", forcedhostname);
      else
	snprintf(text[ntext], sizeof(text[ntext]), "Host: %s", hostname);
      textwidth = get_textwidth(loutput, text[ntext], (unsigned) strlen(text[ntext]), fontsize);
      if (textwidth > maxtextwidth)
	maxtextwidth = textwidth;
      ntext++;
    }

    /* Display whether we're showing physical or logical IDs */
    snprintf(text[ntext], sizeof(text[ntext]), "Indexes: %s", loutput->logical ? "logical" : "physical");
    textwidth = get_textwidth(loutput, text[ntext], (unsigned) strlen(text[ntext]), fontsize);
    if (textwidth > maxtextwidth)
      maxtextwidth = textwidth;
    ntext++;

    /* Display timestamp */
    t = time(NULL);
#ifdef HAVE_STRFTIME
    {
      struct tm *tmp;
      tmp = localtime(&t);
      strftime(text[ntext], sizeof(text[ntext]), "Date: %c", tmp);
    }
#else /* HAVE_STRFTIME */
    {
      char *date;
      int n;
      date = ctime(&t);
      n = strlen(date);
      if (n && date[n-1] == '\n') {
        date[n-1] = 0;
      }
      snprintf(text[ntext], sizeof(text[ntext]), "Date: %s", date);
    }
#endif /* HAVE_STRFTIME */
    textwidth = get_textwidth(loutput, text[ntext], (unsigned) strlen(text[ntext]), fontsize);
    if (textwidth > maxtextwidth)
      maxtextwidth = textwidth;
    ntext++;
  }

  if (loutput->drawing == LSTOPO_DRAWING_PREPARE) {
    /* compute root size, our size, and save it */

    output_compute_pu_min_textwidth(loutput);

    get_type_fun(root->type)(loutput, root, depth, 0, 0);

    /* loutput width is max(root, legend) */
    totwidth = rlud->width;
    if (maxtextwidth + 2*gridsize > totwidth)
      totwidth = maxtextwidth + 2*gridsize;
    loutput->width = totwidth;

    /* loutput height is sum(root, legend) */
    totheight = rlud->height;
    if (legend && fontsize)
      totheight += gridsize + (ntext+loutput->legend_append_nr) * (gridsize+fontsize);
    loutput->height = totheight;

  } else { /* LSTOPO_DRAWING_DRAW */
    /* restore our size that was computed during prepare */
    totwidth = rlud->width;
    totheight = rlud->height;

    /* Draw root for real */
    get_type_fun(root->type)(loutput, root, depth, 0, 0);

    /* Draw legend */
    if (legend && fontsize) {
      offset = rlud->height + gridsize;
      methods->box(loutput, 0xff, 0xff, 0xff, depth, 0, loutput->width, totheight, gridsize + (ntext+loutput->legend_append_nr) * (gridsize+fontsize));
      for(i=0; i<ntext; i++, offset += gridsize + fontsize)
	methods->text(loutput, 0, 0, 0, fontsize, depth, gridsize, offset, text[i]);
      for(i=0; i<loutput->legend_append_nr; i++, offset += gridsize + fontsize)
	methods->text(loutput, 0, 0, 0, fontsize, depth, gridsize, offset, loutput->legend_append[i]);
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
    case HWLOC_OBJ_SYSTEM:
    case HWLOC_OBJ_MACHINE:
    case HWLOC_OBJ_PACKAGE:
    case HWLOC_OBJ_CORE:
    case HWLOC_OBJ_PU:
    case HWLOC_OBJ_GROUP:
    case HWLOC_OBJ_OS_DEVICE:
    case HWLOC_OBJ_MISC: return normal_draw;
    case HWLOC_OBJ_NUMANODE: return node_draw;
    case HWLOC_OBJ_L1CACHE: return cache_draw;
    case HWLOC_OBJ_L2CACHE: return cache_draw;
    case HWLOC_OBJ_L3CACHE: return cache_draw;
    case HWLOC_OBJ_L4CACHE: return cache_draw;
    case HWLOC_OBJ_L5CACHE: return cache_draw;
    case HWLOC_OBJ_L1ICACHE: return cache_draw;
    case HWLOC_OBJ_L2ICACHE: return cache_draw;
    case HWLOC_OBJ_L3ICACHE: return cache_draw;
    case HWLOC_OBJ_PCI_DEVICE: return pci_device_draw;
    case HWLOC_OBJ_BRIDGE: return bridge_draw;
    default:
    case HWLOC_OBJ_TYPE_MAX: assert(0);
  }
  /* for dumb compilers */
  return normal_draw;
}

void
output_draw_start(struct lstopo_output *output)
{
  struct draw_methods *methods = output->methods;
  methods->init(output);
  methods->declare_color(output, 0, 0, 0);
  methods->declare_color(output, NODE_R_COLOR, NODE_G_COLOR, NODE_B_COLOR);
  methods->declare_color(output, PACKAGE_R_COLOR, PACKAGE_G_COLOR, PACKAGE_B_COLOR);
  methods->declare_color(output, MEMORY_R_COLOR, MEMORY_G_COLOR, MEMORY_B_COLOR);
  methods->declare_color(output, CORE_R_COLOR, CORE_G_COLOR, CORE_B_COLOR);
  methods->declare_color(output, THREAD_R_COLOR, THREAD_G_COLOR, THREAD_B_COLOR);
  methods->declare_color(output, RUNNING_R_COLOR, RUNNING_G_COLOR, RUNNING_B_COLOR);
  methods->declare_color(output, FORBIDDEN_R_COLOR, FORBIDDEN_G_COLOR, FORBIDDEN_B_COLOR);
  methods->declare_color(output, CACHE_R_COLOR, CACHE_G_COLOR, CACHE_B_COLOR);
  methods->declare_color(output, MACHINE_R_COLOR, MACHINE_G_COLOR, MACHINE_B_COLOR);
  methods->declare_color(output, SYSTEM_R_COLOR, SYSTEM_G_COLOR, SYSTEM_B_COLOR);
  methods->declare_color(output, MISC_R_COLOR, MISC_G_COLOR, MISC_B_COLOR);
  methods->declare_color(output, PCI_DEVICE_R_COLOR, PCI_DEVICE_G_COLOR, PCI_DEVICE_B_COLOR);
  methods->declare_color(output, BRIDGE_R_COLOR, BRIDGE_G_COLOR, BRIDGE_B_COLOR);
  lstopo_prepare_custom_styles(output, hwloc_get_root_obj(output->topology));
}
