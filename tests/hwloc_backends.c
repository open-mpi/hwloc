/*
 * Copyright © 2011 INRIA.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/* mostly useful with valgrind, to check if backend cleanup properly */

int main(void)
{
  hwloc_topology_t topology;
  char *xmlbuf;
  int xmlbuflen;
  char xmlfile[] = "hwloc_backends.tmpxml.XXXXXX";
  int xmlbufok = 0, xmlfileok = 0;

  printf("trying to export topology to XML buffer and file for later...\n");
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  if (hwloc_topology_export_xmlbuffer(topology, &xmlbuf, &xmlbuflen) < 0)
    printf("XML buffer export failed (%s), ignoring\n", strerror(errno));
  else
    xmlbufok = 1;
  mktemp(xmlfile);
  if (hwloc_topology_export_xml(topology, xmlfile) < 0)
    printf("XML file export failed (%s), ignoring\n", strerror(errno));
  else
    xmlfileok = 1;
  hwloc_topology_destroy(topology);

  printf("init...\n");
  hwloc_topology_init(&topology);
  if (xmlfileok) {
    printf("switching to xml...\n");
    hwloc_topology_set_xml(topology, xmlfile);
  }
  if (xmlbufok) {
    printf("switching to xmlbuffer...\n");
    hwloc_topology_set_xmlbuffer(topology, xmlbuf, xmlbuflen);
  }
  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");
  printf("switching sysfs fsroot...\n");
  hwloc_topology_set_fsroot(topology, "/");

  if (xmlfileok) {
    printf("switching to xml and loading...\n");
    hwloc_topology_set_xml(topology, xmlfile);
    hwloc_topology_load(topology);
  }
  if (xmlbufok) {
    printf("switching to xmlbuffer and loading...\n");
    hwloc_topology_set_xmlbuffer(topology, xmlbuf, xmlbuflen);
    hwloc_topology_load(topology);
  }
  printf("switching to synthetic and loading...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");
  hwloc_topology_load(topology);
  printf("switching sysfs fsroot and loading...\n");
  hwloc_topology_set_fsroot(topology, "/");
  hwloc_topology_load(topology);

  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology, "machine:2 node:3 cache:2 pu:4");

  hwloc_topology_destroy(topology);

  if (xmlbufok)
    hwloc_free_xmlbuffer(topology, xmlbuf);
  if (xmlfileok)
    unlink(xmlfile);

  return 0;
}
