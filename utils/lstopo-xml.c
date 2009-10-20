/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>

#ifdef HAVE_XML

#include <hwloc.h>
#include <string.h>

#include <strings.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <strings.h>

#include "lstopo.h"

static void
output_topology (hwloc_topology_t topology, hwloc_obj_t obj, xmlNodePtr root_node, int verbose_mode)
{
  xmlNodePtr node = NULL;
  char *cpuset = NULL;
  char tmp[255];

  /* xmlNewChild() creates a new node, which is "attached" as child node
   * of root_node node. */
  node = xmlNewChild(root_node, NULL, BAD_CAST "object", NULL);
  xmlNewProp(node, BAD_CAST "type", BAD_CAST hwloc_obj_type_string(obj->type));
  sprintf(tmp, "%d", obj->os_index);
  xmlNewProp(node, BAD_CAST "os_index", BAD_CAST tmp);
  hwloc_cpuset_asprintf(&cpuset, obj->cpuset);
  xmlNewProp(node, BAD_CAST "cpuset", BAD_CAST cpuset);
  free(cpuset);

  switch (obj->type) {
  case HWLOC_OBJ_CACHE:
    sprintf(tmp, "%lu", obj->attr->cache.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%u", obj->attr->cache.depth);
    xmlNewProp(node, BAD_CAST "depth", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_SYSTEM:
    xmlNewProp(node, BAD_CAST "dmi_board_vendor", BAD_CAST obj->attr->machine.dmi_board_vendor);
    xmlNewProp(node, BAD_CAST "dmi_board_name", BAD_CAST obj->attr->machine.dmi_board_name);
    sprintf(tmp, "%lu", obj->attr->system.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->system.huge_page_free);
    xmlNewProp(node, BAD_CAST "huge_page_free", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->machine.huge_page_size_kB);
    xmlNewProp(node, BAD_CAST "huge_page_size_kB", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_MACHINE:
    xmlNewProp(node, BAD_CAST "dmi_board_vendor", BAD_CAST obj->attr->machine.dmi_board_vendor);
    xmlNewProp(node, BAD_CAST "dmi_board_name", BAD_CAST obj->attr->machine.dmi_board_name);
    sprintf(tmp, "%lu", obj->attr->machine.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->machine.huge_page_free);
    xmlNewProp(node, BAD_CAST "huge_page_free", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->machine.huge_page_size_kB);
    xmlNewProp(node, BAD_CAST "huge_page_size_kB", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_NODE:
    sprintf(tmp, "%lu", obj->attr->node.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->node.huge_page_free);
    xmlNewProp(node, BAD_CAST "huge_page_free", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_MISC:
    sprintf(tmp, "%u", obj->attr->misc.depth);
    xmlNewProp(node, BAD_CAST "depth", BAD_CAST tmp);
    break;
  default:
    break;
  }

  if (obj->arity) {
    int x;
    for (x=0; x<obj->arity; x++)
      output_topology (topology, obj->children[x], node, verbose_mode);
  }
}

void output_xml(hwloc_topology_t topology, const char *filename, int verbose_mode)
{
  xmlDocPtr doc = NULL;       /* document pointer */
  xmlNodePtr root_node = NULL; /* root pointer */
  xmlDtdPtr dtd = NULL;       /* DTD pointer */

  if (!strcasecmp(filename, "-.xml"))
    filename = "-";

  LIBXML_TEST_VERSION;

  /* Creates a new document, a node and set it as a root node. */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "root");
  xmlDocSetRootElement(doc, root_node);

  /* Creates a DTD declaration. Isn't mandatory. */
  dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "hwloc.dtd");

  output_topology (topology, hwloc_get_system_obj(topology), root_node, verbose_mode);

  /* Dumping document to stdio or file. */
  xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

  /* Free the document. */
  xmlFreeDoc(doc);

  /* Free the global variables that may have been allocated by the parser. */
  xmlCleanupParser();
}

#endif /* HAVE_XML */
