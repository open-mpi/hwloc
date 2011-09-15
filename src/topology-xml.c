/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2011 INRIA.  All rights reserved.
 * Copyright © 2009-2011 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <assert.h>
#include <strings.h>

#ifdef HWLOC_HAVE_XML
#include <libxml/parser.h>
#include <libxml/tree.h>
#endif

#ifdef HWLOC_HAVE_XML
static void hwloc_libxml2_error_callback(void * ctx __hwloc_attribute_unused, const char * msg __hwloc_attribute_unused, ...) { /* do nothing */ }

static void
hwloc_libxml2_disable_stderrwarnings(void)
{
  static int first = 1;
  if (first) {
    static int verbose = 0;
    char *env = getenv("HWLOC_XML_VERBOSE");
    if (env)
      verbose = atoi(env);
    xmlSetGenericErrorFunc(NULL, verbose ? xmlGenericError : hwloc_libxml2_error_callback);
    first = 0;
  }
}
#endif

/***********************************
 ******** Backend Init/Exit ********
 ***********************************/

/* this can be the first XML call */
int
hwloc_backend_xml_init(struct hwloc_topology *topology, const char *xmlpath, const char *xmlbuffer, int xmlbuflen)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_IMPORT");
  if (!env || !atoi(env)) {
    xmlDoc *doc = NULL;

    LIBXML_TEST_VERSION;
    hwloc_libxml2_disable_stderrwarnings();

    errno = 0; /* set to 0 so that we know if libxml2 changed it */

    if (xmlpath)
      doc = xmlReadFile(xmlpath, NULL, 0);
    else if (xmlbuffer)
      doc = xmlReadMemory(xmlbuffer, xmlbuflen, "", NULL, 0);

    if (!doc) {
      if (!errno)
	/* libxml2 read the file fine, but it got an error during parsing */
      errno = EINVAL;
      return -1;
    }

    topology->backend_params.xml.buffer = NULL;
    topology->backend_params.xml.doc = doc;
  } else
#endif /* HWLOC_HAVE_XML */
  if (xmlbuffer) {
    topology->backend_params.xml.buffer = malloc(xmlbuflen);
    memcpy(topology->backend_params.xml.buffer, xmlbuffer, xmlbuflen);
  } else {
    FILE * file;
    size_t buflen = 4096, offset, readlen;
    char *buffer = malloc(buflen+1);
    size_t ret;

    if (!strcmp(xmlpath, "-"))
      xmlpath = "/dev/stdin";

    file = fopen(xmlpath, "r");
    if (!file)
      return -1;

    offset = 0; readlen = buflen;
    while (1) {
      ret = fread(buffer+offset, 1, readlen, file);

      offset += ret;
      buffer[offset] = 0;

      if (ret != readlen)
        break;

      buflen *= 2;
      buffer = realloc(buffer, buflen+1);
      readlen = buflen/2;
    }

    fclose(file);

    topology->backend_params.xml.buffer = buffer;
    buflen = offset+1;
  }

  topology->is_thissystem = 0;
  assert(topology->backend_type == HWLOC_BACKEND_NONE);
  topology->backend_type = HWLOC_BACKEND_XML;

  topology->backend_params.xml.first_distances = topology->backend_params.xml.last_distances = NULL;

  return 0;
}

/* this canNOT be the first XML call */
void
hwloc_backend_xml_exit(struct hwloc_topology *topology)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_IMPORT");
  if (!env || !atoi(env)) {
    xmlFreeDoc((xmlDoc*)topology->backend_params.xml.doc);
  } else
#endif
  {
    assert(topology->backend_params.xml.buffer);
    free(topology->backend_params.xml.buffer);
  }
  assert(topology->backend_type == HWLOC_BACKEND_XML);
  topology->backend_type = HWLOC_BACKEND_NONE;
}

/************************************************
 ********* XML import (common routines) *********
 ************************************************/

