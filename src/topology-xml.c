/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HWLOC_HAVE_XML

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <assert.h>
#include <strings.h>

int
hwloc_backend_xml_init(struct hwloc_topology *topology, const char *xmlpath)
{
  xmlDoc *doc = NULL;

  assert(topology->backend_type == HWLOC_BACKEND_NONE);

  LIBXML_TEST_VERSION;

  doc = xmlReadFile(xmlpath, NULL, 0);
  if (!doc)
    return -1;

  topology->backend_params.xml.doc = doc;
  topology->is_thissystem = 0;
  topology->backend_type = HWLOC_BACKEND_XML;
  return 0;
}

void
hwloc_backend_xml_exit(struct hwloc_topology *topology)
{
  assert(topology->backend_type == HWLOC_BACKEND_XML);
  xmlFreeDoc((xmlDoc*)topology->backend_params.xml.doc);
  xmlCleanupParser();
  topology->backend_type = HWLOC_BACKEND_NONE;
}

/******************************
 ********* XML import *********
 ******************************/

static void
hwloc__xml_import_topology_attr(struct hwloc_topology *topology __hwloc_attribute_unused,
				const xmlChar *_name, const xmlChar *_value __hwloc_attribute_unused)
{
  const char *name = (const char *) _name;

  fprintf(stderr, "ignoring unknown root attribute %s\n", name);
}

static void
hwloc__xml_import_object_attr(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj,
			      const xmlChar *_name, const xmlChar *_value)
{
  const char *name = (const char *) _name;
  const char *value = (const char *) _value;

  if (!strcmp(name, "type")) {
    /* already handled */
    return;
  }

  else if (!strcmp(name, "os_level"))
    obj->os_level = strtoul(value, NULL, 10);
  else if (!strcmp(name, "os_index"))
    obj->os_index = strtoul(value, NULL, 10);
  else if (!strcmp(name, "cpuset"))
    obj->cpuset = hwloc_cpuset_from_string(value);
  else if (!strcmp(name, "complete_cpuset"))
    obj->complete_cpuset = hwloc_cpuset_from_string(value);
  else if (!strcmp(name, "online_cpuset"))
    obj->online_cpuset = hwloc_cpuset_from_string(value);
  else if (!strcmp(name, "allowed_cpuset"))
    obj->allowed_cpuset = hwloc_cpuset_from_string(value);
  else if (!strcmp(name, "name"))
    obj->name = strdup(value);

  else if (!strcmp(name, "memory_kB")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.memory_kB = lvalue;
	break;
      case HWLOC_OBJ_NODE:
	obj->attr->node.memory_kB = lvalue;
	break;
      case HWLOC_OBJ_MACHINE:
	obj->attr->machine.memory_kB = lvalue;
	break;
      case HWLOC_OBJ_SYSTEM:
	obj->attr->system.memory_kB = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring memory_kB attribute for object type without memory\n");
	break;
    }
  }

  else if (!strcmp(name, "depth")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.depth = lvalue;
	break;
      case HWLOC_OBJ_MISC:
	obj->attr->misc.depth = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring depth attribute for object type without depth\n");
	break;
    }
  }

  else if (!strcmp(name, "huge_page_free")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_NODE:
	obj->attr->node.huge_page_free = lvalue;
	break;
      case HWLOC_OBJ_MACHINE:
	obj->attr->machine.huge_page_free = lvalue;
	break;
      case HWLOC_OBJ_SYSTEM:
	obj->attr->system.huge_page_free = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring huge_page_free attribute for object type without huge pages\n");
	break;
    }
  }

  else if (!strcmp(name, "huge_page_size_kB")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_MACHINE:
	obj->attr->machine.huge_page_size_kB = lvalue;
	break;
      case HWLOC_OBJ_SYSTEM:
	obj->attr->system.huge_page_size_kB = strtoul(value, NULL, 10);
	break;
      default:
	fprintf(stderr, "ignoring huge_page_size_kB attribute for object type without huge pages\n");
	break;
    }
  }

  else if (!strcmp(name, "dmi_board_vendor")) {
    switch (obj->type) {
      case HWLOC_OBJ_MACHINE:
	obj->attr->machine.dmi_board_vendor = strdup(value);
	break;
      default:
	fprintf(stderr, "ignoring dmi_board_vendor attribute for object type without DMI board\n");
	break;
    }
  }

  else if (!strcmp(name, "dmi_board_name")) {
    switch (obj->type) {
      case HWLOC_OBJ_MACHINE:
	obj->attr->machine.dmi_board_name = strdup(value);
	break;
      default:
	fprintf(stderr, "ignoring dmi_board_name attribute for object type without DMI board\n");
	break;
    }
  }

  else
    fprintf(stderr, "ignoring unknown object attribute %s\n", name);
}

