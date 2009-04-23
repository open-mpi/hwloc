/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The configuration file */

#ifndef LIBTOPOLOGY_CONFIGURATION_H
#define LIBTOPOLOGY_CONFIGURATION_H

#include <limits.h>

#define LIBTOPO_NBMAXCPUS	32
#define LT_PER_LEVEL_ROOM	4096

#ifdef LT__NUMA
#  ifndef LIBTOPO_NBMAXNODES
#    define LIBTOPO_NBMAXNODES	8
#  endif
#endif

#define LIBTOPO_CACHE_LEVEL_MAX 3

#if ULONG_MAX == UINT8_MAX
#define LT_BITS_PER_LONG 8
#elif ULONG_MAX == UINT16_MAX
#define LT_BITS_PER_LONG 16
#elif ULONG_MAX == UINT32_MAX
#define LT_BITS_PER_LONG 32
#elif ULONG_MAX == UINT64_MAX
#define LT_BITS_PER_LONG 64
#else
#error "unknown size for unsigned long."
#endif

#endif /* LIBTOPOLOGY_CONFIGURATION_H */
