/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The memory allocation API, for use in libtopology itself.  */

#ifndef TOPOLOGY_ALLOCATOR_H
#define TOPOLOGY_ALLOCATOR_H

#include <stdlib.h>

struct topo_allocator
{
  void * (* allocate) (size_t);
  void * (* allocate_cleared) (size_t);
  void * (* reallocate) (void *, size_t);
  void   (* free) (void *);
};

extern const struct topo_allocator *topo_allocator;
extern const struct topo_allocator  topo_default_allocator;

#define topo_malloc(s)     (topo_allocator->allocate (s))
#define topo_calloc(s)     (topo_allocator->allocate_cleared (s))
#define topo_realloc(p, s) (topo_allocator->reallocate ((p), (s)))
#define topo_free(p)       (topo_allocator->free (s))

#endif /* TOPOLOGY_ALLOCATOR_H */