static void
hwloc__xml_import_object_attr(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj,
			      const char *name, const char *value)
{
  if (!strcmp(name, "type")) {
    /* already handled */
    return;
  }

  else if (!strcmp(name, "os_level"))
    obj->os_level = strtoul(value, NULL, 10);
  else if (!strcmp(name, "os_index"))
    obj->os_index = strtoul(value, NULL, 10);
  else if (!strcmp(name, "cpuset")) {
    obj->cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->cpuset, value);
  } else if (!strcmp(name, "complete_cpuset")) {
    obj->complete_cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->complete_cpuset,value);
  } else if (!strcmp(name, "online_cpuset")) {
    obj->online_cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->online_cpuset, value);
  } else if (!strcmp(name, "allowed_cpuset")) {
    obj->allowed_cpuset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->allowed_cpuset, value);
  } else if (!strcmp(name, "nodeset")) {
    obj->nodeset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->nodeset, value);
  } else if (!strcmp(name, "complete_nodeset")) {
    obj->complete_nodeset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->complete_nodeset, value);
  } else if (!strcmp(name, "allowed_nodeset")) {
    obj->allowed_nodeset = hwloc_bitmap_alloc();
    hwloc_bitmap_sscanf(obj->allowed_nodeset, value);
  } else if (!strcmp(name, "name"))
    obj->name = strdup(value);

  else if (!strcmp(name, "cache_size")) {
    unsigned long long lvalue = strtoull(value, NULL, 10);
    if (obj->type == HWLOC_OBJ_CACHE)
      obj->attr->cache.size = lvalue;
    else
      fprintf(stderr, "ignoring cache_size attribute for non-cache object type\n");
  }

  else if (!strcmp(name, "cache_linesize")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    if (obj->type == HWLOC_OBJ_CACHE)
      obj->attr->cache.linesize = lvalue;
    else
      fprintf(stderr, "ignoring cache_linesize attribute for non-cache object type\n");
  }

  else if (!strcmp(name, "cache_associativity")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    if (obj->type == HWLOC_OBJ_CACHE)
      obj->attr->cache.associativity = lvalue;
    else
      fprintf(stderr, "ignoring cache_associativity attribute for non-cache object type\n");
  }

  else if (!strcmp(name, "local_memory"))
    obj->memory.local_memory = strtoull(value, NULL, 10);

  else if (!strcmp(name, "depth")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.depth = lvalue;
	break;
      case HWLOC_OBJ_GROUP:
	obj->attr->group.depth = lvalue;
	break;
      case HWLOC_OBJ_BRIDGE:
	obj->attr->bridge.depth = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring depth attribute for object type without depth\n");
	break;
    }
  }

  else if (!strcmp(name, "pci_busid")) {
    switch (obj->type) {
    case HWLOC_OBJ_PCI_DEVICE:
    case HWLOC_OBJ_BRIDGE: {
      unsigned domain, bus, dev, func;
      if (sscanf(value, "%04x:%02x:%02x.%01x",
		 &domain, &bus, &dev, &func) != 4) {
	fprintf(stderr, "ignoring invalid pci_busid format string %s\n", value);
      } else {
	obj->attr->pcidev.domain = domain;
	obj->attr->pcidev.bus = bus;
	obj->attr->pcidev.dev = dev;
	obj->attr->pcidev.func = func;
      }
      break;
    }
    default:
      fprintf(stderr, "ignoring pci_busid attribute for non-PCI object\n");
      break;
    }
  }

  else if (!strcmp(name, "pci_type")) {
    switch (obj->type) {
    case HWLOC_OBJ_PCI_DEVICE:
    case HWLOC_OBJ_BRIDGE: {
      unsigned classid, vendor, device, subvendor, subdevice, revision;
      if (sscanf(value, "%04x [%04x:%04x] [%04x:%04x] %02x",
		 &classid, &vendor, &device, &subvendor, &subdevice, &revision) != 6) {
	fprintf(stderr, "ignoring invalid pci_type format string %s\n", value);
      } else {
	obj->attr->pcidev.class_id = classid;
	obj->attr->pcidev.vendor_id = vendor;
	obj->attr->pcidev.device_id = device;
	obj->attr->pcidev.subvendor_id = subvendor;
	obj->attr->pcidev.subdevice_id = subdevice;
	obj->attr->pcidev.revision = revision;
      }
      break;
    }
    default:
      fprintf(stderr, "ignoring pci_type attribute for non-PCI object\n");
      break;
    }
  }

  else if (!strcmp(name, "pci_link_speed")) {
    switch (obj->type) {
    case HWLOC_OBJ_PCI_DEVICE:
    case HWLOC_OBJ_BRIDGE: {
      obj->attr->pcidev.linkspeed = atof(value);
      break;
    }
    default:
      fprintf(stderr, "ignoring pci_link_speed attribute for non-PCI object\n");
      break;
    }
  }

  else if (!strcmp(name, "bridge_type")) {
    switch (obj->type) {
    case HWLOC_OBJ_BRIDGE: {
      unsigned upstream_type, downstream_type;
      if (sscanf(value, "%u-%u", &upstream_type, &downstream_type) != 2)
	fprintf(stderr, "ignoring invalid bridge_type format string %s\n", value);
      else {
	obj->attr->bridge.upstream_type = upstream_type;
	obj->attr->bridge.downstream_type = downstream_type;
      };
      break;
    }
    default:
      fprintf(stderr, "ignoring bridge_type attribute for non-bridge object\n");
      break;
    }
  }

  else if (!strcmp(name, "bridge_pci")) {
    switch (obj->type) {
    case HWLOC_OBJ_BRIDGE: {
      unsigned domain, secbus, subbus;
      if (sscanf(value, "%04x:[%02x-%02x]",
		 &domain, &secbus, &subbus) != 3) {
	fprintf(stderr, "ignoring invalid bridge_pci format string %s\n", value);
      } else {
	obj->attr->bridge.downstream.pci.domain = domain;
	obj->attr->bridge.downstream.pci.secondary_bus = secbus;
	obj->attr->bridge.downstream.pci.subordinate_bus = subbus;
      }
      break;
    }
    default:
      fprintf(stderr, "ignoring bridge_pci attribute for non-bridge object\n");
      break;
    }
  }

  else if (!strcmp(name, "osdev_type")) {
    switch (obj->type) {
    case HWLOC_OBJ_OS_DEVICE: {
      unsigned osdev_type;
      if (sscanf(value, "%u", &osdev_type) != 1)
	fprintf(stderr, "ignoring invalid osdev_type format string %s\n", value);
      else
	obj->attr->osdev.type = osdev_type;
      break;
    }
    default:
      fprintf(stderr, "ignoring osdev_type attribute for non-osdev object\n");
      break;
    }
  }




  /*************************
   * deprecated (from 1.0)
   */
  else if (!strcmp(name, "dmi_board_vendor")) {
    hwloc_obj_add_info(obj, "DMIBoardVendor", strdup(value));
  }
  else if (!strcmp(name, "dmi_board_name")) {
    hwloc_obj_add_info(obj, "DMIBoardName", strdup(value));
  }

  /*************************
   * deprecated (from 0.9)
   */
  else if (!strcmp(name, "memory_kB")) {
    unsigned long long lvalue = strtoull(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.size = lvalue << 10;
	break;
      case HWLOC_OBJ_NODE:
      case HWLOC_OBJ_MACHINE:
      case HWLOC_OBJ_SYSTEM:
	obj->memory.local_memory = lvalue << 10;
	break;
      default:
	fprintf(stderr, "ignoring memory_kB attribute for object type without memory\n");
	break;
    }
  }
  else if (!strcmp(name, "huge_page_size_kB")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_NODE:
      case HWLOC_OBJ_MACHINE:
      case HWLOC_OBJ_SYSTEM:
	if (!obj->memory.page_types) {
	  obj->memory.page_types = malloc(sizeof(*obj->memory.page_types));
	  obj->memory.page_types_len = 1;
	}
	obj->memory.page_types[0].size = lvalue << 10;
	break;
      default:
	fprintf(stderr, "ignoring huge_page_size_kB attribute for object type without huge pages\n");
	break;
    }
  }
  else if (!strcmp(name, "huge_page_free")) {
    unsigned long lvalue = strtoul(value, NULL, 10);
    switch (obj->type) {
      case HWLOC_OBJ_NODE:
      case HWLOC_OBJ_MACHINE:
      case HWLOC_OBJ_SYSTEM:
	if (!obj->memory.page_types) {
	  obj->memory.page_types = malloc(sizeof(*obj->memory.page_types));
	  obj->memory.page_types_len = 1;
	}
	obj->memory.page_types[0].count = lvalue;
	break;
      default:
	fprintf(stderr, "ignoring huge_page_free attribute for object type without huge pages\n");
	break;
    }
  }
  /*
   * end of deprecated (from 0.9)
   *******************************/



  else
    fprintf(stderr, "ignoring unknown object attribute %s\n", name);
}

/************************************************
 ********* XML import (libxml2 routines) ********
 ************************************************/

