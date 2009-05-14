/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The configuration file */

#ifndef TOPOLOGY_DEBUG_H
#define TOPOLOGY_DEBUG_H

#include <config.h>

#ifdef TOPO_DEBUG
#define topo_debug(s, ...) fprintf(stderr, s, ##__VA_ARGS__) 
#else
#define topo_debug(s, ...) do { }while(0)
#endif

#endif /* TOPOLOGY_DEBUG_H */
