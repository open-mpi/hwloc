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

#include <config.h>
#include <topology.h>
#include <topology/helper.h>
#include <topology/debug.h>

#ifdef HAVE_XML

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <assert.h>

int
topo_backend_xml_init(struct topo_topology *topology, const char *xmlpath)
{
  xmlDoc *doc = NULL;

  assert(topology->backend_type == TOPO_BACKEND_NONE);

  LIBXML_TEST_VERSION;

  doc = xmlReadFile(xmlpath, NULL, 0);
  if (!doc)
    return -1;

  topology->backend_params.xml.doc = doc;
  topology->is_fake = 1;
  topology->backend_type = TOPO_BACKEND_XML;
  return 0;
}

void
topo_backend_xml_exit(struct topo_topology *topology)
{
  assert(topology->backend_type == TOPO_BACKEND_XML);
  xmlFreeDoc((xmlDoc*)topology->backend_params.xml.doc);
  xmlCleanupParser();
  topology->backend_type = TOPO_BACKEND_NONE;
}

static void
topo__process_root_attr(struct topo_topology *topology,
			const xmlChar *_name, const xmlChar *_value)
{
  const char *name = (const char *) _name;
  const char *value = (const char *) _value;

  if (!strcmp(name, "dmi_board_vendor"))
    topology->dmi_board_vendor = strdup(value);
  else if (!strcmp(name, "dmi_board_name"))
    topology->dmi_board_name = strdup(value);
  else if (!strcmp(name, "huge_page_size_kB"))
    topology->huge_page_size_kB = strtoul(value, NULL, 10);
  else
    fprintf(stderr, "ignoring unknown root attribute %s\n", name);
}

static void
topo__process_object_attr(struct topo_topology *topology, struct topo_obj *obj,
			  const xmlChar *_name, const xmlChar *_value)
{
  const char *name = (const char *) _name;
  const char *value = (const char *) _value;

  if (!strcmp(name, "type")) {
    obj->type = topo_obj_type_of_string(value);
    if (obj->type == TOPO_OBJ_TYPE_MAX)
      fprintf(stderr, "ignoring unknown object type %s\n", value);
  }

  else if (!strcmp(name, "physical_index"))
    obj->physical_index = strtoul(value, NULL, 10);
  else if (!strcmp(name, "cpuset"))
    topo_cpuset_from_string(value, &obj->cpuset);
  else if (!strcmp(name, "cache_memory_kB"))
    obj->attr.cache.memory_kB = strtoul(value, NULL, 10);
  else if (!strcmp(name, "cache_depth"))
    obj->attr.cache.depth = strtoul(value, NULL, 10);
  else if (!strcmp(name, "node_memory_kB"))
    obj->attr.node.memory_kB = strtoul(value, NULL, 10);
  else if (!strcmp(name, "node_huge_page_free"))
    obj->attr.node.huge_page_free = strtoul(value, NULL, 10);
  else if (!strcmp(name, "machine_memory_kB"))
    obj->attr.machine.memory_kB = strtoul(value, NULL, 10);
  else if (!strcmp(name, "machine_huge_page_free"))
    obj->attr.machine.huge_page_free = strtoul(value, NULL, 10);
  else if (!strcmp(name, "system_memory_kB"))
    obj->attr.system.memory_kB = strtoul(value, NULL, 10);
  else if (!strcmp(name, "system_huge_page_free"))
    obj->attr.system.huge_page_free = strtoul(value, NULL, 10);
  else if (!strcmp(name, "misc_depth"))
    obj->attr.misc.depth = strtoul(value, NULL, 10);

  else
    fprintf(stderr, "ignoring unknown object attribute %s\n", name);
}

static void
topo__look_xml_attr(struct topo_topology *topology, struct topo_obj *obj,
		    const xmlChar *attrname, xmlNode *node)
{
  /* use the first valid attribute content */
  for (; node; node = node->next) {
    if (node->type == XML_TEXT_NODE) {
      if (node->content && node->content[0] != '\0' && node->content[0] != '\n') {
	if (obj)
	  topo__process_object_attr(topology, obj, attrname, node->content);
	else
	  topo__process_root_attr(topology, attrname, node->content);
	break;
      }
    } else {
	fprintf(stderr, "ignoring unexpected xml attr node type %d name %s\n", node->type, node->name);
    }
  }
}

static void
topo__look_xml_node(struct topo_topology *topology, xmlNode *node, int depth)
{
  for (; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE) {
      xmlAttr *attr = NULL;
      struct topo_obj *obj = NULL;

      if (depth == 0) {
	/* root node should be in root class */
	if (strcmp((const char *) node->name, "root"))
	  fprintf(stderr, "root node of class `%s' instead of `root'\n", node->name);

	/* no object yet */

      } else {
	/* object node should be in object class */
	if (strcmp((const char*) node->name, "object"))
	  fprintf(stderr, "object node of class `%s' instead of `object'\n", node->name);

	if (depth > 1)
	  obj = topo_alloc_setup_object(TOPO_OBJ_TYPE_MAX, -1);
	else
	  obj = topology->levels[0][0];
      }

      /* process attributes */
      for (attr = node->properties; attr; attr = attr->next) {
	if (attr->type == XML_ATTRIBUTE_NODE) {
	  if (attr->children)
	    topo__look_xml_attr(topology, obj, attr->name, attr->children);
	} else {
	  fprintf(stderr, "ignoring unexpected xml attr type %d\n", node->type);
	}
      }

      if (depth == 0) {
	/* no object in xml doc root */
	assert (!obj);

      } else if (depth == 1) {
	/* system object is already there */
	if (obj->type != TOPO_OBJ_SYSTEM) {
	  fprintf(stderr, "enforcing System type in top level instead of %s\n",
		  topo_obj_type_string(obj->type));
	  obj->type = TOPO_OBJ_SYSTEM;
	}

      } else {
	/* add object */
	if (obj->type == TOPO_OBJ_TYPE_MAX) {
	  fprintf(stderr, "ignoring object with invalid type %d\n", obj->type);
	  free(obj);
	} else if (obj->type == TOPO_OBJ_SYSTEM) {
	  fprintf(stderr, "ignoring system object at invalid depth %d\n", depth);
	  free(obj);
	} else {
	  if (!topo_cpuset_isincluded(&obj->cpuset, &topology->levels[0][0]->cpuset))
	    fprintf(stderr, "ignoring object (cpuset %s) not covered by system (cpuset %s)\n",
		    TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset),
		    TOPO_CPUSET_PRINTF_VALUE(&topology->levels[0][0]->cpuset));
	  else
	    topo_add_object(topology, obj);
	}
      }

      /* process children */
      if (node->children)
	topo__look_xml_node(topology, node->children, depth+1);

    } else if (node->type == XML_TEXT_NODE) {
      if (node->content && node->content[0] != '\0' && node->content[0] != '\n')
	fprintf(stderr, "ignoring object text content %s\n", node->content);      
    } else {
      fprintf(stderr, "ignoring unexpected xml node type %d\n", node->type);
    }
  }
}

void
topo_look_xml(struct topo_topology *topology)
{
  xmlNode* root_node;

  root_node = xmlDocGetRootElement((xmlDoc*) topology->backend_params.xml.doc);

  topo__look_xml_node(topology, root_node, 0);
}

#endif /* HAVE_XML */