#ifdef HWLOC_HAVE_XML
static void hwloc__libxml_import_node(struct hwloc_topology *topology, struct hwloc_obj *parent, xmlNode *node, int depth);

static const xmlChar *
hwloc__libxml_import_attr_value(xmlAttr *attr)
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

static void
hwloc__libxml_import_object_node(struct hwloc_topology *topology, struct hwloc_obj *parent, struct hwloc_obj *obj, xmlNode *node, int depth)
{
  xmlAttr *attr = NULL;

  /* first determine the object type */
  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE && !strcmp((const char*) attr->name, "type")) {
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
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
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
      if (value)
	hwloc__xml_import_object_attr(topology, obj, (const char*) attr->name, (const char *) value);
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
    hwloc__libxml_import_node(topology, obj, node->children, depth+1);
}

static void
hwloc__libxml_import_pagetype_node(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj, xmlNode *node)
{
  uint64_t size = 0, count = 0;
  xmlAttr *attr = NULL;

  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
      if (value) {
	if (!strcmp((char *) attr->name, "size"))
	  size = strtoul((char *) value, NULL, 10);
	else if (!strcmp((char *) attr->name, "count"))
	  count = strtoul((char *) value, NULL, 10);
	else
	  fprintf(stderr, "ignoring unknown pagetype attribute %s\n", (char *) attr->name);
      }
    } else {
      fprintf(stderr, "ignoring unexpected xml pagetype attr type %u\n", attr->type);
    }
  }

  if (size) {
    int idx = obj->memory.page_types_len;
    obj->memory.page_types = realloc(obj->memory.page_types, (idx+1)*sizeof(*obj->memory.page_types));
    obj->memory.page_types_len = idx+1;
    obj->memory.page_types[idx].size = size;
    obj->memory.page_types[idx].count = count;
  } else
    fprintf(stderr, "ignoring pagetype attribute without size\n");
}

static void
hwloc__libxml_import_distances_node(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj, xmlNode *node)
{
  unsigned long reldepth = 0, nbobjs = 0;
  float latbase = 0;
  xmlAttr *attr = NULL;
  xmlNode *subnode;

  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
      if (value) {
	if (!strcmp((char *) attr->name, "nbobjs"))
	  nbobjs = strtoul((char *) value, NULL, 10);
	else if (!strcmp((char *) attr->name, "relative_depth"))
	  reldepth = strtoul((char *) value, NULL, 10);
	else if (!strcmp((char *) attr->name, "latency_base"))
          latbase = (float) atof((char *) value);
	else
	  fprintf(stderr, "ignoring unknown distances attribute %s\n", (char *) attr->name);
      } else
	fprintf(stderr, "ignoring unexpected xml distances attr name `%s' with no value\n", (const char*) attr->name);
    } else {
      fprintf(stderr, "ignoring unexpected xml distances attr type %u\n", attr->type);
    }
  }

  if (nbobjs && reldepth && latbase) {
    unsigned nbcells, i;
    float *matrix, latmax = 0;
    struct hwloc_xml_imported_distances_s *distances;

    nbcells = 0;
    if (node->children)
      for(subnode = node->children; subnode; subnode = subnode->next)
 	if (subnode->type == XML_ELEMENT_NODE)
	  nbcells++;
    if (nbcells != nbobjs*nbobjs) {
      fprintf(stderr, "ignoring distances with %u cells instead of %lu\n", nbcells, nbobjs*nbobjs);
      return;
    }

    distances = malloc(sizeof(*distances));
    distances->root = obj;
    distances->distances.relative_depth = reldepth;
    distances->distances.nbobjs = nbobjs;
    distances->distances.latency = matrix = malloc(nbcells*sizeof(float));
    distances->distances.latency_base = latbase;

    i = 0;
    for(subnode = node->children; subnode; subnode = subnode->next)
      if (subnode->type == XML_ELEMENT_NODE) {
	/* read one cell */
	for (attr = subnode->properties; attr; attr = attr->next)
	  if (attr->type == XML_ATTRIBUTE_NODE) {
	    const xmlChar *value = hwloc__libxml_import_attr_value(attr);
	    if (value) {
	      if (!strcmp((char *) attr->name, "value")) {
                float val = (float) atof((char *) value);
		matrix[i] = val;
		if (val > latmax)
		  latmax = val;
	      } else
		fprintf(stderr, "ignoring unknown distance attribute %s\n", (char *) attr->name);
	    } else
	      fprintf(stderr, "ignoring unexpected xml distance attr name `%s' with no value\n", (const char*) attr->name);
	  } else {
	    fprintf(stderr, "ignoring unexpected xml distance attr type %u\n", attr->type);
	  }
	/* next matrix cell */
	i++;
      }
    distances->distances.latency_max = latmax;

    if (topology->backend_params.xml.last_distances)
      topology->backend_params.xml.last_distances->next = distances;
    else
      topology->backend_params.xml.first_distances = distances;
    distances->prev = topology->backend_params.xml.last_distances;
    distances->next = NULL;
  }
}

static void
hwloc__libxml_import_info_node(struct hwloc_topology *topology __hwloc_attribute_unused, struct hwloc_obj *obj, xmlNode *node)
{
  char *infoname = NULL;
  char *infovalue = NULL;
  xmlAttr *attr = NULL;

  for (attr = node->properties; attr; attr = attr->next) {
    if (attr->type == XML_ATTRIBUTE_NODE) {
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
      if (value) {
	if (!strcmp((char *) attr->name, "name"))
	  infoname = (char *) value;
	else if (!strcmp((char *) attr->name, "value"))
	  infovalue = (char *) value;
	else
	  fprintf(stderr, "ignoring unknown info attribute %s\n", (char *) attr->name);
      }
    } else {
      fprintf(stderr, "ignoring unexpected xml info attr type %u\n", attr->type);
    }
  }

  if (infoname)
    /* empty strings are ignored by libxml */
    hwloc_obj_add_info(obj, infoname, infovalue ? infovalue : "");
  else
    fprintf(stderr, "ignoring info attribute without name\n");
}

