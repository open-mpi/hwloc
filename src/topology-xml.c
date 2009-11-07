/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HAVE_XML

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <assert.h>

int
hwloc_backend_xml_init(struct hwloc_topology *topology, const char *xmlpath)
{
  xmlDoc *doc = NULL;

  assert(topology->backend_type == HWLOC_BACKEND_NONE);

  LIBXML_TEST_VERSION;

  doc = xmlReadFile(xmlpath, NULL, 0);
  if (!doc)
    return -1;

  /* TODO: warn if dtd is not hwloc.dtd? */

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

static void
hwloc__process_root_attr(struct hwloc_topology *topology,
			const xmlChar *_name, const xmlChar *_value)
{
  const char *name = (const char *) _name;
/* unused for now   const char *value = (const char *) _value; */

  fprintf(stderr, "ignoring unknown root attribute %s\n", name);
}

static void
hwloc__process_object_attr(struct hwloc_topology *topology, struct hwloc_obj *obj,
			  const xmlChar *_name, const xmlChar *_value)
{
  const char *name = (const char *) _name;
  const char *value = (const char *) _value;

  if (!strcmp(name, "type")) {
    /* already handled */
    return;
  }

  else if (!strcmp(name, "os_index"))
    obj->os_index = strtoul(value, NULL, 10);
  else if (!strcmp(name, "cpuset"))
    obj->cpuset = hwloc_cpuset_from_string(value);

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
      case HWLOC_OBJ_SYSTEM:
	obj->attr->system.dmi_board_vendor = strdup(value);
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
      case HWLOC_OBJ_SYSTEM:
	obj->attr->system.dmi_board_name = strdup(value);
	break;
      default:
	fprintf(stderr, "ignoring dmi_board_name attribute for object type without DMI board\n");
	break;
    }
  }

  else
    fprintf(stderr, "ignoring unknown object attribute %s\n", name);
}

static void
hwloc__look_xml_attr(struct hwloc_topology *topology, struct hwloc_obj *obj,
		    const xmlChar *attrname, xmlNode *node)
{
  /* use the first valid attribute content */
  for (; node; node = node->next) {
    if (node->type == XML_TEXT_NODE) {
      if (node->content && node->content[0] != '\0' && node->content[0] != '\n') {
	if (obj)
	  hwloc__process_object_attr(topology, obj, attrname, node->content);
	else
	  hwloc__process_root_attr(topology, attrname, node->content);
	break;
      }
    } else {
	fprintf(stderr, "ignoring unexpected xml attr node type %u name %s\n", node->type, (const char*) node->name);
    }
  }
}

static void
hwloc__look_xml_node(struct hwloc_topology *topology, xmlNode *node, int depth)
{
  for (; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE) {
      xmlAttr *attr = NULL;
      struct hwloc_obj *obj = NULL;

      if (depth == 0) {
	/* root node should be in root class */
	if (strcmp((const char *) node->name, "root"))
	  fprintf(stderr, "root node of class `%s' instead of `root'\n", (const char*) node->name);

	/* no object yet */

      } else {
	/* object node should be in object class */
	if (strcmp((const char*) node->name, "object"))
	  fprintf(stderr, "object node of class `%s' instead of `object'\n", (const char*) node->name);

	if (depth > 1)
	  obj = hwloc_alloc_setup_object(HWLOC_OBJ_TYPE_MAX, -1);
	else
	  obj = topology->levels[0][0];
      }

      /* first determine the object type */
      for (attr = node->properties; attr; attr = attr->next) {
	if (attr->type == XML_ATTRIBUTE_NODE) {
	  xmlNode *node;
	  for (node = attr->children; node; node = node->next) {
	    if (node->type == XML_TEXT_NODE) {
	      if (node->content && node->content[0] != '\0' && node->content[0] != '\n') {
		if (!strcmp((const char*) attr->name, "type")) {
		  obj->type = hwloc_obj_type_of_string((const char*) node->content);
		  if (obj->type == HWLOC_OBJ_TYPE_MAX)
		    fprintf(stderr, "ignoring unknown object type %s\n", (const char*) node->content);
		  else
		    break;
		}
	      }
	    } else {
		fprintf(stderr, "ignoring unexpected xml attr node type %u name %s\n", node->type, (const char*) node->name);
	    }
	  }
	  if (obj && obj->type == HWLOC_OBJ_TYPE_MAX) {
	    fprintf(stderr, "ignoring attributes of object without type\n");
	    return;
	  }
	} else {
	  fprintf(stderr, "ignoring unexpected xml attr type %u\n", attr->type);
	}
      }

      /* process attributes */
      for (attr = node->properties; attr; attr = attr->next) {
	if (attr->type == XML_ATTRIBUTE_NODE) {
	  if (attr->children)
	    hwloc__look_xml_attr(topology, obj, attr->name, attr->children);
	} else {
	  fprintf(stderr, "ignoring unexpected xml attr type %u\n", attr->type);
	}
      }

      if (depth == 0) {
	/* no object in xml doc root */
	assert (!obj);

      } else if (depth == 1) {
	/* system object is already there */
	if (obj->type != HWLOC_OBJ_SYSTEM) {
	  fprintf(stderr, "enforcing System type in top level instead of %s\n",
		  hwloc_obj_type_string(obj->type));
	  obj->type = HWLOC_OBJ_SYSTEM;
	}

      } else {
	/* add object */
	if (obj->type >= HWLOC_OBJ_TYPE_MAX) {
	  fprintf(stderr, "ignoring object with invalid type %u\n", obj->type);
	  free(obj);
	} else if (obj->type == HWLOC_OBJ_SYSTEM) {
	  fprintf(stderr, "ignoring system object at invalid depth %d\n", depth);
	  free(obj);
	} else {
	  if (!hwloc_cpuset_isincluded(obj->cpuset, topology->levels[0][0]->cpuset)) {
            char *s1 = hwloc_cpuset_printf_value(obj->cpuset), *s2 = hwloc_cpuset_printf_value(topology->levels[0][0]->cpuset);
	    fprintf(stderr, "ignoring object (cpuset %s) not covered by system (cpuset %s)\n", s1, s2);
            free(s1);
            free(s2);
          }
	  else
	    hwloc_add_object(topology, obj);
	}
      }

      /* process children */
      if (node->children)
	hwloc__look_xml_node(topology, node->children, depth+1);

    } else if (node->type == XML_TEXT_NODE) {
      if (node->content && node->content[0] != '\0' && node->content[0] != '\n')
	fprintf(stderr, "ignoring object text content %s\n", (const char*) node->content);      
    } else {
      fprintf(stderr, "ignoring unexpected xml node type %u\n", node->type);
    }
  }
}

void
hwloc_look_xml(struct hwloc_topology *topology)
{
  xmlNode* root_node;

  root_node = xmlDocGetRootElement((xmlDoc*) topology->backend_params.xml.doc);

  hwloc__look_xml_node(topology, root_node, 0);
}

#endif /* HAVE_XML */
