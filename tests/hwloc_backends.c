/*
 * Copyright © 2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <assert.h>

/* mostly useful with valgrind, to check if backend cleanup properly */

int main(void)
{
  hwloc_topology_t topology1, topology2;
  char *xmlbuf;
  int xmlbuflen;
  char xmlfile[] = "hwloc_backends.tmpxml.XXXXXX";
  int xmlbufok = 0, xmlfileok = 0;
  hwloc_obj_t sw;
  int err;

  printf("trying to export topology to XML buffer and file for later...\n");
  hwloc_topology_init(&topology1);
  hwloc_topology_load(topology1);
  assert(hwloc_topology_is_thissystem(topology1));
  if (hwloc_topology_export_xmlbuffer(topology1, &xmlbuf, &xmlbuflen) < 0)
    printf("XML buffer export failed (%s), ignoring\n", strerror(errno));
  else
    xmlbufok = 1;
  mktemp(xmlfile);
  if (hwloc_topology_export_xml(topology1, xmlfile) < 0)
    printf("XML file export failed (%s), ignoring\n", strerror(errno));
  else
    xmlfileok = 1;


  printf("init...\n");
  hwloc_topology_init(&topology2);
  if (xmlfileok) {
    printf("switching to xml...\n");
    assert(!hwloc_topology_set_xml(topology2, xmlfile));
  }
  if (xmlbufok) {
    printf("switching to xmlbuffer...\n");
    assert(!hwloc_topology_set_xmlbuffer(topology2, xmlbuf, xmlbuflen));
  }
  printf("switching to custom...\n");
  hwloc_topology_set_custom(topology2);
  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology2, "machine:2 node:3 cache:2 pu:4");
  printf("switching sysfs fsroot to // ...\n");
  hwloc_topology_set_fsroot(topology2, "//"); /* valid path that won't be recognized as '/' */
  printf("switching sysfs fsroot to / ...\n");
  hwloc_topology_set_fsroot(topology2, "/");

  if (xmlfileok) {
    printf("switching to xml and loading...\n");
    assert(!hwloc_topology_set_xml(topology2, xmlfile));
    hwloc_topology_load(topology2);
    hwloc_topology_check(topology2);
    assert(!hwloc_topology_is_thissystem(topology2));
  }
  if (xmlbufok) {
    printf("switching to xmlbuffer and loading...\n");
    assert(!hwloc_topology_set_xmlbuffer(topology2, xmlbuf, xmlbuflen));
    hwloc_topology_load(topology2);
    hwloc_topology_check(topology2);
    assert(!hwloc_topology_is_thissystem(topology2));
  }
  printf("switching to custom and loading...\n");
  hwloc_topology_set_custom(topology2);
  sw = hwloc_custom_insert_group_object_by_parent(topology2, hwloc_get_root_obj(topology2), 0);
  assert(sw);
  hwloc_custom_insert_topology(topology2, sw, topology1, NULL);
  hwloc_topology_load(topology2);
  hwloc_topology_check(topology2);
  assert(!hwloc_topology_is_thissystem(topology2));
  /* don't try fsroot here because it fails on !linux, we would revert back to custom, which requires some insert to make the topology valid */
  printf("switching to synthetic and loading...\n");
  hwloc_topology_set_synthetic(topology2, "machine:2 node:3 cache:2 pu:4");
  hwloc_topology_load(topology2);
  hwloc_topology_check(topology2);
  assert(!hwloc_topology_is_thissystem(topology2));
  printf("switching sysfs fsroot to // and loading...\n");
  hwloc_topology_set_fsroot(topology2, "//"); /* valid path that won't be recognized as '/' */
  hwloc_topology_load(topology2);
  hwloc_topology_check(topology2);
  assert(!hwloc_topology_is_thissystem(topology2)); /* earlier fsroot worked, or we're still synthetic */
  printf("switching sysfs fsroot to / and loading...\n");
  err = hwloc_topology_set_fsroot(topology2, "/");
  hwloc_topology_load(topology2);
  hwloc_topology_check(topology2);
  assert(hwloc_topology_is_thissystem(topology2) == !err); /* on Linux, '/' is recognized as thissystem. on !Linux, set_fsroot() failed and we went back to synthetic */

  printf("switching to synthetic...\n");
  hwloc_topology_set_synthetic(topology2, "machine:2 node:3 cache:2 pu:4");

  hwloc_topology_destroy(topology2);


  if (xmlbufok)
    hwloc_free_xmlbuffer(topology1, xmlbuf);
  if (xmlfileok)
    unlink(xmlfile);
  hwloc_topology_destroy(topology1);

  return 0;
}