static const xmlChar *
hwloc__xml_import_attr_value(xmlAttr *attr)
{
  xmlNode *subnode;
  /* use the first valid attribute content */
  for (subnode = attr->children; subnode; subnode = subnode->next) {
    if (subnode->type == XML_TEXT_NODE) {
      if (subnode->content && subnode->content[0] != '\0' && subnode->content[0] != '\n')
	return subnode->content;
    } else {
      fprintf(stderr, "ignoring unexpected xml attr node type %u\n", subnode->type);
    }
  }
  return NULL;
}

static void hwloc__xml_import_node(struct hwloc_topology *topology, struct hwloc_obj *parent, xmlNode *node, int depth);

static void
hwloc__xml_import_object_node(struct hwloc_topology *topology, struct hwloc_obj *parent, struct hwloc_obj *obj, xmlNode *node, int depth)
{
  xmlAttr *attr = NULL;

  /* first determine the object type */
  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE && !strcmp((const char*) attr->name, "type")) {
      const xmlChar *value = hwloc__xml_import_attr_value(attr);
      if (!value) {
	fprintf(stderr, "ignoring xml object without type attr %s\n", (const char*) value);
	return;
      }
      obj->type = hwloc_obj_type_of_string((const char*) value);
      if (obj->type == (hwloc_obj_type_t)-1) {
	fprintf(stderr, "ignoring unknown object type %s\n", (const char*) value);
	return;
      }
      break;
    } else {
      fprintf(stderr, "ignoring unexpected xml attr type %u\n", attr->type);
    }
  }
  if (obj->type == HWLOC_OBJ_TYPE_MAX) {
    fprintf(stderr, "ignoring object without type\n");
    return;
  }

  /* process attributes now that the type is known */
  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      const xmlChar *value = hwloc__xml_import_attr_value(attr);
      if (value)
	hwloc__xml_import_object_attr(topology, obj, attr->name, value);
      else
	fprintf(stderr, "ignoring unexpected xml object attr name `%s' with no value\n", attr->name);
    } else {
      fprintf(stderr, "ignoring unexpected xml object attr type %u\n", attr->type);
    }
  }

  if (depth > 0) { /* root object is already in place */
    /* add object */
    hwloc_insert_object_by_parent(topology, parent, obj);
  }

  /* process children */
  if (node->children)
    hwloc__xml_import_node(topology, obj, node->children, depth+1);
}

static void
hwloc__xml_import_node(struct hwloc_topology *topology, struct hwloc_obj *parent, xmlNode *node, int depth)
{
  for (; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE) {
      if (!strcmp((const char*) node->name, "object")) {
	/* object attributes */
	struct hwloc_obj *obj = NULL;
	if (depth)
	  obj = hwloc_alloc_setup_object(HWLOC_OBJ_TYPE_MAX, -1);
	else
	  obj = topology->levels[0][0];
	hwloc__xml_import_object_node(topology, parent, obj, node, depth);

      } else {
	/* unknown class */
	fprintf(stderr, "ignoring unexpected node class `%s'\n", (const char*) node->name);
	continue;
      }

    } else if (node->type == XML_TEXT_NODE) {
      if (node->content && node->content[0] != '\0' && node->content[0] != '\n')
	fprintf(stderr, "ignoring object text content %s\n", (const char*) node->content);
    } else {
      fprintf(stderr, "ignoring unexpected xml node type %u\n", node->type);
    }
  }
}

static void
hwloc__xml_import_topology_node(struct hwloc_topology *topology, xmlNode *node)
{
  xmlAttr *attr = NULL;

  if (strcmp((const char *) node->name, "topology") && strcmp((const char *) node->name, "root")) {
    /* root node should be in "topology" class (or "root" if importing from < 1.0) */
    fprintf(stderr, "ignoring object of class `%s' not at the top the xml hierarchy\n", (const char *) node->name);
    return;
  }

  /* process attributes */
  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      const xmlChar *value = hwloc__xml_import_attr_value(attr);
      if (value)
	hwloc__xml_import_topology_attr(topology, attr->name, value);
      else
	fprintf(stderr, "ignoring unexpected xml root attr name `%s' with no value\n", attr->name);
    } else {
      fprintf(stderr, "ignoring unexpected xml root attr type %u\n", attr->type);
    }
  }

  /* process children */
  if (node->children)
    hwloc__xml_import_node(topology, NULL, node->children, 0);
}

void
hwloc_look_xml(struct hwloc_topology *topology)
{
  xmlNode* root_node;
  xmlDtd *dtd;

  topology->support.discovery.proc = 1;

  dtd = xmlGetIntSubset((xmlDoc*) topology->backend_params.xml.doc);
  if (!dtd)
    fprintf(stderr, "Loading XML topology without DTD\n");
  else if (strcmp((char *) dtd->SystemID, "hwloc.dtd"))
    fprintf(stderr, "Loading XML topology with wrong DTD SystemID (%s instead of %s)\n",
	    (char *) dtd->SystemID, "hwloc.dtd");

  root_node = xmlDocGetRootElement((xmlDoc*) topology->backend_params.xml.doc);

  hwloc__xml_import_topology_node(topology, root_node);
  if (root_node->next)
    fprintf(stderr, "ignoring non-first root nodes\n");

  /* TODO: abort if we got an invalid topology or so */
}