static void
hwloc__libxml_import_node(struct hwloc_topology *topology, struct hwloc_obj *parent, xmlNode *node, int depth)
{
  for (; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE) {
      if (!strcmp((const char*) node->name, "object")) {
	/* object attributes */
	struct hwloc_obj *obj;
	if (depth)
	  obj = hwloc_alloc_setup_object(HWLOC_OBJ_TYPE_MAX, -1);
	else
	  obj = topology->levels[0][0];
	hwloc__libxml_import_object_node(topology, parent, obj, node, depth);

      } else if (!strcmp((const char*) node->name, "page_type")) {
	hwloc__libxml_import_pagetype_node(topology, parent, node);

      } else if (!strcmp((const char*) node->name, "info")) {
	hwloc__libxml_import_info_node(topology, parent, node);

      } else if (!strcmp((const char*) node->name, "distances")) {
	hwloc__libxml_import_distances_node(topology, parent, node);

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
hwloc__libxml_import_topology_node(struct hwloc_topology *topology, xmlNode *node)
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
      const xmlChar *value = hwloc__libxml_import_attr_value(attr);
      if (value) {
	fprintf(stderr, "ignoring unknown root attribute %s\n", (char *) attr->name);
      }
    } else {
      fprintf(stderr, "ignoring unexpected xml root attr type %u\n", attr->type);
    }
  }

  /* process children */
  if (node->children)
    hwloc__libxml_import_node(topology, NULL, node->children, 0);
}
#endif /* HWLOC_HAVE_XML */

/***************************************************
 ********* XML import (NO-libxml2 routines) ********
 ***************************************************/

/* skip spaces until the next interesting char */
static char *
hwloc__nolibxml_import_ignore_spaces(char *buffer)
{
  return buffer + strspn(buffer, " \t\n");
}

/* return the name of the next attribute,
 * a pointer to its value,
 * and a pointer to what's following
 */
static char *
hwloc__nolibxml_import_find_attr(char *buffer, char **valuep, char **remainingp)
{
  int namelen;
  size_t len, escaped;
  char *value, *end;

  /* find the beginning of an attribute */
  buffer = hwloc__nolibxml_import_ignore_spaces(buffer);
  namelen = strspn(buffer, "abcdefghijklmnopqrstuvwxyz_");
  if (buffer[namelen] != '=' || buffer[namelen+1] != '\"')
    return NULL;
  buffer[namelen] = '\0';

  /* find the beginning of its value, and unescape it */
  *valuep = value = buffer+namelen+2;
  len = 0; escaped = 0;
  while (value[len+escaped] != '\"') {
    if (value[len+escaped] == '&') {
      if (!strcmp(&value[1+len+escaped], "#10;")) {
	escaped += 4;
	value[1+len] = '\n';
      } else if (!strcmp(&value[1+len+escaped], "#13;")) {
	escaped += 4;
	value[1+len] = '\r';
      } else if (!strcmp(&value[1+len+escaped], "#9;")) {
	escaped += 3;
	value[1+len] = '\t';
      } else if (!strcmp(&value[1+len+escaped], "quot;")) {
	escaped += 5;
	value[1+len] = '\"';
      } else if (!strcmp(&value[1+len+escaped], "lt;")) {
	escaped += 3;
	value[1+len] = '<';
      } else if (!strcmp(&value[1+len+escaped], "gt;")) {
	escaped += 3;
	value[1+len] = '>';
      } else if (!strcmp(&value[1+len+escaped], "amp;")) {
	escaped += 4;
	value[1+len] = '&';
      } else {
	return NULL;
      }
      value[len] = value[len+escaped];
    }
    len++;
    if (value[len+escaped] == '\0')
      return NULL;
  }
  value[len] = '\0';

  /* find next attribute */
  end = &value[len+escaped+1]; /* skip the ending " */
  *remainingp = hwloc__nolibxml_import_ignore_spaces(end);

  return buffer;
}

/* return the name of the next tag,
 * a pointer to its attribute string,
 * and a pointer to what's following (either children or brothers).
 */
static char *
hwloc__nolibxml_import_find_tag(char *buffer, char **attr, char **remaining, int *closed)
{
  char *end;

  /* find the beginning of the tag */
  buffer = hwloc__nolibxml_import_ignore_spaces(buffer);
  if (buffer[0] != '<')
    return NULL;
  buffer++;

  /* find the end, mark it and return it */
  end = strchr(buffer, '>');
  if (!end)
    return NULL;
  end[0] = '\0';
  *remaining = end+1;

  /* handle auto-closing tags */
  if (end[-1] == '/') {
    *closed = 1;
    end[-1] = '\0';
  } else
    *closed = 0;

  /* find attributes */
  if (buffer[0] == '/') {
    /* closing tags have no attributes */
    *attr = NULL;
  } else {
    /* look right after the tag name */
    int namelen = strspn(buffer, "abcdefghijklmnopqrstuvwxyz_");
    if (buffer[namelen] == '\0') {
      /* found the (previously marked) end of the tag, there are no attributes */
      *attr = NULL;
    } else if (buffer[namelen] == ' ') {
      /* found a space, likely starting attributes */
      buffer[namelen] = '\0';
      *attr = buffer+namelen+1;
    } else
      return NULL;
  }

  return buffer;
}

/* handle the page_type, its contents, and return what's next */
static char *
hwloc__nolibxml_import_page_type(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_obj_t obj,
				 char *attr, char *remaining, int closed)
{
  uint64_t size = 0, count = 0;
  char *tag;

  while (1) {
    char *attrname, *attrvalue;
    attrname = hwloc__nolibxml_import_find_attr(attr, &attrvalue, &attr);
    if (!attrname)
      break;
    if (!strcmp(attrname, "size"))
      size = strtoul(attrvalue, NULL, 10);
    else if (!strcmp(attrname, "count"))
      count = strtoul(attrvalue, NULL, 10);
    else
      return NULL;
  }
  if (size) {
    int idx = obj->memory.page_types_len;
    obj->memory.page_types = realloc(obj->memory.page_types, (idx+1)*sizeof(*obj->memory.page_types));
    obj->memory.page_types_len = idx+1;
    obj->memory.page_types[idx].size = size;
    obj->memory.page_types[idx].count = count;
  }

  if (closed)
    return remaining;

  tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
  if (!tag || strcmp(tag, "/page_type"))
    return NULL;
  return remaining;
}

/* handle the info, its contents, and return what's next */
static char *
hwloc__nolibxml_import_info(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_obj_t obj,
			    char *attr, char *remaining, int closed)
{
  char *infoname = NULL;
  char *infovalue = NULL;
  char *tag;

  while (1) {
    char *attrname, *attrvalue;
    attrname = hwloc__nolibxml_import_find_attr(attr, &attrvalue, &attr);
    if (!attrname)
      break;
    if (!strcmp(attrname, "name"))
      infoname = attrvalue;
    else if (!strcmp(attrname, "value"))
      infovalue = attrvalue;
    else
      return NULL;
  }
  if (infoname)
    /* empty strings are ignored by libxml */
    hwloc_obj_add_info(obj, infoname, infovalue ? infovalue : "");

  if (closed)
    return remaining;

  tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
  if (!tag || strcmp(tag, "/info"))
    return NULL;
  return remaining;
}

