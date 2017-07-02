/*
 * Copyright © 2009-2010 Oracle and/or its affiliates.  All rights reserved.
 *
 * Copyright © 2017 Inria.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#ifdef HWLOC_INSIDE_PLUGIN
/*
 * these declarations are internal only, they are not available to plugins
 * (functions below are internal static symbols).
 */
#error This file should not be used in plugins
#endif


#ifndef HWLOC_PRIVATE_SOLARIS_CHIPTYPE_H
#define HWLOC_PRIVATE_SOLARIS_CHIPTYPE_H

struct hwloc_solaris_chip_info_s {
  char *model;
  char *type;
};

/* fills the structure with 0 on error */
extern void hwloc_solaris_get_chip_info(struct hwloc_solaris_chip_info_s *info);

#endif /* HWLOC_PRIVATE_SOLARIS_CHIPTYPE_H */
