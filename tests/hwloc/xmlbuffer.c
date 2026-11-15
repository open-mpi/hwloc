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
  hwloc_obj_type_snprintf(buf, sizeof(buf), obj, HWLOC_OBJ_SNPRINTF_FLAG_LONG_NAMES, topology);
  hwloc_obj_attr_snprintf(buf, sizeof(buf), obj, " ", HWLOC_OBJ_SNPRINTF_FLAG_MORE_ATTRS, topology);
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

/* a v2 root may only preallocate memory tiers once. a second MemoryTiersNr
 * whose allocation failed used to leave the tier array NULL while keeping
 * the first count, which was then used to index that NULL array.
 */
static void duplicate_memtiers_nr_test(void)
{
  static const char xml[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE topology SYSTEM \"hwloc2.dtd\">\n"
    "<topology version=\"2.0\">\n"
    "  <object type=\"Machine\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" gp_index=\"1\">\n"
    "    <info name=\"MemoryTiersNr\" value=\"1\"/>\n"
    "    <info name=\"MemoryTiersNr\" value=\"-1\"/>\n" /* too large to allocate */
    "    <object type=\"NUMANode\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" gp_index=\"2\">\n"
    "      <info name=\"MemoryTier\" value=\"0\"/>\n"
    "    </object>\n"
    "    <object type=\"PU\" os_index=\"0\" cpuset=\"0x1\" complete_cpuset=\"0x1\" nodeset=\"0x1\" complete_nodeset=\"0x1\" gp_index=\"3\"/>\n"
    "  </object>\n"
    "</topology>\n";
  hwloc_topology_t topology;
  int err;

  hwloc_topology_init(&topology);
  err = hwloc_topology_set_xmlbuffer(topology, xml, sizeof(xml));
  assert(!err);
  err = hwloc_topology_load(topology);
  assert(!err);
  hwloc_topology_destroy(topology);
}

/* a diff buffer starting with a closing tag has no root element.
 * the minimalistic importer used to compare the uninitialized root tag
 * name in this case, and to parse the root attributes from an
 * uninitialized state, which could still point to a previous diff buffer.
 */
static void diff_without_root_test(void)
{
  static const char valid[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE topologydiff SYSTEM \"hwloc2-diff.dtd\">\n"
    "<topologydiff refname=\"foo\">\n"
    "  <diff type=\"0\" obj_depth=\"0\" obj_index=\"0\" obj_attr_type=\"2\" obj_attr_name=\"Foo\" obj_attr_oldvalue=\"Bar\" obj_attr_newvalue=\"Baz\"/>\n"
    "</topologydiff>\n";
  static const char noroot[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE topologydiff SYSTEM \"hwloc2-diff.dtd\">\n"
    "</topologydiff>\n";
  hwloc_topology_diff_t diff, diff2 = NULL;
  char *refname, *refname2 = NULL;
  int err;

  err = hwloc_topology_diff_load_xmlbuffer(valid, sizeof(valid), &diff, &refname);
  assert(!err);
  assert(diff);
  err = hwloc_topology_diff_load_xmlbuffer(noroot, sizeof(noroot), &diff2, &refname2);
  assert(err < 0);
  assert(!diff2);
  assert(!refname2);
  hwloc_topology_diff_destroy(diff);
  free(refname);
}

/* userdata buffers are only valid for length bytes, the exporter must not
 * read past them. the nolibxml exporter used to print the whole string,
 * writing more content than the length attribute and failing the reload.
 */
static void userdata_export_cb(void *reserved, hwloc_topology_t topology, hwloc_obj_t obj)
{
  int err = hwloc_export_obj_userdata(reserved, topology, obj, "Prefix", "abcdEXTRA", 4);
  assert(!err);
}

static int userdata_imported = 0;

static void userdata_import_cb(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_obj_t obj __hwloc_attribute_unused,
                               const char *name, const void *buffer, size_t length)
{
  assert(!strcmp(name, "Prefix"));
  assert(length == 4);
  assert(!memcmp(buffer, "abcd", 4));
  userdata_imported++;
}

static void userdata_length_test(void)
{
  hwloc_topology_t topology, reimport;
  char *buf;
  int size, err;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "pu:2");
  hwloc_topology_load(topology);
  hwloc_get_root_obj(topology)->userdata = (void *) 1;
  hwloc_topology_set_userdata_export_callback(topology, userdata_export_cb);
  err = hwloc_topology_export_xmlbuffer(topology, &buf, &size, 0);
  assert(!err);

  hwloc_topology_init(&reimport);
  hwloc_topology_set_userdata_import_callback(reimport, userdata_import_cb);
  err = hwloc_topology_set_xmlbuffer(reimport, buf, size);
  assert(!err);
  err = hwloc_topology_load(reimport);
  assert(!err);
  assert(userdata_imported == 1);
  hwloc_topology_destroy(reimport);

  hwloc_free_xmlbuffer(topology, buf);
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

  duplicate_memtiers_nr_test();

  diff_without_root_test();

  userdata_length_test();

  return 0;
}
