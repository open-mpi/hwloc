/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <topology.h>

static void *
allocate_cleared (size_t size)
{
  return calloc (size, 1);
}

static void *
reallocate (void *ptr, size_t size)
{
  return realloc (ptr, size);
}

const struct topo_allocator topo_default_allocator =
  {
    .allocate = malloc,
    .allocate_cleared = allocate_cleared,
    .reallocate = reallocate,
    .free = free
  };

const struct topo_allocator *topo_allocator = &topo_default_allocator;
