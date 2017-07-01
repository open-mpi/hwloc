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

char* hwloc_solaris_get_chip_type(void);
char* hwloc_solaris_get_chip_model(void);

#endif /* HWLOC_PRIVATE_SOLARIS_CHIPTYPE_H */