/* handle the distances, its contents, and return what's next */
static char *
hwloc__nolibxml_import_distances(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_obj_t obj,
				 char *attr, char *remaining, int closed)
{
  unsigned long reldepth = 0, nbobjs = 0;
  float latbase = 0;
  char *tag;

  while (1) {
    char *attrname, *attrvalue;
    attrname = hwloc__nolibxml_import_find_attr(attr, &attrvalue, &attr);
    if (!attrname)
      break;
    if (!strcmp(attrname, "nbobjs"))
      nbobjs = strtoul(attrvalue, NULL, 10);
    else if (!strcmp(attrname, "relative_depth"))
      reldepth = strtoul(attrvalue, NULL, 10);
    else if (!strcmp(attrname, "latency_base"))
      latbase = (float) atof(attrvalue);
    else
      return NULL;
  }

  if (closed)
    /* latency children are needed */
    return NULL;

  if (nbobjs && reldepth && latbase) {
    unsigned i;
    float *matrix, latmax = 0;
    struct hwloc_xml_imported_distances_s *distances;

    distances = malloc(sizeof(*distances));
    distances->root = obj;
    distances->distances.relative_depth = reldepth;
    distances->distances.nbobjs = nbobjs;
    distances->distances.latency = matrix = malloc(nbobjs*nbobjs*sizeof(float));
    distances->distances.latency_base = latbase;

    for(i=0; i<nbobjs*nbobjs; i++) {
      char *attrname, *attrvalue;
      float val;
      tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
      if (!tag || strcmp(tag, "latency"))
	return NULL;
      attrname = hwloc__nolibxml_import_find_attr(attr, &attrvalue, &attr);
      if (!attrname || strcmp(attrname, "value"))
	return NULL;
      val = (float) atof((char *) attrvalue);
      matrix[i] = val;
      if (val > latmax)
	latmax = val;
      if (closed)
	continue;
      tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
      if (!tag || strcmp(tag, "/latency"))
	return NULL;
      }
    distances->distances.latency_max = latmax;

    if (topology->backend_params.xml.last_distances)
      topology->backend_params.xml.last_distances->next = distances;
    else
      topology->backend_params.xml.first_distances = distances;
    distances->prev = topology->backend_params.xml.last_distances;
    distances->next = NULL;
  }

  tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
  if (!tag || strcmp(tag, "/distances"))
    return NULL;
  return remaining;
}

/* handle the object, its contents, and return what's next */
static char *
hwloc__nolibxml_import_object(hwloc_topology_t topology, hwloc_obj_t obj,
			      char *attr, char *remaining, int closed)
{
  char *tag;

  while (1) {
    char *attrname, *attrvalue;
    attrname = hwloc__nolibxml_import_find_attr(attr, &attrvalue, &attr);
    if (!attrname)
      break;
    if (!strcmp(attrname, "type")) {
      obj->type = hwloc_obj_type_of_string(attrvalue);
      if (obj->type == (hwloc_obj_type_t)-1)
        return NULL;
    } else {
      /* type needed first */
      if (obj->type == (hwloc_obj_type_t)-1)
        return NULL;
      hwloc__xml_import_object_attr(topology, obj, attrname, attrvalue);
    }
  }

  if (closed)
    return remaining;

  while (1) {
    tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
    if (!tag)
      return NULL;
    else if (!strcmp(tag, "object")) {
      hwloc_obj_t childobj = hwloc_alloc_setup_object(HWLOC_OBJ_TYPE_MAX, -1);
      hwloc_insert_object_by_parent(topology, obj, childobj);
      remaining = hwloc__nolibxml_import_object(topology, childobj, attr, remaining, closed);
    }
    else if (!strcmp(tag, "page_type"))
      remaining = hwloc__nolibxml_import_page_type(topology, obj, attr, remaining, closed);
    else if (!strcmp(tag, "info"))
      remaining = hwloc__nolibxml_import_info(topology, obj, attr, remaining, closed);
    else if (!strcmp(tag, "distances"))
      remaining = hwloc__nolibxml_import_distances(topology, obj, attr, remaining, closed);
    else if (!strcmp(tag, "/object"))
      break;
    else
      return NULL;

    if (!remaining)
      return NULL;
  }

  return remaining;
}


/***********************************
 ********* main XML import *********
 ***********************************/

static void
hwloc_xml__handle_distances(struct hwloc_topology *topology)
{
  struct hwloc_xml_imported_distances_s *xmldist, *next = topology->backend_params.xml.first_distances;

  if (!next)
    return;

  /* connect things now because we need levels to check/build, they'll be reconnected properly later anyway */
  hwloc_connect_children(topology->levels[0][0]);
  hwloc_connect_levels(topology);

  while ((xmldist = next) != NULL) {
    hwloc_obj_t root = xmldist->root;
    unsigned depth = root->depth + xmldist->distances.relative_depth;
    unsigned nbobjs = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, root->cpuset, depth);
    if (nbobjs != xmldist->distances.nbobjs) {
      /* distances invalid, drop */
      fprintf(stderr, "ignoring invalid distance matrix with %u objs instead of %u\n",
	     xmldist->distances.nbobjs, nbobjs);
      free(xmldist->distances.latency);
    } else {
      /* distances valid, add it to the internal OS distances list for grouping */
      unsigned *indexes = malloc(nbobjs * sizeof(unsigned));
      hwloc_obj_t child, *objs = malloc(nbobjs * sizeof(hwloc_obj_t));
      unsigned j;
      for(j=0, child = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, depth, NULL);
	  j<nbobjs;
	  j++, child = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, depth, child)) {
	indexes[j] = child->os_index;
	objs[j] = child;
      }
      for(j=0; j<nbobjs*nbobjs; j++)
	xmldist->distances.latency[j] *= xmldist->distances.latency_base;
      hwloc_distances_set(topology, objs[0]->type, nbobjs, indexes, objs, xmldist->distances.latency, 0 /* XML cannot force */);
    }

    next = xmldist->next;
    free(xmldist);
  }
  topology->backend_params.xml.first_distances = topology->backend_params.xml.last_distances = NULL;
}

