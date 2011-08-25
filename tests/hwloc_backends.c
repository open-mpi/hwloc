/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>
#include <private/autogen/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#ifdef HWLOC_HAVE_XML
#include <libxml/xmlmemory.h>
#endif

/* mostly useful with valgrind, to check if backend cleanup properly */

int main(void)
{
  hwloc_topology_t topology;

#ifdef HWLOC_HAVE_XML
  char *xmlbuf;
  int xmlbuflen;
  char xmlfile[] = "hwloc_backends.tmpxml.XXXXXX";
  printf("exporting topology topology to XML buffer and file for later...\n");
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  hwloc_topology_export_xmlbuffer(topology, &xmlbuf, &xmlbuflen);
  mktemp(xmlfile);
  hwloc_topology_export_xml(topology, xmlfile);
  hwloc_topology_destroy(topology);
#endif

  printf("init...\n");
  hwloc_topology_init(&topology);
#ifdef HWLOC_HAVE_XML
  printf("switching to xml...\n");
  hwloc_topology_set_xml(topology, xmlfile);
  printf("switching to xmlbuffer...\n");
  hwloc_topology_set_xmlbuffer(topology, xmlbuf, xmlbuflen);
#endif
  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");
  printf("switching sysfs fsroot...\n");
  hwloc_topology_set_fsroot(topology, "/");

#ifdef HWLOC_HAVE_XML
  printf("switching to xml and loading...\n");
  hwloc_topology_set_xml(topology, xmlfile);
  hwloc_topology_load(topology);
  printf("switching to xmlbuffer and loading...\n");
  hwloc_topology_set_xmlbuffer(topology, xmlbuf, xmlbuflen);
  hwloc_topology_load(topology);
#endif
  printf("switching to synthetic and loading...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");
  hwloc_topology_load(topology);
  printf("switching sysfs fsroot and loading...\n");
  hwloc_topology_set_fsroot(topology, "/");
  hwloc_topology_load(topology);

  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");

  hwloc_topology_destroy(topology);

#ifdef HWLOC_HAVE_XML
  xmlFree(BAD_CAST xmlbuf);
  unlink(xmlfile);
#endif

  return 0;
}
