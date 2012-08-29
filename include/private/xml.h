/*
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef PRIVATE_XML_H
#define PRIVATE_XML_H 1

struct hwloc_topology;
struct hwloc_obj;

#include <sys/types.h>

#ifdef HWLOC_HAVE_LIBXML2
#include <libxml/tree.h>
#endif

extern int hwloc__xml_verbose(void);

typedef struct hwloc__xml_import_state_s {
  struct hwloc__xml_import_state_s *parent;

  int (*next_attr)(struct hwloc__xml_import_state_s * state, char **namep, char **valuep);
  int (*find_child)(struct hwloc__xml_import_state_s * state, struct hwloc__xml_import_state_s * childstate, char **tagp);
  int (*close_tag)(struct hwloc__xml_import_state_s * state); /* look for an explicit closing tag </name> */
  void (*close_child)(struct hwloc__xml_import_state_s * state);
  int (*get_content)(struct hwloc__xml_import_state_s * state, char **beginp, size_t expected_length);
  void (*close_content)(struct hwloc__xml_import_state_s * state);

  /* opaque data used to store backend-specific data.
   * statically allocated to allow stack-allocation by the common code without knowing actual backend needs.
   */
  char data[32];
} * hwloc__xml_import_state_t;

typedef struct hwloc__xml_export_state_s {
  struct hwloc__xml_export_state_s *parent;

  void (*new_child)(struct hwloc__xml_export_state_s *parentstate, struct hwloc__xml_export_state_s *state, const char *name);
  void (*new_prop)(struct hwloc__xml_export_state_s *state, const char *name, const char *value);
  void (*add_content)(struct hwloc__xml_export_state_s *state, const char *buffer, size_t length);
  void (*end_object)(struct hwloc__xml_export_state_s *state, const char *name);

  /* opaque data used to store backend-specific data.
   * statically allocated to allow stack-allocation by the common code without knowing actual backend needs.
   */
  char data[40];
} * hwloc__xml_export_state_t;

extern void hwloc__xml_export_object (hwloc__xml_export_state_t state, struct hwloc_topology *topology, struct hwloc_obj *obj);

extern int hwloc_nolibxml_backend_init(struct hwloc_topology *topology, const char *xmlpath, const char *xmlbuffer, int xmlbuflen);
extern int hwloc_nolibxml_export_file(struct hwloc_topology *topology, const char *filename);
extern int hwloc_nolibxml_export_buffer(struct hwloc_topology *topology, char **xmlbuffer, int *buflen);
extern void hwloc_nolibxml_free_buffer(void *xmlbuffer);

#ifdef HWLOC_HAVE_LIBXML2
extern int hwloc_libxml_backend_init(struct hwloc_topology *topology, const char *xmlpath, const char *xmlbuffer, int xmlbuflen);
extern int hwloc_libxml_export_file(struct hwloc_topology *topology, const char *filename);
extern int hwloc_libxml_export_buffer(struct hwloc_topology *topology, char **xmlbuffer, int *buflen);
extern void hwloc_libxml_free_buffer(void *xmlbuffer);
#endif

#endif /* PRIVATE_XML_H */