/* this canNOT be the first XML call */
void
hwloc_look_xml(struct hwloc_topology *topology)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_IMPORT");
  if (!env || !atoi(env)) {
    xmlNode* root_node;
    xmlDtd *dtd;

    dtd = xmlGetIntSubset((xmlDoc*) topology->backend_params.xml.doc);
    if (!dtd)
      fprintf(stderr, "Loading XML topology without DTD\n");
    else if (strcmp((char *) dtd->SystemID, "hwloc.dtd"))
      fprintf(stderr, "Loading XML topology with wrong DTD SystemID (%s instead of %s)\n",
	      (char *) dtd->SystemID, "hwloc.dtd");

    root_node = xmlDocGetRootElement((xmlDoc*) topology->backend_params.xml.doc);

    hwloc__libxml_import_topology_node(topology, root_node);
    if (root_node->next)
      fprintf(stderr, "ignoring non-first root nodes\n");
  } else
#endif /* HWLOC_HAVE_XML */
  {
    char *buffer = topology->backend_params.xml.buffer;
    char *tag, *attr, *remaining;
    int closed;
    /* skip headers */
    while (!strncmp(buffer, "<?xml ", 6) || !strncmp(buffer, "<!DOCTYPE ", 10)) {
      buffer = strchr(buffer, '\n');
      if (!buffer)
	goto failed;
      buffer++;
    }
    /* find topology tag */
    tag = hwloc__nolibxml_import_find_tag(buffer, &attr, &remaining, &closed);
    if (!tag || strcmp(tag, "topology") || attr || !remaining || closed)
      goto failed;
    /* find object tag */
    tag = hwloc__nolibxml_import_find_tag(remaining, &attr, &remaining, &closed);
    if (!tag || strcmp(tag, "object"))
      goto failed;
    /* find end of topology tag */
    remaining = hwloc__nolibxml_import_object(topology, topology->levels[0][0], attr, remaining, closed);
    if (!remaining || !strncmp(remaining, "</topology>", 11))
      goto failed;
  }

  /* keep the "Backend" information intact */
  /* we could add "BackendSource=XML" to notify that XML was used between the actual backend and here */

  /* if we added some distances, we must check them, and make them groupable */
  hwloc_xml__handle_distances(topology);

  topology->support.discovery->pu = 1;

  return;

 failed:
  /* FIXME what if XML warnings are disabled? */
  fprintf(stderr, "failed to parse, try with libxml if it wasn't enabled\n");
  /* FIXME better message */
}

/************************************************
 ********* XML export (common routines) *********
 ************************************************/

typedef struct hwloc__xml_export_output_s {
  int use_libxml;
  /* only useful if not using libxml */
  char *buffer;
  size_t written;
  size_t remaining;
  unsigned indent;
  /* only useful if not using libxml */
#ifdef HWLOC_HAVE_XML
  xmlNodePtr current_node;
#endif
} * hwloc__xml_export_output_t;

static void
hwloc__xml_export_update_buffer(hwloc__xml_export_output_t output, int res)
{
  if (res >= 0) {
    output->written += res;
    if (res >= (int) output->remaining)
      res = output->remaining>0 ? output->remaining-1 : 0;
    output->buffer += res;
    output->remaining -= res;
  }
}

static void
hwloc__xml_export_new_child(hwloc__xml_export_output_t output, const char *name)
{
  if (output->use_libxml) {
#ifdef HWLOC_HAVE_XML
    output->current_node = xmlNewChild(output->current_node, NULL, BAD_CAST name, NULL);
#else
    assert(0);
#endif
  } else {
    int res = snprintf(output->buffer, output->remaining, "%*s<%s", output->indent, "", name);
    hwloc__xml_export_update_buffer(output, res);
    output->indent += 2;
  }
}

static char *
hwloc__xml_export_escape_string(const char *src)
{
  int fulllen, sublen;
  char *escaped, *dst;

  fulllen = strlen(src);

  sublen = strcspn(src, "\n\r\t\"<>&");
  if (sublen == fulllen)
    return NULL; /* nothing to escape */

  escaped = malloc(fulllen*6+1); /* escaped chars are replaced by at most 6 char */
  dst = escaped;

  memcpy(dst, src, sublen);
  src += sublen;
  dst += sublen;

  while (*src) {
    int replen;
    switch (*src) {
    case '\n': strcpy(dst, "&#10;");  replen=5; break;
    case '\r': strcpy(dst, "&#13;");  replen=5; break;
    case '\t': strcpy(dst, "&#9;");   replen=4; break;
    case '\"': strcpy(dst, "&quot;"); replen=6; break;
    case '<':  strcpy(dst, "&lt;");   replen=4; break;
    case '>':  strcpy(dst, "&gt;");   replen=4; break;
    case '&':  strcpy(dst, "&amp;");  replen=5; break;
    default: break;
    }
    dst+=replen; src++;

    sublen = strcspn(src, "\n\r\t\"<>&");
    memcpy(dst, src, sublen);
    src += sublen;
    dst += sublen;
  }

  *dst = 0;
  return escaped;
}

static void
hwloc__xml_export_new_prop(hwloc__xml_export_output_t output, const char *name, const char *value)
{
  if (output->use_libxml) {
#ifdef HWLOC_HAVE_XML
    xmlNewProp(output->current_node, BAD_CAST name, BAD_CAST value);
#else
    assert(0);
#endif
  } else {
    char *escaped = hwloc__xml_export_escape_string(value);
    int res = snprintf(output->buffer, output->remaining, " %s=\"%s\"", name, escaped ? escaped : value);
    hwloc__xml_export_update_buffer(output, res);
    free(escaped);
  }
}

static void
hwloc__xml_export_end_props(hwloc__xml_export_output_t output)
{
  if (output->use_libxml) {
#ifdef HWLOC_HAVE_XML
    /* nothing to do */
#else
    assert(0);
#endif
  } else {
    int res = snprintf(output->buffer, output->remaining, ">\n");
    hwloc__xml_export_update_buffer(output, res);
  }
}

static void
hwloc__xml_export_end_child(hwloc__xml_export_output_t output, const char *name)
{
  if (output->use_libxml) {
#ifdef HWLOC_HAVE_XML
    output->current_node = output->current_node->parent;
#else
    assert(0);
#endif
  } else {
    int res;
    output->indent -= 2;
    res = snprintf(output->buffer, output->remaining, "%*s</%s>\n", output->indent, "", name);
    hwloc__xml_export_update_buffer(output, res);
  }
}

