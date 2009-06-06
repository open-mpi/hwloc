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

#ifdef HAVE_XML

#include <topology.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "lstopo.h"

static void
output_topology (topo_topology_t topology, topo_obj_t obj, xmlNodePtr root_node, int verbose_mode)
{
  xmlNodePtr node = NULL;
  char name[255];
  char cpuset[TOPO_CPUSET_STRING_LENGTH+1];
  char tmp[255];

  sprintf(name, "%s", topo_obj_type_string(obj->type));

  /* xmlNewChild() creates a new node, which is "attached" as child node
   * of root_node node. */
  node = xmlNewChild(root_node, NULL,
		     BAD_CAST name,
		     NULL);
  xmlNewProp(node, BAD_CAST "type", BAD_CAST topo_obj_type_string(obj->type));
  sprintf(tmp, "%d", obj->physical_index);
  xmlNewProp(node, BAD_CAST "physical_index", BAD_CAST tmp);
  topo_cpuset_snprintf(cpuset, sizeof(cpuset), &obj->cpuset);
  xmlNewProp(node, BAD_CAST "cpuset", BAD_CAST cpuset);

  switch (obj->type) {
  case TOPO_OBJ_CACHE:
    sprintf(tmp, "%lu", obj->attr.cache.memory_kB);
    xmlNewProp(node, BAD_CAST "cache_memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%u", obj->attr.cache.depth);
    xmlNewProp(node, BAD_CAST "cache_depth", BAD_CAST tmp);
    break;
  case TOPO_OBJ_NODE:
  case TOPO_OBJ_MACHINE:
    sprintf(tmp, "%lu", obj->attr.node.memory_kB);
    xmlNewProp(node, BAD_CAST "node_memory_kB", BAD_CAST tmp);
    sprintf(tmp, "%lu", obj->attr.node.huge_page_free);
    xmlNewProp(node, BAD_CAST "node_huge_page_free", BAD_CAST tmp);
    break;
  case TOPO_OBJ_MISC:
    sprintf(tmp, "%u", obj->attr.misc.depth);
    xmlNewProp(node, BAD_CAST "misc_depth", BAD_CAST tmp);
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

void output_xml(topo_topology_t topology, const char *filename, int verbose_mode)
{
  xmlDocPtr doc = NULL;       /* document pointer */
  xmlNodePtr root_node = NULL; /* root pointer */
  xmlDtdPtr dtd = NULL;       /* DTD pointer */
  char tmp[255];
  struct topo_topology_info info;

  topo_topology_get_info(topology, &info);

  LIBXML_TEST_VERSION;

  /* Creates a new document, a node and set it as a root node. */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "root");
  xmlDocSetRootElement(doc, root_node);

  /* Creates a DTD declaration. Isn't mandatory. */
  dtd = xmlCreateIntSubset(doc, BAD_CAST "root", NULL, BAD_CAST "lstopo.dtd");

  output_topology (topology, topo_get_system_obj(topology), root_node, verbose_mode);

  xmlNewProp(root_node, BAD_CAST "dmi_board_vendor", BAD_CAST info.dmi_board_vendor);
  xmlNewProp(root_node, BAD_CAST "dmi_board_name", BAD_CAST info.dmi_board_name);
  sprintf(tmp, "%lu", info.huge_page_size_kB);
  xmlNewProp(root_node, BAD_CAST "huge_page_size_kB", BAD_CAST tmp);

  /* Dumping document to stdio or file. */
  xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

  /* Free the document. */
  xmlFreeDoc(doc);

  /* Free the global variables that may have been allocated by the parser. */
  xmlCleanupParser();
}

#endif /* HAVE_XML */
