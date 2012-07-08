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

  /* only useful if not using libxml */
  char *tagbuffer; /* buffer containing the next tag */
  char *attrbuffer; /* buffer containing the next attribute of the current node */
  char *tagname; /* tag name of the current node */
  int closed; /* set if the current node is auto-closing */

  /* only useful if using libxml */
#ifdef HWLOC_HAVE_LIBXML2
  xmlNode *libxml_node; /* current libxml node, always valid */
  xmlNode *libxml_child; /* last processed child, or NULL if none yet */
  xmlAttr *libxml_attr; /* last processed attribute, or NULL if none yet */
#endif
} * hwloc__xml_import_state_t;

typedef struct hwloc__xml_export_output_s {
  void (*new_child)(struct hwloc__xml_export_output_s *output, const char *name);
  void (*new_prop)(struct hwloc__xml_export_output_s *output, const char *name, const char *value);
  void (*end_props)(struct hwloc__xml_export_output_s *output, unsigned nr_children);
  void (*end_child)(struct hwloc__xml_export_output_s *output, const char *name, unsigned nr_children);

  /* only useful if not using libxml */
  char *buffer; /* (moving) buffer where to write */
  size_t written; /* how many bytes were written (or would have be written if not truncated) */
  size_t remaining; /* how many bytes are still available in the buffer */
  unsigned indent; /* indentation level for the next line */

  /* only useful if not using libxml */
#ifdef HWLOC_HAVE_LIBXML2
  xmlNodePtr current_node; /* current node to output */
#endif
} * hwloc__xml_export_output_t;

extern void hwloc__xml_export_object (hwloc__xml_export_output_t output, struct hwloc_topology *topology, struct hwloc_obj *obj);

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