static void
hwloc__xml_export_object (hwloc__xml_export_output_t output, hwloc_topology_t topology, hwloc_obj_t obj)
{
  char *cpuset = NULL;
  char tmp[255];
  unsigned i;

  hwloc__xml_export_new_child(output, "object");
  hwloc__xml_export_new_prop(output, "type", hwloc_obj_type_string(obj->type));
  if (obj->os_level != -1) {
    sprintf(tmp, "%d", obj->os_level);
    hwloc__xml_export_new_prop(output, "os_level", tmp);
  }
  if (obj->os_index != (unsigned) -1) {
    sprintf(tmp, "%u", obj->os_index);
    hwloc__xml_export_new_prop(output, "os_index", tmp);
  }
  if (obj->cpuset) {
    hwloc_bitmap_asprintf(&cpuset, obj->cpuset);
    hwloc__xml_export_new_prop(output, "cpuset", cpuset);
    free(cpuset);
  }
  if (obj->complete_cpuset) {
    hwloc_bitmap_asprintf(&cpuset, obj->complete_cpuset);
    hwloc__xml_export_new_prop(output, "complete_cpuset", cpuset);
    free(cpuset);
  }
  if (obj->online_cpuset) {
    hwloc_bitmap_asprintf(&cpuset, obj->online_cpuset);
    hwloc__xml_export_new_prop(output, "online_cpuset", cpuset);
    free(cpuset);
  }
  if (obj->allowed_cpuset) {
    hwloc_bitmap_asprintf(&cpuset, obj->allowed_cpuset);
    hwloc__xml_export_new_prop(output, "allowed_cpuset", cpuset);
    free(cpuset);
  }
  if (obj->nodeset && !hwloc_bitmap_isfull(obj->nodeset)) {
    hwloc_bitmap_asprintf(&cpuset, obj->nodeset);
    hwloc__xml_export_new_prop(output, "nodeset", cpuset);
    free(cpuset);
  }
  if (obj->complete_nodeset && !hwloc_bitmap_isfull(obj->complete_nodeset)) {
    hwloc_bitmap_asprintf(&cpuset, obj->complete_nodeset);
    hwloc__xml_export_new_prop(output, "complete_nodeset", cpuset);
    free(cpuset);
  }
  if (obj->allowed_nodeset && !hwloc_bitmap_isfull(obj->allowed_nodeset)) {
    hwloc_bitmap_asprintf(&cpuset, obj->allowed_nodeset);
    hwloc__xml_export_new_prop(output, "allowed_nodeset", cpuset);
    free(cpuset);
  }

  if (obj->name)
    hwloc__xml_export_new_prop(output, "name", obj->name);

  switch (obj->type) {
  case HWLOC_OBJ_CACHE:
    sprintf(tmp, "%llu", (unsigned long long) obj->attr->cache.size);
    hwloc__xml_export_new_prop(output, "cache_size", tmp);
    sprintf(tmp, "%u", obj->attr->cache.depth);
    hwloc__xml_export_new_prop(output, "depth", tmp);
    sprintf(tmp, "%u", (unsigned) obj->attr->cache.linesize);
    hwloc__xml_export_new_prop(output, "cache_linesize", tmp);
    sprintf(tmp, "%d", (unsigned) obj->attr->cache.associativity);
    hwloc__xml_export_new_prop(output, "cache_associativity", tmp);
    break;
  case HWLOC_OBJ_GROUP:
    sprintf(tmp, "%u", obj->attr->group.depth);
    hwloc__xml_export_new_prop(output, "depth", tmp);
    break;
  case HWLOC_OBJ_BRIDGE:
    sprintf(tmp, "%u-%u", obj->attr->bridge.upstream_type, obj->attr->bridge.downstream_type);
    hwloc__xml_export_new_prop(output, "bridge_type", tmp);
    sprintf(tmp, "%u", obj->attr->bridge.depth);
    hwloc__xml_export_new_prop(output, "depth", tmp);
    if (obj->attr->bridge.downstream_type == HWLOC_OBJ_BRIDGE_PCI) {
      sprintf(tmp, "%04x:[%02x-%02x]",
	      (unsigned) obj->attr->bridge.downstream.pci.domain,
	      (unsigned) obj->attr->bridge.downstream.pci.secondary_bus,
	      (unsigned) obj->attr->bridge.downstream.pci.subordinate_bus);
      hwloc__xml_export_new_prop(output, "bridge_pci", tmp);
    }
    if (obj->attr->bridge.upstream_type != HWLOC_OBJ_BRIDGE_PCI)
      break;
    /* fallthrough */
  case HWLOC_OBJ_PCI_DEVICE:
    sprintf(tmp, "%04x:%02x:%02x.%01x",
	    (unsigned) obj->attr->pcidev.domain,
	    (unsigned) obj->attr->pcidev.bus,
	    (unsigned) obj->attr->pcidev.dev,
	    (unsigned) obj->attr->pcidev.func);
    hwloc__xml_export_new_prop(output, "pci_busid", tmp);
    sprintf(tmp, "%04x [%04x:%04x] [%04x:%04x] %02x",
	    (unsigned) obj->attr->pcidev.class_id,
	    (unsigned) obj->attr->pcidev.vendor_id, (unsigned) obj->attr->pcidev.device_id,
	    (unsigned) obj->attr->pcidev.subvendor_id, (unsigned) obj->attr->pcidev.subdevice_id,
	    (unsigned) obj->attr->pcidev.revision);
    hwloc__xml_export_new_prop(output, "pci_type", tmp);
    sprintf(tmp, "%f", obj->attr->pcidev.linkspeed);
    hwloc__xml_export_new_prop(output, "pci_link_speed", tmp);
    break;
  case HWLOC_OBJ_OS_DEVICE:
    sprintf(tmp, "%u", obj->attr->osdev.type);
    hwloc__xml_export_new_prop(output, "osdev_type", tmp);
    break;
  default:
    break;
  }

  if (obj->memory.local_memory) {
    sprintf(tmp, "%llu", (unsigned long long) obj->memory.local_memory);
    hwloc__xml_export_new_prop(output, "local_memory", tmp);
  }

  hwloc__xml_export_end_props(output);

  for(i=0; i<obj->memory.page_types_len; i++) {
    hwloc__xml_export_new_child(output, "page_type");
    sprintf(tmp, "%llu", (unsigned long long) obj->memory.page_types[i].size);
    hwloc__xml_export_new_prop(output, "size", tmp);
    sprintf(tmp, "%llu", (unsigned long long) obj->memory.page_types[i].count);
    hwloc__xml_export_new_prop(output, "count", tmp);
    hwloc__xml_export_end_props(output);
    hwloc__xml_export_end_child(output, "page_type");
  }

  for(i=0; i<obj->infos_count; i++) {
    hwloc__xml_export_new_child(output, "info");
    hwloc__xml_export_new_prop(output, "name", obj->infos[i].name);
    hwloc__xml_export_new_prop(output, "value", obj->infos[i].value);
    hwloc__xml_export_end_props(output);
    hwloc__xml_export_end_child(output, "info");
  }

  for(i=0; i<obj->distances_count; i++) {
    unsigned nbobjs = obj->distances[i]->nbobjs;
    unsigned j;
    hwloc__xml_export_new_child(output, "distances");
    sprintf(tmp, "%u", nbobjs);
    hwloc__xml_export_new_prop(output, "nbobjs", tmp);
    sprintf(tmp, "%u", obj->distances[i]->relative_depth);
    hwloc__xml_export_new_prop(output, "relative_depth", tmp);
    sprintf(tmp, "%f", obj->distances[i]->latency_base);
    hwloc__xml_export_new_prop(output, "latency_base", tmp);
    hwloc__xml_export_end_props(output);
    for(j=0; j<nbobjs*nbobjs; j++) {
      hwloc__xml_export_new_child(output, "latency");
      sprintf(tmp, "%f", obj->distances[i]->latency[j]);
      hwloc__xml_export_new_prop(output, "value", tmp);
      hwloc__xml_export_end_props(output);
      hwloc__xml_export_end_child(output, "latency");
    }
    hwloc__xml_export_end_child(output, "distances");
  }

  if (obj->arity) {
    unsigned x;
    for (x=0; x<obj->arity; x++)
      hwloc__xml_export_object (output, topology, obj->children[x]);
  }

  hwloc__xml_export_end_child(output, "object");
}

