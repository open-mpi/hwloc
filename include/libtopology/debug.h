/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The configuration file */

#ifndef LIBTOPOLOGY_DEBUG_H
#define LIBTOPOLOGY_DEBUG_H

#include <config.h>

#ifdef TOPO_DEBUG
#define ltdebug(s, ...) fprintf(stderr, s, ##__VA_ARGS__) 
#else
#define ltdebug(s, ...) do { }while(0)
#endif

#endif /* LIBTOPOLOGY_DEBUG_H */