/******************************
 ********* XML export *********
 ******************************/

static void
hwloc__xml_export_object (hwloc_topology_t topology, hwloc_obj_t obj, xmlNodePtr root_node)
{
  xmlNodePtr node = NULL;
  char *cpuset = NULL;
  char tmp[255];

  /* xmlNewChild() creates a new node, which is "attached" as child node
   * of root_node node. */
  node = xmlNewChild(root_node, NULL, BAD_CAST "object", NULL);
  xmlNewProp(node, BAD_CAST "type", BAD_CAST hwloc_obj_type_string(obj->type));
  sprintf(tmp, "%d", obj->os_level);
  xmlNewProp(node, BAD_CAST "os_level", BAD_CAST tmp);
  if (obj->os_index != (unsigned) -1) {
    sprintf(tmp, "%u", obj->os_index);
    xmlNewProp(node, BAD_CAST "os_index", BAD_CAST tmp);
  }
  if (obj->cpuset) {
    hwloc_cpuset_asprintf(&cpuset, obj->cpuset);
    xmlNewProp(node, BAD_CAST "cpuset", BAD_CAST cpuset);
    free(cpuset);
  }
  if (obj->complete_cpuset) {
    hwloc_cpuset_asprintf(&cpuset, obj->complete_cpuset);
    xmlNewProp(node, BAD_CAST "complete_cpuset", BAD_CAST cpuset);
    free(cpuset);
  }
  if (obj->online_cpuset) {
    hwloc_cpuset_asprintf(&cpuset, obj->online_cpuset);
    xmlNewProp(node, BAD_CAST "online_cpuset", BAD_CAST cpuset);
    free(cpuset);
  }
  if (obj->allowed_cpuset) {
    hwloc_cpuset_asprintf(&cpuset, obj->allowed_cpuset);
    xmlNewProp(node, BAD_CAST "allowed_cpuset", BAD_CAST cpuset);
    free(cpuset);
  }

  if (obj->name)
    xmlNewProp(node, BAD_CAST "name", BAD_CAST obj->name);

  switch (obj->type) {
  case HWLOC_OBJ_CACHE:
    sprintf(tmp, "%lu", obj->attr->cache.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%u", obj->attr->cache.depth);
    xmlNewProp(node, BAD_CAST "depth", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_SYSTEM:
    sprintf(tmp, "%lu", obj->attr->system.memory_kB);
    xmlNewProp(node, BAD_CAST "memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->system.huge_page_free);
    xmlNewProp(node, BAD_CAST "huge_page_free", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr->system.huge_page_size_kB);
    xmlNewProp(node, BAD_CAST "huge_page_size_kB", BAD_CAST tmp);
    break;
  case HWLOC_OBJ_MACHINE:
    if (obj->attr->machine.dmi_board_vendor)
      xmlNewProp(node, BAD_CAST "dmi_board_vendor", BAD_CAST obj->attr->machine.dmi_board_vendor);
    if (obj->attr->machine.dmi_board_name)
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
    unsigned x;
    for (x=0; x<obj->arity; x++)
      hwloc__xml_export_object (topology, obj->children[x], node);
  }
}

static void
hwloc__xml_export_topology_info (hwloc_topology_t topology __hwloc_attribute_unused, xmlNodePtr root_node __hwloc_attribute_unused)
{
}

void hwloc_topology_export_xml(hwloc_topology_t topology, const char *filename)
{
  xmlDocPtr doc = NULL;       /* document pointer */
  xmlNodePtr root_node = NULL; /* root pointer */
  xmlDtdPtr dtd = NULL;       /* DTD pointer */

  LIBXML_TEST_VERSION;

  /* Creates a new document, a node and set it as a root node. */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "topology");
  xmlDocSetRootElement(doc, root_node);

  /* Creates a DTD declaration. Isn't mandatory. */
  dtd = xmlCreateIntSubset(doc, BAD_CAST "topology", NULL, BAD_CAST "hwloc.dtd");

  hwloc__xml_export_object (topology, hwloc_get_root_obj(topology), root_node);

  hwloc__xml_export_topology_info (topology, root_node);

  /* Dumping document to stdio or file. */
  xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

  /* Free the document. */
  xmlFreeDoc(doc);

  /* Free the global variables that may have been allocated by the parser. */
  xmlCleanupParser();
}

#endif /* HWLOC_HAVE_XML */