/************************************************
 ********* XML export (libxml2 routines) ********
 ************************************************/

#ifdef HWLOC_HAVE_XML
/* libxml2 specific export preparation */
static xmlDocPtr
hwloc__libxml2_prepare_export(hwloc_topology_t topology)
{
  struct hwloc__xml_export_output_s output;
  xmlDocPtr doc = NULL;       /* document pointer */
  xmlNodePtr root_node = NULL; /* root pointer */

  LIBXML_TEST_VERSION;
  hwloc_libxml2_disable_stderrwarnings();

  /* Creates a new document, a node and set it as a root node. */
  doc = xmlNewDoc(BAD_CAST "1.0");
  root_node = xmlNewNode(NULL, BAD_CAST "topology");
  xmlDocSetRootElement(doc, root_node);

  /* Creates a DTD declaration. Isn't mandatory. */
  (void) xmlCreateIntSubset(doc, BAD_CAST "topology", NULL, BAD_CAST "hwloc.dtd");

  output.use_libxml = 1;
  output.current_node = root_node;
  hwloc__xml_export_object (&output, topology, hwloc_get_root_obj(topology));

  return doc;
}
#endif /* HWLOC_HAVE_XML */

/***************************************************
 ********* XML export (NO-libxml2 routines) ********
 ***************************************************/

static size_t
hwloc___nolibxml_prepare_export(hwloc_topology_t topology, char *xmlbuffer, int buflen)
{
  struct hwloc__xml_export_output_s output;
  int res;

  output.use_libxml = 0;
  output.indent = 0;
  output.written = 0;
  output.buffer = xmlbuffer;
  output.remaining = buflen;

  res = snprintf(output.buffer, output.remaining,
		 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		 "<!DOCTYPE topology SYSTEM \"hwloc.dtd\">\n");
  hwloc__xml_export_update_buffer(&output, res);
  hwloc__xml_export_new_child(&output, "topology");
  hwloc__xml_export_end_props(&output);
  hwloc__xml_export_object (&output, topology, hwloc_get_root_obj(topology));
  hwloc__xml_export_end_child(&output, "topology");

  return output.written+1;
}

static void
hwloc__nolibxml_prepare_export(hwloc_topology_t topology, char **bufferp, int *buflenp)
{
  char *buffer;
  size_t bufferlen, res;

  bufferlen = 16384; /* random guess for large enough default */
  buffer = malloc(bufferlen);
  res = hwloc___nolibxml_prepare_export(topology, buffer, bufferlen);

  if (res > bufferlen) {
    buffer = realloc(buffer, res);
    hwloc___nolibxml_prepare_export(topology, buffer, res);
  }

  *bufferp = buffer;
  *buflenp = res;
}

/**********************************
 ********* main XML export ********
 **********************************/

/* this can be the first XML call */
int hwloc_topology_export_xml(hwloc_topology_t topology, const char *filename)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_EXPORT");
  if (!env || !atoi(env)) {
    xmlDocPtr doc;
    int ret;

    errno = 0; /* set to 0 so that we know if libxml2 changed it */

    doc = hwloc__libxml2_prepare_export(topology);
    ret = xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
    xmlFreeDoc(doc);

    if (ret < 0) {
      if (!errno)
	/* libxml2 likely got an error before doing I/O */
	errno = EINVAL;
      return -1;
    }
  } else
#endif
  {
    FILE *file;
    char *buffer;
    int bufferlen;

    if (!strcmp(filename, "-")) {
      file = stdout;
    } else {
      file = fopen(filename, "w");
      if (!file)
        return -1;
    }

    hwloc__nolibxml_prepare_export(topology, &buffer, &bufferlen);
    fwrite(buffer, bufferlen-1 /* don't write the ending \0 */, 1, file);
    free(buffer);

    if (file != stdout)
      fclose(file);
  }

  return 0;
}

/* this can be the first XML call */
int hwloc_topology_export_xmlbuffer(hwloc_topology_t topology, char **xmlbuffer, int *buflen)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_EXPORT");
  if (!env || !atoi(env)) {
    xmlDocPtr doc = hwloc__libxml2_prepare_export(topology);
    xmlDocDumpFormatMemoryEnc(doc, (xmlChar **)xmlbuffer, buflen, "UTF-8", 1);
    xmlFreeDoc(doc);
  } else
#endif
  {
    hwloc__nolibxml_prepare_export(topology, xmlbuffer, buflen);
  }
  return 0;
}

void hwloc_free_xmlbuffer(hwloc_topology_t topology __hwloc_attribute_unused, char *xmlbuffer)
{
#ifdef HWLOC_HAVE_XML
  char *env = getenv("HWLOC_NO_LIBXML_EXPORT");
  if (!env || !atoi(env)) {
    xmlFree(BAD_CAST xmlbuffer);
  } else
#endif
  {
    free(xmlbuffer);
  }
}
