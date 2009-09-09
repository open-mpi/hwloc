/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* The configuration file */

#ifndef HWLOC_DEBUG_H
#define HWLOC_DEBUG_H

#include <private/config.h>

#ifdef HWLOC_DEBUG
#define hwloc_debug(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
#else
#define hwloc_debug(s, ...) do { }while(0)
#endif

#endif /* HWLOC_DEBUG_H */
