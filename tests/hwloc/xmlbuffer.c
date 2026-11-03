/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2010-2020 Inria.  All rights reserved.
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hwloc.h"

static int one_test(void)
{
  hwloc_topology_t topology;
  int size1, size2;
  char *buf1, *copy1, *buf2;
  int err = 0, i;
  char s[129];
  char t[10];

  for(i=0; i<128; i++)
    s[i] = ' ';
  s[128] = 0;
  for(i=32; i<=126; i++)
    s[i] = i;
  s['\t'] = '\t';
  s['\n'] = '\n';
  s['\r'] = '\r';

  t[0] = 'x';
  for(i=1; i<=7; i++)
    t[i] = i;
  t[8] = 'y';
  t[9] = '\0';

  hwloc_topology_init(&topology);
  hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
  hwloc_topology_load(topology);
  assert(hwloc_topology_is_thissystem(topology));
  hwloc_obj_add_info(hwloc_get_root_obj(topology), "UglyString", s);
  hwloc_obj_add_info(hwloc_get_root_obj(topology), "UberUglyString", t);
  hwloc_topology_export_xmlbuffer(topology, &buf1, &size1, 0);
  hwloc_topology_destroy(topology);
  printf("exported to buffer %p length %d\n", buf1, size1);

  /* copy the returned buffer to a newly malloc'd one
   * to check that the returned length is correct (contains ending 0, etc).
   */
  copy1 = malloc(size1);
  assert(copy1);
  memcpy(copy1, buf1, size1);

  hwloc_topology_init(&topology);
  hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
  assert(!hwloc_topology_set_xmlbuffer(topology, copy1, size1));
  hwloc_topology_load(topology);
  assert(!hwloc_topology_is_thissystem(topology));
  if (strcmp(hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "UglyString"), s))
    assert(0);
  if (strcmp(hwloc_obj_get_info_by_name(hwloc_get_root_obj(topology), "UberUglyString"), "xy"))
    assert(0);
  hwloc_topology_export_xmlbuffer(topology, &buf2, &size2, 0);
  printf("re-exported to buffer %p length %d\n", buf2, size2);

  if (strcmp(buf1, buf2)) {
    printf("### First exported buffer is:\n");
    printf("%s", buf1);
    printf("### End of first export buffer\n");
    printf("### Second exported buffer is:\n");
    printf("%s", buf2);
    printf("### End of second export buffer\n");
    err = 1;
  }

  hwloc_free_xmlbuffer(topology, buf1);
  hwloc_free_xmlbuffer(topology, buf2);
  free(copy1);

  hwloc_topology_destroy(topology);

  return err;
}

/* an invalid pci_busid makes the importer ignore the object,
 * the root cannot be ignored and freed, the load must fail instead */
static void invalid_root_test(void)
{
  static const char xml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE topology SYSTEM \"hwloc2.dtd\">\n"
    "<topology version=\"3.0\">\n"
    "  <object type=\"PCIDev\" pci_busid=\"x\"/>\n"
    "</topology>\n";
  hwloc_topology_t topology;
  int err;

  hwloc_topology_init(&topology);
  err = hwloc_topology_set_xmlbuffer(topology, xml, sizeof(xml));
  assert(!err);
  err = hwloc_topology_load(topology);
  assert(err < 0);
  hwloc_topology_destroy(topology);
}

/* print the type and attributes of obj and all its children, whatever kind */
static void print_all(hwloc_topology_t topology, hwloc_obj_t obj)
{
  hwloc_obj_t child;
  char buf[256];
  hwloc_obj_type_snprintf(buf, sizeof(buf), obj, 1);
  hwloc_obj_attr_snprintf(buf, sizeof(buf), obj, " ", 1);
  for(child = obj->first_child; child; child = child->next_sibling)
    print_all(topology, child);
  for(child = obj->memory_first_child; child; child = child->next_sibling)
    print_all(topology, child);
  for(child = obj->io_first_child; child; child = child->next_sibling)
    print_all(topology, child);
  for(child = obj->misc_first_child; child; child = child->next_sibling)
    print_all(topology, child);
}

/* a bridge may only have a PCI downstream (and a HOST or PCI upstream).
 * a crafted bridge with another downstream type used to be imported as-is
 * and then abort on an assertion (or read uninitialised stack in a NDEBUG
 * build) as soon as its type/attributes were printed. it must be ignored
 * at import instead, so the loaded topology holds no such bridge.
 */
static void invalid_bridge_test(void)
{
  static const char xml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE topology SYSTEM \"hwloc2.dtd\">\n"
    "<topology version=\"3.0\">\n"
    "  <object type=\"Machine\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" allowed_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" allowed_nodeset=\"0x1\" gp_index=\"1\">\n"
    "    <object type=\"NUMANode\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" gp_index=\"2\" local_memory=\"1048576\"/>\n"
    "    <object type=\"PU\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" gp_index=\"3\"/>\n"
    "    <object type=\"Bridge\" gp_index=\"4\">\n"
    "      <object type=\"PCIDev\" gp_index=\"5\" pci_busid=\"0000:01:00.0\" pci_type=\"0200 [8086:1521] [8086:0001] 01\"/>\n"
    "    </object>\n"
    "  </object>\n"
    "</topology>\n";
  hwloc_topology_t topology;
  int err;

  hwloc_topology_init(&topology);
  /* keep bridges so that the invalid one would reach the printers if not ignored */
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_BRIDGE, HWLOC_TYPE_FILTER_KEEP_ALL);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PCI_DEVICE, HWLOC_TYPE_FILTER_KEEP_ALL);
  err = hwloc_topology_set_xmlbuffer(topology, xml, sizeof(xml));
  assert(!err);
  err = hwloc_topology_load(topology);
  assert(!err);

  /* the invalid bridge must have been dropped at import */
  assert(hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_BRIDGE) == 0);

  /* exercise the attribute/type printers on every object, this is where
   * an invalid bridge used to abort or overread.
   */
  print_all(topology, hwloc_get_root_obj(topology));

  hwloc_topology_destroy(topology);
}

int main(int argc, char *argv[])
{
  int err;

  if (argc < 3) {
    fprintf(stderr, "Need 0 or 1 twice as arguments for enabling/disabling libxml import and export\n");
    fprintf(stderr, "For instance `xmlbuffer 0 1' enables libxml for export only\n");
    fprintf(stderr, "Those arguments are passed by wrapper.sh during make check\n");
    exit(EXIT_FAILURE);
  }

  putenv((char *) "HWLOC_LIBXML_CLEANUP=1");

  if (atoi(argv[1])) {
    putenv((char *) "HWLOC_LIBXML_IMPORT=1");
    printf("import=libxml   ");
  } else {
    putenv((char *) "HWLOC_LIBXML_IMPORT=0");
    printf("import=nolibxml ");
  }

  if (atoi(argv[2])) {
    putenv((char *) "HWLOC_LIBXML_EXPORT=1");
    printf("export=libxml\n");
  } else {
    putenv((char *) "HWLOC_LIBXML_EXPORT=0");
    printf("export=nolibxml\n");
  }

  err = one_test();
  if (err < 0)
    return err;

  invalid_root_test();

  invalid_bridge_test();

  return 0;
}
